// Copyright TemmieTim

#include "Ability/ResourceComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UResourceComponent::UResourceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// Baseline Default: Medi Beam (250 cap, 50/s drain, 40/s regen, 0.5s delay, 7.5s lockout)
	MaxResource = 250.f;
	CurrentResource = 250.f;
	DrainRatePerSecond = 50.f;
	RegenRatePerSecond = 40.f;
	RegenDelaySeconds = 0.5f;
	OverheatLockoutDuration = 7.5f;
	bUninterruptedRegen = false;

	bIsDraining = false;
	bIsOverheated = false;
	TimeSinceLastDrain = 0.f;
}

void UResourceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UResourceComponent, CurrentResource);
	DOREPLIFETIME(UResourceComponent, MaxResource);
	DOREPLIFETIME(UResourceComponent, bIsOverheated);
}

void UResourceComponent::ConfigureResource(float InMaxResource, float InDrainRate, float InRegenRate, float InRegenDelay, float InOverheatLockout, bool bInUninterruptedRegen)
{
	MaxResource = InMaxResource;
	CurrentResource = InMaxResource;
	DrainRatePerSecond = InDrainRate;
	RegenRatePerSecond = InRegenRate;
	RegenDelaySeconds = InRegenDelay;
	OverheatLockoutDuration = InOverheatLockout;
	bUninterruptedRegen = bInUninterruptedRegen;

	BroadcastResource();
}

void UResourceComponent::StartDraining()
{
	if (bIsOverheated) return;
	bIsDraining = true;
	TimeSinceLastDrain = 0.f;
}

void UResourceComponent::StopDraining()
{
	bIsDraining = false;
}

void UResourceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	bool bUpdated = false;

	// 1. Drain Resource
	if (bIsDraining && !bIsOverheated)
	{
		CurrentResource = FMath::Max(0.0f, CurrentResource - (DrainRatePerSecond * DeltaTime));
		TimeSinceLastDrain = 0.0f;
		bUpdated = true;

		// Overheat trigger when depleted
		if (CurrentResource <= 0.0f)
		{
			bIsOverheated = true;
			bIsDraining = false;
			OnRep_Overheated();

			// Lockout runs concurrently with cooling (7.5s)
			GetWorld()->GetTimerManager().SetTimer(LockoutTimerHandle, this, &ThisClass::EndOverheatLockout, OverheatLockoutDuration, false);
		}
	}
	else
	{
		TimeSinceLastDrain += DeltaTime;
	}

	// 2. Regenerate Resource
	const bool bCanRegen = bUninterruptedRegen || (TimeSinceLastDrain >= RegenDelaySeconds);
	if (bCanRegen && CurrentResource < MaxResource)
	{
		CurrentResource = FMath::Min(MaxResource, CurrentResource + (RegenRatePerSecond * DeltaTime));
		bUpdated = true;
	}

	if (bUpdated)
	{
		BroadcastResource();
	}
}

void UResourceComponent::EndOverheatLockout()
{
	// Lockout elapsed; beam/cannister is ready to use again
	bIsOverheated = false;
	OnRep_Overheated();
}

void UResourceComponent::BroadcastResource()
{
	const float Pct = MaxResource > 0.f ? (CurrentResource / MaxResource) : 0.f;
	OnResourceChanged.Broadcast(CurrentResource, MaxResource, Pct);
}

void UResourceComponent::OnRep_Resource()
{
	BroadcastResource();
}

void UResourceComponent::OnRep_Overheated()
{
	OnOverheatStateChanged.Broadcast(bIsOverheated);
}