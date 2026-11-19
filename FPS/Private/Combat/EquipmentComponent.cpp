// Copyright TemmieTim

#include "Combat/EquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "Data/HeroDataAsset.h"
#include "Weapon/Weapon.h"
#include "GameFramework/Character.h"
#include "Combat/HealthComponent.h"
#include "Character/ShooterCharacter.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	ActiveSlotIndex = 0;
	Slots.SetNum(NUM_EQUIPMENT_SLOTS);
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquipmentComponent, ActiveSlotIndex);
	DOREPLIFETIME(UEquipmentComponent, Slots);
}

bool UEquipmentComponent::ValidateLoadout(const TArray<FHeroEquipmentSlotConfig>& ProposedLoadout, FString& OutErrorReason)
{
	if (ProposedLoadout.Num() > NUM_EQUIPMENT_SLOTS)
	{
		OutErrorReason = TEXT("Loadout exceeds 4 equipment slots.");
		return false;
	}

	int32 SecondaryCount = 0;
	int32 MeleeCount = 0;

	for (int32 Index = 0; Index < ProposedLoadout.Num(); ++Index)
	{
		const FHeroEquipmentSlotConfig& Config = ProposedLoadout[Index];

		// Slot 1 (Index 0) must be Primary or None
		if (Index == 0 && Config.Category != EEquipmentCategory::Primary && Config.Category != EEquipmentCategory::None)
		{
			OutErrorReason = TEXT("Slot 1 is strictly reserved for Primary weapons.");
			return false;
		}

		if (Config.Category == EEquipmentCategory::Secondary)
		{
			SecondaryCount++;
		}
		else if (Config.Category == EEquipmentCategory::Melee)
		{
			MeleeCount++;
		}
	}

	if (SecondaryCount > 1)
	{
		OutErrorReason = TEXT("Loadout violates restriction: Maximum 1 Secondary weapon allowed.");
		return false;
	}

	if (MeleeCount > 1)
	{
		OutErrorReason = TEXT("Loadout violates restriction: Maximum 1 Melee weapon allowed.");
		return false;
	}

	return true;
}

void UEquipmentComponent::InitializeLoadout(const UHeroDataAsset* HeroData)
{
	if (!HeroData || !GetOwner() || !GetOwner()->HasAuthority()) return;

	FString ValidationError;
	if (!ValidateLoadout(HeroData->DefaultLoadout, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Hero [%s] loadout validation failed: %s"), *HeroData->InternalHeroID.ToString(), *ValidationError);
		return;
	}

	// Clean up any existing inventory
	DestroyInventory();

	Slots.Empty();
	Slots.SetNum(NUM_EQUIPMENT_SLOTS);

	APawn* OwningPawn = Cast<APawn>(GetOwner());

	for (int32 Index = 0; Index < HeroData->DefaultLoadout.Num() && Index < NUM_EQUIPMENT_SLOTS; ++Index)
	{
		const FHeroEquipmentSlotConfig& Config = HeroData->DefaultLoadout[Index];

		Slots[Index].Category = Config.Category;
		Slots[Index].EquipmentID = Config.EquipmentID;
		Slots[Index].bIsPassive = Config.bIsPassive;

		// 1. Passive Equipment: Stat injection directly into HealthComponent (No handheld actor spawned)
		if (Config.bIsPassive)
		{
			ApplyPassiveEquipment(Config.EquipmentID, true);
		}
		// 2. Handheld Weapons/Gadgets: Spawn and attach
		else if (Config.WeaponClass && OwningPawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = OwningPawn;
			SpawnParams.Instigator = OwningPawn;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(Config.WeaponClass, SpawnParams);
			if (SpawnedWeapon)
			{
				Slots[Index].SpawnedWeapon = SpawnedWeapon;
				SpawnedWeapon->AttachToOwningPawn(OwningPawn);
				SpawnedWeapon->SetActorHiddenInGame(true);
			}
		}

		OnEquipmentSlotUpdated.Broadcast(Index);
	}

	// Equip the first valid selectable slot (default Slot 1)
	ActiveSlotIndex = 0;
	EquipActiveSlot();
}

void UEquipmentComponent::ApplyPassiveEquipment(const FName& EquipmentID, bool bApply)
{
	AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetOwner());
	if (!ShooterChar) return;

	UHealthComponent* Health = ShooterChar->GetHealthComponent();
	if (!Health) return;

	const float Multiplier = bApply ? 1.0f : -1.0f;

	// Armor Plates: +50 Max Armor
	if (EquipmentID == FName("skm_gadget_armorplates"))
	{
		Health->ModifyMaxArmorBonus(50.f * Multiplier);
	}
	// Shield Generator: +50 Max Shields
	else if (EquipmentID == FName("skm_gadget_shieldgenerator"))
	{
		Health->ModifyMaxShieldBonus(50.f * Multiplier);
	}
}

