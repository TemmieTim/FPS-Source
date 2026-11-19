// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/DamageTypes.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_EightParams(FOnHealthPoolsChanged, float, CurrentHealth, float, MaxHealth, float, CurrentShields, float, MaxShields, float, CurrentArmor, float, MaxArmor, float, CurrentOverhealth, float, MaxOverhealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDownedStateChanged, bool, bIsDowned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageReceived, const FDamageEventData&, DamageData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealingReceived, const FHealingEventData&, HealingData);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Centralized Server-Authoritative Damage Entry Point */
	UFUNCTION(BlueprintCallable, Category = "FPS|Health")
	void ApplyDamageEvent(UPARAM(ref) FDamageEventData& DamageEvent);

	/** Centralized Server-Authoritative Healing Entry Point */
	UFUNCTION(BlueprintCallable, Category = "FPS|Health")
	void ApplyHealingEvent(UPARAM(ref) FHealingEventData& HealingEvent);

	/** Applies temporary Overhealth with a specific decay rate */
	UFUNCTION(BlueprintCallable, Category = "FPS|Health")
	void AddOverhealth(float Amount, float DecayPerSecond);

	/** Initializes or modifies maximum pool limits (e.g., from Hero Data or Passive Plates) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Health")
	void SetPoolMaxValues(float InMaxHealth, float InMaxShields, float InMaxArmor, bool bResetToMax = false);

	/** Passive Equipment Stat Modifiers */
	void ModifyMaxArmorBonus(float ArmorDelta);
	void ModifyMaxShieldBonus(float ShieldDelta);

	/** Revival Interaction */
	bool CanBeRevived() const { return bIsDowned && !bIsBeingRevived; }
	bool ReserveRevival(AActor* Reviver);
	void CancelRevival(AActor* Reviver);
	void CompleteRevival();

	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	bool IsDowned() const { return bIsDowned; }

	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	bool IsInvulnerable() const { return bIsInvulnerable; }

	// Getters for Health Pools
	float GetHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetShields() const { return CurrentShields; }
	float GetMaxShields() const { return MaxShields; }
	float GetArmor() const { return CurrentArmor; }
	float GetMaxArmor() const { return MaxArmor; }
	float GetOverhealth() const { return CurrentOverhealth; }

	// Delegates for UI & Game State
	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnHealthPoolsChanged OnHealthPoolsChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnDownedStateChanged OnDownedStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnDamageReceived OnDamageReceived;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnHealingReceived OnHealingReceived;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float CurrentHealth;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float CurrentShields;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float MaxShields;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float CurrentArmor;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float MaxArmor;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float CurrentOverhealth;

	UPROPERTY(ReplicatedUsing = OnRep_HealthPools)
	float MaxOverhealth;

	UPROPERTY(ReplicatedUsing = OnRep_DownedState)
	bool bIsDowned;

	UPROPERTY(Replicated)
	bool bIsInvulnerable;

	UPROPERTY(Replicated)
	bool bIsBeingRevived;

	UPROPERTY(Replicated)
	TWeakObjectPtr<AActor> CurrentReviver;

private:
	UFUNCTION()
	void OnRep_HealthPools();

	UFUNCTION()
	void OnRep_DownedState();

	void BroadcastPools();
	void ProcessPassiveRegeneration(float DeltaTime);
	void ProcessOverhealthDecay(float DeltaTime);
	void EnterDownedState();
	void EndPostReviveInvulnerability();

	// Passive Regeneration Tracking
	float DamageRegenTimer;
	float OverhealthDecayRate;
	FTimerHandle PostReviveInvulnTimerHandle;

	static constexpr float REGEN_DAMAGE_DELAY = 3.0f;
	static constexpr float SHIELD_REGEN_RATE = 30.0f;
	static constexpr float POST_REVIVE_INVULN_DURATION = 1.5f;
};