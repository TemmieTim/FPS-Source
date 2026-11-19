// Copyright TemmieTim

#include "AI/ShooterAIController.h"
#include "BrainComponent.h"
#include "Interfaces/CombatInterface.h"

AShooterAIController::AShooterAIController()
{
	bWantsPlayerState = true;
	bSetControlRotationFromPawnOrientation = false;
}

void AShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (DefaultBehaviorTree)
	{
		RunBehaviorTree(DefaultBehaviorTree);
	}
}

void AShooterAIController::OnUnPossess()
{
	Super::OnUnPossess();

	// Clear active orders when releasing a pawn for Solo Character Switching
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Pawn unpossessed during character switch"));
	}
}

void AShooterAIController::CommandFire(bool bPressed)
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestFire(GetPawn(), bPressed);
	}
}

void AShooterAIController::CommandAim(bool bPressed)
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestAim(GetPawn(), bPressed);
	}
}

void AShooterAIController::CommandReload()
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestReload(GetPawn());
	}
}

void AShooterAIController::CommandSelectSlot(int32 SlotIndex)
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestSelectSlot(GetPawn(), SlotIndex);
	}
}