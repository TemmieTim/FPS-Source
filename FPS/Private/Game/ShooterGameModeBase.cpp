// Copyright TemmieTim

#include "Game/ShooterGameModeBase.h"
#include "Character/ShooterCharacter.h"
#include "Combat/HealthComponent.h"
#include "Kismet/GameplayStatics.h"

AShooterGameModeBase::AShooterGameModeBase()
{
	bIsSoloMode = true; // Set to false in Online Co-op game modes
}

void AShooterGameModeBase::RegisterSquadMember(AShooterCharacter* SquadMember)
{
	if (!SquadMember || ActiveSquad.Contains(SquadMember)) return;

	ActiveSquad.Add(SquadMember);

	if (UHealthComponent* Health = SquadMember->GetHealthComponent())
	{
		Health->OnDownedStateChanged.AddDynamic(this, &ThisClass::HandleSquadMemberDownedStateChanged);
	}
}

void AShooterGameModeBase::HandleSquadMemberDownedStateChanged(bool bIsDowned)
{
	CheckSquadStatus();
}

void AShooterGameModeBase::CheckSquadStatus()
{
	if (ActiveSquad.Num() == 0) return;

	bool bAllDowned = true;

	for (AShooterCharacter* Member : ActiveSquad)
	{
		if (Member && !Member->IsDowned())
		{
			bAllDowned = false;
			break;
		}
	}

	if (bAllDowned)
	{
		HandleSquadWipe();
	}
}

void AShooterGameModeBase::HandleSquadWipe()
{
	UE_LOG(LogTemp, Warning, TEXT("[GAME OVER] All squad members are downed. Squad wipe triggered."));
	OnSquadWiped.Broadcast();

	// Default behavior: Restart level / reload checkpoint
	// In Arena mode, this can navigate back to the lobby
}