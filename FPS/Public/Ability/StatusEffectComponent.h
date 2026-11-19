// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusEffectComponent.generated.h"

USTRUCT(BlueprintType)
struct FPS_API FSlowInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Status")
	FName SourceID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Status")
	float SlowPercentage = 0.f; // e.g., 0.40f for 40% slow

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Status")
	float DurationRemaining = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlowStateChanged, float, EffectiveSlowMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageModifierChanged, float, IncomingDamageMultiplier);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusEffectComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Applies a slow effect. Slows do NOT stack additively; the strongest active slow takes precedence */
	UFUNCTION(BlueprintCallable, Category = "FPS|Status")
	void ApplySlowEffect(FName SourceID, float SlowPercentage, float DurationSeconds);

	/** Applies a timed damage vulnerability modifier (e.g. Throwing Knife +50% for 6s) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Status")
	void ApplyVulnerability(float VulnerabilityBonus, float DurationSeconds);

	/** Sets active damage reduction (e.g. Healing Cannister 40% reduction while in use) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Status")
	void SetDamageReduction(float ReductionPercentage, bool bActive);

	/** Returns multiplier for incoming damage (1.0 = normal, 1.5 = vulnerable, 0.6 = 40% reduction) */
	UFUNCTION(BlueprintPure, Category = "FPS|Status")
	float GetIncomingDamageMultiplier() const;

	/** Returns movement speed multiplier (1.0 = normal, 0.6 = 40% slow) */
	UFUNCTION(BlueprintPure, Category = "FPS|Status")
	float GetEffectiveMovementMultiplier() const { return EffectiveMovementMultiplier; }

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnSlowStateChanged OnSlowStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnDamageModifierChanged OnDamageModifierChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_StatusValues)
	float EffectiveMovementMultiplier;

	UPROPERTY(ReplicatedUsing = OnRep_StatusValues)
	float ActiveVulnerabilityMultiplier;

	UPROPERTY(ReplicatedUsing = OnRep_StatusValues)
	float ActiveDamageReduction;

private:
	UFUNCTION()
	void OnRep_StatusValues();

	void UpdateActiveSlows(float DeltaTime);
	void UpdateVulnerability(float DeltaTime);

	TArray<FSlowInstance> ActiveSlows;
	float VulnerabilityDurationRemaining;
};