void UEquipmentComponent::CycleEquipment(int32 Direction)
{
	if (Direction == 0) return;

	const int32 Step = Direction > 0 ? 1 : -1;
	int32 CandidateIndex = ActiveSlotIndex;

	for (int32 i = 0; i < NUM_EQUIPMENT_SLOTS; ++i)
	{
		CandidateIndex = (CandidateIndex + Step + NUM_EQUIPMENT_SLOTS) % NUM_EQUIPMENT_SLOTS;

		if (Slots.IsValidIndex(CandidateIndex) && Slots[CandidateIndex].IsSelectable())
		{
			SelectSlot(CandidateIndex);
			return;
		}
	}
}

void UEquipmentComponent::SelectSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || SlotIndex == ActiveSlotIndex) return;
	if (!Slots[SlotIndex].IsSelectable()) return;

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_SelectSlot(SlotIndex);
		return;
	}

	const int32 OldSlot = ActiveSlotIndex;
	ActiveSlotIndex = SlotIndex;

	UnequipSlot(OldSlot);
	EquipActiveSlot();

	OnRep_ActiveSlotIndex(OldSlot);
}

void UEquipmentComponent::Server_SelectSlot_Implementation(int32 SlotIndex)
{
	SelectSlot(SlotIndex);
}

void UEquipmentComponent::EquipActiveSlot()
{
	if (!Slots.IsValidIndex(ActiveSlotIndex)) return;

	AWeapon* NewWeapon = Slots[ActiveSlotIndex].SpawnedWeapon;
	if (NewWeapon)
	{
		NewWeapon->SetActorHiddenInGame(false);
		if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
		{
			NewWeapon->AttachToOwningPawn(OwningPawn);
		}
	}

	OnActiveSlotChanged.Broadcast(ActiveSlotIndex, NewWeapon);
}

void UEquipmentComponent::UnequipSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex)) return;

	AWeapon* OldWeapon = Slots[SlotIndex].SpawnedWeapon;
	if (OldWeapon)
	{
		OldWeapon->SetActorHiddenInGame(true);
	}
}

void UEquipmentComponent::OnRep_ActiveSlotIndex(int32 OldSlotIndex)
{
	UnequipSlot(OldSlotIndex);
	EquipActiveSlot();
}

AWeapon* UEquipmentComponent::GetActiveWeapon() const
{
	if (Slots.IsValidIndex(ActiveSlotIndex))
	{
		return Slots[ActiveSlotIndex].SpawnedWeapon;
	}
	return nullptr;
}

void UEquipmentComponent::DestroyInventory()
{
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index].bIsPassive)
		{
			ApplyPassiveEquipment(Slots[Index].EquipmentID, false);
		}
		else if (Slots[Index].SpawnedWeapon)
		{
			Slots[Index].SpawnedWeapon->Destroy();
			Slots[Index].SpawnedWeapon = nullptr;
		}
	}
	Slots.Empty();
}