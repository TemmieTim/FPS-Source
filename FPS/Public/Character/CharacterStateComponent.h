// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterStateComponent.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Downed UMETA(DisplayName = "Downed"),
	BeingRevived UMETA(DisplayName = "Being Revived"),
	GettingUp UMETA(DisplayName = "Getting Up")
};

UENUM(BlueprintType)
enum class EInvisibilityState : uint8
{
	None UMETA(DisplayName = "None / Visible"),
	Casting UMETA(DisplayName = "Casting Invisibility (1.0s)"),
	Invisible UMETA(DisplayName = "Invisible"),
	Decloaking UMETA(DisplayName = "Decloaking (1.0s)"),
	Cooldown UMETA(DisplayName = "On Cooldown (4.0s)")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvisibilityStateChanged, EInvisibilityState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvisibilityDetectedChanged, bool, bIsDetectedByEnemy);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_API UCharacterStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterStateComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Centralized Action Interruption: Called whenever character shoots, aims, reloads, cycles, jumps, or crouches */
	UFUNCTION(BlueprintCallable, Category = "FPS|State")
	void RequestInterruptSprint(const FString& Reason);

	/** Enables/disables Fox Agent's sprint invisibility trait */
	void SetHasSprintInvisibilityPassive(bool bHasPassive) { bHasSprintInvisibility = bHasPassive; }

	/** Called by Movement when sprint starts or stops */
	void HandleSprintChanged(bool bIsSprinting);

	/** Called by HealthComponent when taking damage */
	void HandleDamageTaken();

	UFUNCTION(BlueprintPure, Category = "FPS|State")
	EInvisibilityState GetInvisibilityState() const { return InvisibilityState; }

	UFUNCTION(BlueprintPure, Category = "FPS|State")
	bool IsInvisible() const { return InvisibilityState == EInvisibilityState::Invisible; }

	UFUNCTION(BlueprintPure, Category = "FPS|State")
	bool IsDetectedByEnemy() const { return bIsDetected; }

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnInvisibilityStateChanged OnInvisibilityStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnInvisibilityDetectedChanged OnInvisibilityDetectedChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_InvisibilityState)
	EInvisibilityState InvisibilityState;

	UPROPERTY(ReplicatedUsing = OnRep_IsDetected)
	bool bIsDetected;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Stealth")
	bool bHasSprintInvisibility;

private:
	UFUNCTION()
	void OnRep_InvisibilityState();

	UFUNCTION()
	void OnRep_IsDetected();

	void StartInvisCast();
	void CompleteInvisCast();
	void BreakInvisibility();
	void FinishDecloak();
	void FinishCooldown();
	void CheckProximityDetection();

	FTimerHandle CastTimerHandle;
	FTimerHandle DecloakTimerHandle;
	FTimerHandle CooldownTimerHandle;

	static constexpr float INVIS_CAST_DURATION = 1.0f;
	static constexpr float INVIS_DECLOAK_DURATION = 1.0f;
	static constexpr float INVIS_COOLDOWN_DURATION = 4.0f;
	static constexpr float ENEMY_DETECTION_RADIUS_UU = 400.0f; // 4 metres
};