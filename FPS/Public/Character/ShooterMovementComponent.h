// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterMovementComponent.generated.h"

UCLASS()
class FPS_API UShooterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UShooterMovementComponent();

	/** Configure movement tuning for arena-shooter responsiveness */
	void ApplyHeroMovementSettings(float WalkSpeed, float SprintSpeed, float InJumpZVelocity, bool bCanDoubleJump = false);

	/** Sets sprint intent */
	void SetSprinting(bool bWantsToSprint);

	UFUNCTION(BlueprintPure, Category = "FPS|Movement")
	bool IsSprinting() const { return bIsSprinting; }

	float GetConfiguredWalkSpeed() const { return ConfiguredWalkSpeed; }
	float GetConfiguredSprintSpeed() const { return ConfiguredSprintSpeed; }

protected:
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual float GetMaxSpeed() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Movement")
	float ConfiguredWalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Movement")
	float ConfiguredSprintSpeed;

	UPROPERTY(Replicated)
	bool bIsSprinting;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};