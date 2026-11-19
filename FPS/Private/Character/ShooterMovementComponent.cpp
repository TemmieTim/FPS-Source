// Copyright TemmieTim

#include "Character/ShooterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UShooterMovementComponent::UShooterMovementComponent()
{
	// Default Baseline (Soldier)
	ConfiguredWalkSpeed = 510.f;
	ConfiguredSprintSpeed = 660.f;
	MaxWalkSpeed = ConfiguredWalkSpeed;
	MaxWalkSpeedCrouched = 300.f;
	JumpZVelocity = 570.f;

	// Arena Shooter Responsiveness: Instant acceleration & high air control
	MaxAcceleration = 20000.f;      // Reaches target speed almost instantly (no ramp-up)
	BrakingDecelerationWalking = 15000.f; // Stops crisp and fast
	GroundFriction = 8.0f;
	BrakingFrictionFactor = 1.0f;
	bUseSeparateBrakingFriction = true;

	AirControl = 0.85f;             // High responsiveness while airborne
	AirControlBoostMultiplier = 1.5f;

	bIsSprinting = false;
	NavAgentProps.bCanCrouch = true;
}

void UShooterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UShooterMovementComponent, bIsSprinting, COND_SimulatedOnly);
}

void UShooterMovementComponent::ApplyHeroMovementSettings(float WalkSpeed, float SprintSpeed, float InJumpZVelocity, bool bCanDoubleJump)
{
	ConfiguredWalkSpeed = WalkSpeed;
	ConfiguredSprintSpeed = SprintSpeed;
	MaxWalkSpeed = ConfiguredWalkSpeed;
	JumpZVelocity = InJumpZVelocity;

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		// Bunny Scout passive double jump: built-in JumpMaxCount = 2
		OwnerChar->JumpMaxCount = bCanDoubleJump ? 2 : 1;
	}
}

void UShooterMovementComponent::SetSprinting(bool bWantsToSprint)
{
	bIsSprinting = bWantsToSprint;
}

void UShooterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	// Cannot sprint if moving backwards or standing still
	if (bIsSprinting)
	{
		const float ForwardSpeed = FVector::DotProduct(Velocity, UpdatedComponent->GetForwardVector());
		if (ForwardSpeed <= 50.f)
		{
			bIsSprinting = false;
		}
	}
}

float UShooterMovementComponent::GetMaxSpeed() const
{
	if (IsCrouching())
	{
		return MaxWalkSpeedCrouched;
	}

	if (bIsSprinting)
	{
		return ConfiguredSprintSpeed;
	}

	return ConfiguredWalkSpeed;
}