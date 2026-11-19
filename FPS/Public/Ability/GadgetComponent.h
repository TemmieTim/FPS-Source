// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GadgetComponent.generated.h"

USTRUCT(BlueprintType)
struct FPS_API FGadgetSlotState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Gadget")
	int32 MaxCharges = 1;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Gadget")
	int32 CurrentCharges = 1;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Gadget")
	float CooldownSeconds = 10.f;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Gadget")
	float CooldownRemaining = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGadgetCooldownUpdated, int32, SlotIndex, int32, ChargesRemaining, float, CooldownRemaining);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_API UGadgetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGadgetComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Configures a gadget slot with charges and cooldown duration */
	void RegisterGadgetSlot(int32 SlotIndex, int32 MaxCharges, float CooldownSeconds);

	/** Activates gadget: consumes charge, starts cooldown, and AUTO-SWITCHES to Slot 1 */
	UFUNCTION(BlueprintCallable, Category = "FPS|Gadget")
	bool ActivateGadget(int32 SlotIndex);

	/** Query whether a slot has an available charge for selection/cycling */
	UFUNCTION(BlueprintPure, Category = "FPS|Gadget")
	bool IsSlotReady(int32 SlotIndex) const;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnGadgetCooldownUpdated OnGadgetCooldownUpdated;

protected:
	UPROPERTY(Replicated)
	TArray<FGadgetSlotState> SlotStates;

private:
	void SwitchToPrimarySlot();
	static constexpr int32 NUM_SLOTS = 4;
};