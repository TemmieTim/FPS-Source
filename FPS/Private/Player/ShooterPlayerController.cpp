// Copyright TemmieTim

#include "Player/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/ShooterCharacter.h"
#include "AI/ShooterAIController.h"
#include "Interfaces/CombatInterface.h"

AShooterPlayerController::AShooterPlayerController()
{
	bReplicates = true;
}

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (ShooterIMC)
		{
			Subsystem->AddMappingContext(ShooterIMC, 0);
		}
	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent);

	// Movement & Locomotion
	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Input_Jump_Started);
	EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::Input_Jump_Completed);
	EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::Input_Crouch);
	EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::Input_Sprint_Started);
	EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::Input_Sprint_Completed);

	// Combat & Equipment
	EIC->BindAction(FireWeaponAction, ETriggerEvent::Started, this, &ThisClass::Input_Fire_Started);
	EIC->BindAction(FireWeaponAction, ETriggerEvent::Completed, this, &ThisClass::Input_Fire_Completed);
	EIC->BindAction(AimWeaponAction, ETriggerEvent::Started, this, &ThisClass::Input_Aim_Started);
	EIC->BindAction(AimWeaponAction, ETriggerEvent::Completed, this, &ThisClass::Input_Aim_Completed);
	EIC->BindAction(ReloadWeaponAction, ETriggerEvent::Started, this, &ThisClass::Input_Reload);
	EIC->BindAction(CycleWeaponAction, ETriggerEvent::Triggered, this, &ThisClass::Input_CycleWeapon);
}

void AShooterPlayerController::Input_Move(const FInputActionValue& Value)
{
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn()))
	{
		ShooterChar->RequestMove(Value.Get<FVector2D>());
	}
}

void AShooterPlayerController::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddYawInput(LookAxis.X);
	AddPitchInput(LookAxis.Y);
}

void AShooterPlayerController::Input_Jump_Started()
{
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn()))
	{
		ShooterChar->RequestJump();
	}
}

void AShooterPlayerController::Input_Jump_Completed()
{
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn()))
	{
		ShooterChar->RequestStopJumping();
	}
}

void AShooterPlayerController::Input_Crouch()
{
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn()))
	{
		ShooterChar->RequestToggleCrouch();
	}
}

void AShooterPlayerController::Input_Sprint_Started()
{
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn()))
	{
		ShooterChar->RequestSprint(true);
	}
}

void AShooterPlayerController::Input_Sprint_Completed()
{
	if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn()))
	{
		ShooterChar->RequestSprint(false);
	}
}

void AShooterPlayerController::Input_Fire_Started()
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestFire(GetPawn(), true);
	}
}

void AShooterPlayerController::Input_Fire_Completed()
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestFire(GetPawn(), false);
	}
}

void AShooterPlayerController::Input_Aim_Started()
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestAim(GetPawn(), true);
	}
}

void AShooterPlayerController::Input_Aim_Completed()
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestAim(GetPawn(), false);
	}
}

void AShooterPlayerController::Input_Reload()
{
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestReload(GetPawn());
	}
}

void AShooterPlayerController::Input_CycleWeapon(const FInputActionValue& Value)
{
	const float Direction = Value.Get<float>();
	if (GetPawn() && GetPawn()->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_RequestCycleWeapon(GetPawn(), Direction > 0.f ? 1 : -1);
	}
}

bool AShooterPlayerController::SwitchToSquadCharacter(AShooterCharacter* TargetCharacter)
{
	// 1. Rule: Character switching is strictly forbidden in online Co-op
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("Character switching is disabled in Co-op mode."));
		return false;
	}

	AShooterCharacter* CurrentPawn = Cast<AShooterCharacter>(GetPawn());
	if (!CurrentPawn || !TargetCharacter || CurrentPawn == TargetCharacter)
	{
		return false;
	}

	AShooterAIController* TargetAI = Cast<AShooterAIController>(TargetCharacter->GetController());
	if (!TargetAI)
	{
		return false;
	}

	// 2. Clear old character input/sprint
	CurrentPawn->RequestSprint(false);

	// 3. Step 1: AI releases Target Character
	TargetAI->UnPossess();

	// 4. Step 2: Human releases Current Character
	UnPossess();

	// 5. Step 3: Human possesses Target Character
	Possess(TargetCharacter);

	// 6. Step 4: AI possesses Previous Character
	TargetAI->Possess(CurrentPawn);

	// 7. Configure Camera for newly controlled Pawn
	if (TargetCharacter->IsDowned())
	{
		// Downed character uses Orbit Camera
		TargetCharacter->SetUseThirdPersonCamera(true);
	}
	else
	{
		// Living character returns to First-Person Camera
		TargetCharacter->SetUseThirdPersonCamera(false);
	}

	// 8. Re-apply Enhanced Input Mapping Context to new pawn
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (ShooterIMC)
		{
			Subsystem->RemoveMappingContext(ShooterIMC);
			Subsystem->AddMappingContext(ShooterIMC, 0);
		}
	}

	return true;
}