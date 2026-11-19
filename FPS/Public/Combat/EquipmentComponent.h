// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/EquipmentTypes.h"
#include "EquipmentComponent.generated.h"

class AWeapon;
class UHeroDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveSlotChanged, int32, NewSlotIndex, AWeapon*, NewEquippedWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotUpdated, int32, SlotIndex);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initializes slots and passives from a Hero Data Asset */
	void InitializeLoadout(const UHeroDataAsset* HeroData);

	/** Cycles to next/previous selectable equipment (Direction = +1 or -1) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Equipment")
	void CycleEquipment(int32 Direction);

	/** Selects a specific slot (Index 0..3 for Slots 1..4) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Equipment")
	void SelectSlot(int32 SlotIndex);

	/** Validates a loadout against slot restrictions (Max 1 Secondary, Max 1 Melee) */
	static bool ValidateLoadout(const TArray<FHeroEquipmentSlotConfig>& ProposedLoadout, FString& OutErrorReason);

	UFUNCTION(BlueprintPure, Category = "FPS|Equipment")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "FPS|Equipment")
	AWeapon* GetActiveWeapon() const;

	UFUNCTION(BlueprintPure, Category = "FPS|Equipment")
	const TArray<FEquipmentSlot>& GetEquipmentSlots() const { return Slots; }

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnActiveSlotChanged OnActiveSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnEquipmentSlotUpdated OnEquipmentSlotUpdated;

	void DestroyInventory();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex;

	UPROPERTY(Replicated)
	TArray<FEquipmentSlot> Slots;

private:
	UFUNCTION()
	void OnRep_ActiveSlotIndex(int32 OldSlotIndex);

	void EquipActiveSlot();
	void UnequipSlot(int32 SlotIndex);

	void ApplyPassiveEquipment(const FName& EquipmentID, bool bApply);

	UFUNCTION(Server, Reliable)
	void Server_SelectSlot(int32 SlotIndex);

	static constexpr int32 NUM_EQUIPMENT_SLOTS = 4;
};