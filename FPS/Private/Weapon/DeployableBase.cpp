// Copyright TemmieTim

#include "Weapon/DeployableBase.h"
#include "Combat/HealthComponent.h"
#include "TimerManager.h"

ADeployableBase::ADeployableBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Health = CreateDefaultSubobject<UHealthComponent>("Health");
	Health->SetIsReplicated(true);

	MaxLifetimeSeconds = 7.0f; // e.g. Dome Shield 7 seconds
	bHasLifetime = true;
}

void ADeployableBase::BeginPlay()
{
	Super::BeginPlay();

	if (Health)
	{
		Health->OnDownedStateChanged.AddDynamic(this, &ThisClass::HandleDowned);
	}

	if (HasAuthority() && bHasLifetime)
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &AActor::K2_DestroyActor, MaxLifetimeSeconds, false);
	}
}

void ADeployableBase::HandleDowned(bool bIsDowned)
{
	if (bIsDowned && HasAuthority())
	{
		Destroy();
	}
}