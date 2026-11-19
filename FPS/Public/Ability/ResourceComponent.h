// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnResourceChanged, float, CurrentResource, float, MaxResource, float, Percent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverheatStateChanged, bool, bIsOverheated);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_API UResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Configures meter capacities and rates */
	void ConfigureResource(float InMaxResource, float InDrainRate, float InRegenRate, float InRegenDelay, float InOverheatLockout, bool bInUninterruptedRegen = false);

	UFUNCTION(BlueprintCallable, Category = "FPS|Resource")
	void StartDraining();

	UFUNCTION(BlueprintCallable, Category = "FPS|Resource")
	void StopDraining();

	UFUNCTION(BlueprintPure, Category = "FPS|Resource")
	bool CanUse() const { return !bIsOverheated && CurrentResource > 0.f; }

	UFUNCTION(BlueprintPure, Category = "FPS|Resource")
	float GetCurrentResource() const { return CurrentResource; }

	UFUNCTION(BlueprintPure, Category = "FPS|Resource")
	float GetMaxResource() const { return MaxResource; }

	UFUNCTION(BlueprintPure, Category = "FPS|Resource")
	bool IsOverheated() const { return bIsOverheated; }

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnResourceChanged OnResourceChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnOverheatStateChanged OnOverheatStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Resource)
	float CurrentResource;

	UPROPERTY(ReplicatedUsing = OnRep_Resource)
	float MaxResource;

	UPROPERTY(ReplicatedUsing = OnRep_Overheated)
	bool bIsOverheated;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Resource")
	float DrainRatePerSecond;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Resource")
	float RegenRatePerSecond;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Resource")
	float RegenDelaySeconds;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Resource")
	float OverheatLockoutDuration;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Resource")
	bool bUninterruptedRegen; // Croc Heavy's Cannister regens even while draining

private:
	UFUNCTION()
	void OnRep_Resource();

	UFUNCTION()
	void OnRep_Overheated();

	void BroadcastResource();
	void EndOverheatLockout();

	bool bIsDraining;
	float TimeSinceLastDrain;
	FTimerHandle LockoutTimerHandle;
};