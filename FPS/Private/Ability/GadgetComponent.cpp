// Copyright TemmieTim

#include "Ability/GadgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/ShooterCharacter.h"
#include "Combat/EquipmentComponent.h"

UGadgetComponent::UGadgetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	SlotStates.SetNum(NUM_SLOTS);
}

void UGadgetComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGadgetComponent, SlotStates);
}

void UGadgetComponent::RegisterGadgetSlot(int32 SlotIndex, int32 MaxCharges, float CooldownSeconds)
{
	if (!SlotStates.IsValidIndex(SlotIndex)) return;

	SlotStates[SlotIndex].MaxCharges = MaxCharges;
	SlotStates[SlotIndex].CurrentCharges = MaxCharges;
	SlotStates[SlotIndex].CooldownSeconds = CooldownSeconds;
	SlotStates[SlotIndex].CooldownRemaining = 0.f;
}

bool UGadgetComponent::IsSlotReady(int32 SlotIndex) const
{
	if (!SlotStates.IsValidIndex(SlotIndex)) return true; // Default ready if unmanaged
	return SlotStates[SlotIndex].CurrentCharges > 0;
}

bool UGadgetComponent::ActivateGadget(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!SlotStates.IsValidIndex(SlotIndex)) return false;

	FGadgetSlotState& State = SlotStates[SlotIndex];
	if (State.CurrentCharges <= 0) return false;

	// Consume charge and initiate cooldown
	State.CurrentCharges--;
	State.CooldownRemaining = State.CooldownSeconds;

	OnGadgetCooldownUpdated.Broadcast(SlotIndex, State.CurrentCharges, State.CooldownRemaining);

	// AUTO-SWITCH RULE: Switch back to Slot 1 (Primary Weapon) immediately after activating a gadget
	SwitchToPrimarySlot();

	return true;
}

void UGadgetComponent::SwitchToPrimarySlot()
{
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetOwner()))
	{
		if (UEquipmentComponent* EquipComp = ShooterChar->GetEquipmentComponent())
		{
			EquipComp->SelectSlot(0); // Slot 1 (Primary)
		}
	}
}

void UGadgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (int32 Index = 0; Index < SlotStates.Num(); ++Index)
	{
		FGadgetSlotState& State = SlotStates[Index];

		if (State.CooldownRemaining > 0.0f)
		{
			State.CooldownRemaining = FMath::Max(0.0f, State.CooldownRemaining - DeltaTime);

			if (State.CooldownRemaining <= 0.0f)
			{
				// Charge recovered
				State.CurrentCharges = FMath::Min(State.MaxCharges, State.CurrentCharges + 1);

				// If still missing charges, loop the cooldown
				if (State.CurrentCharges < State.MaxCharges)
				{
					State.CooldownRemaining = State.CooldownSeconds;
				}
			}

			OnGadgetCooldownUpdated.Broadcast(Index, State.CurrentCharges, State.CooldownRemaining);
		}
	}
}