// Copyright TemmieTim

#include "Character/CharacterStateComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/ShooterCharacter.h"
#include "Engine/OverlapResult.h"

UCharacterStateComponent::UCharacterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	InvisibilityState = EInvisibilityState::None;
	bIsDetected = false;
	bHasSprintInvisibility = false;
}

void UCharacterStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCharacterStateComponent, InvisibilityState);
	DOREPLIFETIME(UCharacterStateComponent, bIsDetected);
	DOREPLIFETIME(UCharacterStateComponent, bHasSprintInvisibility);
}

void UCharacterStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Server-Authoritative 4-metre detection check
	if (GetOwner() && GetOwner()->HasAuthority() && InvisibilityState == EInvisibilityState::Invisible)
	{
		CheckProximityDetection();
	}
}

void UCharacterStateComponent::HandleSprintChanged(bool bIsSprinting)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!bHasSprintInvisibility) return;

	if (bIsSprinting)
	{
		// Cannot start casting if currently decloaking or on cooldown
		if (InvisibilityState == EInvisibilityState::None)
		{
			StartInvisCast();
		}
	}
	else
	{
		// Stopped sprinting
		if (InvisibilityState == EInvisibilityState::Casting || InvisibilityState == EInvisibilityState::Invisible)
		{
			BreakInvisibility();
		}
	}
}

void UCharacterStateComponent::RequestInterruptSprint(const FString& Reason)
{
	if (!GetOwner()) return;

	// Tell the character to stop sprinting
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetOwner()))
	{
		ShooterChar->RequestSprint(false);
	}

	// Break invisibility if active or casting
	if (GetOwner()->HasAuthority())
	{
		if (InvisibilityState == EInvisibilityState::Casting || InvisibilityState == EInvisibilityState::Invisible)
		{
			BreakInvisibility();
		}
	}
}

void UCharacterStateComponent::HandleDamageTaken()
{
	// Taking damage instantly breaks invisibility
	RequestInterruptSprint(TEXT("Took Damage"));
}

void UCharacterStateComponent::StartInvisCast()
{
	InvisibilityState = EInvisibilityState::Casting;
	bIsDetected = false;
	OnRep_InvisibilityState();

	GetWorld()->GetTimerManager().SetTimer(CastTimerHandle, this, &UCharacterStateComponent::CompleteInvisCast, INVIS_CAST_DURATION, false);
}

void UCharacterStateComponent::CompleteInvisCast()
{
	InvisibilityState = EInvisibilityState::Invisible;
	bIsDetected = false;
	OnRep_InvisibilityState();
}

void UCharacterStateComponent::BreakInvisibility()
{
	GetWorld()->GetTimerManager().ClearTimer(CastTimerHandle);

	InvisibilityState = EInvisibilityState::Decloaking;
	bIsDetected = false;
	OnRep_InvisibilityState();

	// 1.0s Decloaking duration
	GetWorld()->GetTimerManager().SetTimer(DecloakTimerHandle, this, &UCharacterStateComponent::FinishDecloak, INVIS_DECLOAK_DURATION, false);
}

void UCharacterStateComponent::FinishDecloak()
{
	InvisibilityState = EInvisibilityState::Cooldown;
	OnRep_InvisibilityState();

	// 4.0s Cooldown duration
	GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &UCharacterStateComponent::FinishCooldown, INVIS_COOLDOWN_DURATION, false);
}

void UCharacterStateComponent::FinishCooldown()
{
	InvisibilityState = EInvisibilityState::None;
	OnRep_InvisibilityState();
}

void UCharacterStateComponent::CheckProximityDetection()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	const FVector Origin = OwnerActor->GetActorLocation();
	FCollisionShape DetectionSphere = FCollisionShape::MakeSphere(ENEMY_DETECTION_RADIUS_UU);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FOverlapResult> Overlaps;
	const bool bHit = GetWorld()->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, DetectionSphere, QueryParams);

	bool bFoundEnemyNear = false;
	if (bHit)
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* OtherActor = Overlap.GetActor();
			// Ensure it is another character and not an ally
			if (OtherActor && OtherActor != OwnerActor)
			{
				bFoundEnemyNear = true;
				break;
			}
		}
	}

	if (bIsDetected != bFoundEnemyNear)
	{
		bIsDetected = bFoundEnemyNear;
		OnRep_IsDetected();
	}
}

void UCharacterStateComponent::OnRep_InvisibilityState()
{
	OnInvisibilityStateChanged.Broadcast(InvisibilityState);
}

void UCharacterStateComponent::OnRep_IsDetected()
{
	OnInvisibilityDetectedChanged.Broadcast(bIsDetected);
}