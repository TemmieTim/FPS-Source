// Copyright TemmieTim

#include "Character/ShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Game/ShooterGameModeBase.h"
#include "Combat/CombatComponent.h"
#include "Combat/HealthComponent.h"
#include "Combat/EquipmentComponent.h"
#include "Character/ShooterMovementComponent.h"
#include "Character/CharacterStateComponent.h"
#include "Data/HeroDataAsset.h"
#include "Data/WeaponData.h"
#include "Weapon/Weapon.h"
#include "Kismet/KismetMathLibrary.h"

AShooterCharacter::AShooterCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->MovementState.bCanCrouch = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// 1. Health Component (4 Defensive Pools, Passive Regen, Armor Mitigation, Downed State)
	Health = CreateDefaultSubobject<UHealthComponent>("Health");
	Health->SetIsReplicated(true);

	// 2. Character State Component (Action Validation, Interruption Engine, Fox Agent Stealth)
	CharacterState = CreateDefaultSubobject<UCharacterStateComponent>("CharacterState");
	CharacterState->SetIsReplicated(true);

	// 3. Equipment Component (4 Slots, Loadout Validation, Smart Cycling, Passive Stat Injection)
	Equipment = CreateDefaultSubobject<UEquipmentComponent>("Equipment");
	Equipment->SetIsReplicated(true);

	// 4. First-Person Camera (attached to Root at Eye Height)
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>("FirstPersonCamera");
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// 5. First-Person Arms Mesh
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh1P");
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetBoundsScale(3.0f);
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	// 6. Third-Person Body Mesh
	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;

	// 7. External Orbit Camera (for Downed crawling, Emotes, and Third-Person perspective)
	OrbitSpringArm = CreateDefaultSubobject<USpringArmComponent>("OrbitSpringArm");
	OrbitSpringArm->SetupAttachment(GetRootComponent());
	OrbitSpringArm->TargetArmLength = 250.f;
	OrbitSpringArm->SocketOffset = FVector(0.f, 0.f, 50.f);
	OrbitSpringArm->bUsePawnControlRotation = true;
	OrbitSpringArm->bDoCollisionTest = true;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>("ThirdPersonCamera");
	ThirdPersonCamera->SetupAttachment(OrbitSpringArm, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	ThirdPersonCamera->SetActive(false); // Default to First-Person

	// 8. Legacy Combat Component
	Combat = CreateDefaultSubobject<UCombatComponent>("Combat");
	Combat->SetIsReplicated(true);

	// 9. Status Effect Component (Non-additive slows, Vulnerability, Cannister damage reduction)
	StatusEffect = CreateDefaultSubobject<UStatusEffectComponent>("StatusEffect");
	StatusEffect->SetIsReplicated(true);

	// 10. Gadget Component (Charges, Cooldowns, Auto-Switch to Slot 1)
	Gadget = CreateDefaultSubobject<UGadgetComponent>("Gadget");
	Gadget->SetIsReplicated(true);

	DefaultFieldOfView = 90.0f;
	TurningStatus = ETurningInPlace::NotTurning;
	bWeaponFirstReplicated = false;
	bIsSprinting = false;
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Register with GameMode for Squad Wipe tracking (Authority only)
	if (HasAuthority())
	{
		if (AShooterGameModeBase* GM = Cast<AShooterGameModeBase>(GetWorld()->GetAuthGameMode()))
		{
			GM->RegisterSquadMember(this);
		}
	}

	// Initialize hero stats, movement, meshes, and starting loadout from Data Asset
	if (InitialHeroData)
	{
		InitializeFromHeroData(InitialHeroData);
	}

	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(DefaultFieldOfView);
	}

	if (Health)
	{
		Health->OnDownedStateChanged.AddDynamic(this, &ThisClass::HandleDownedStateChanged);
		Health->OnDamageReceived.AddDynamic(this, &ThisClass::HandleDamageReceived);
	}

	StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
}

void AShooterCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Safely clean up inventory during actor teardown instead of BeginDestroy (prevents GC crashes)
	if (IsValid(Equipment))
	{
		Equipment->DestroyInventory();
	}

	if (IsValid(Combat))
	{
		Combat->DestroyInventory();
	}

	Super::EndPlay(EndPlayReason);
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Sync legacy Combat->CurrentWeapon on BOTH Server and Clients
	if (Combat && Equipment)
	{
		Combat->CurrentWeapon = Equipment->GetActiveWeapon();
	}

	CalculateTurnInPlaceParameters(DeltaTime);
	CalculateFABRIKSocketTransform();
}

void AShooterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (InitialHeroData && HasAuthority())
	{
		InitializeFromHeroData(InitialHeroData);
	}

	// Sync legacy pointer so Udemy Blueprints don't crash on null
	if (Combat && Equipment)
	{
		Combat->CurrentWeapon = Equipment->GetActiveWeapon();
	}
}

void AShooterCharacter::UnPossessed()
{
	Super::UnPossessed();
}

void AShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (IsValid(Combat))
	{
		Combat->InitializeWeaponWidgets();
	}
}

/* =========================================================================
   Hero Data Asset Initialization (Phase 4)
   ========================================================================= */

void AShooterCharacter::InitializeFromHeroData(const UHeroDataAsset* HeroData)
{
	if (!HeroData) return;

	// 1. Health & Defensive Pools (Full reset to configured hero maximums)
	if (Health)
	{
		Health->SetPoolMaxValues(HeroData->BaseHealth, HeroData->BaseShields, HeroData->BaseArmor, true);
	}

	// 2. Responsive Movement & Bunny Scout Double Jump
	if (UShooterMovementComponent* SMC = GetShooterMovementComponent())
	{
		SMC->ApplyHeroMovementSettings(HeroData->WalkSpeed, HeroData->SprintSpeed, HeroData->JumpZVelocity, HeroData->bCanDoubleJump);
	}

	// 3. Fox Agent Sprint Invisibility Passive
	if (CharacterState)
	{
		CharacterState->SetHasSprintInvisibilityPassive(HeroData->bHasSprintInvisibility);
	}

	// 4. Character Presentation Meshes & Animation Classes
	if (HeroData->FirstPersonMesh && Mesh1P)
	{
		Mesh1P->SetSkeletalMesh(HeroData->FirstPersonMesh);
		if (HeroData->FirstPersonAnimClass)
		{
			Mesh1P->SetAnimInstanceClass(HeroData->FirstPersonAnimClass);
		}
	}

	if (HeroData->ThirdPersonMesh && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(HeroData->ThirdPersonMesh);
		if (HeroData->ThirdPersonAnimClass)
		{
			GetMesh()->SetAnimInstanceClass(HeroData->ThirdPersonAnimClass);
		}
	}

	// 5. 4-Slot Equipment Loadout & Passives (Server Authoritative)
	if (Equipment && HasAuthority())
	{
		Equipment->InitializeLoadout(HeroData);
	}
}

/* =========================================================================
   ICombatInterface Implementation (Shared by Player & AI)
   ========================================================================= */

void AShooterCharacter::GetAimViewPoint_Implementation(FVector& OutLocation, FRotator& OutRotation) const
{
	// 1. If locally controlled by a human player, use active camera
	if (IsLocallyControlled())
	{
		if (ThirdPersonCamera && ThirdPersonCamera->IsActive())
		{
			OutLocation = ThirdPersonCamera->GetComponentLocation();
			OutRotation = ThirdPersonCamera->GetComponentRotation();
			return;
		}

		if (FirstPersonCamera)
		{
			OutLocation = FirstPersonCamera->GetComponentLocation();
			OutRotation = FirstPersonCamera->GetComponentRotation();
			return;
		}
	}

	// 2. If controlled by AI or remote proxy, derive from Actor Eyes / Base Aim Rotation
	GetActorEyesViewPoint(OutLocation, OutRotation);
	OutRotation = GetBaseAimRotation();
}

void AShooterCharacter::RequestFire_Implementation(bool bPressed)
{
	if (IsDowned()) return;

	if (bPressed && CharacterState)
	{
		CharacterState->RequestInterruptSprint(TEXT("Fired Weapon"));
	}

	// Trigger active weapon through Equipment Component
	if (Equipment && Equipment->GetActiveWeapon())
	{
		if (bPressed)
		{
			Equipment->GetActiveWeapon()->StartFire();
		}
		else
		{
			Equipment->GetActiveWeapon()->StopFire();
		}
		return;
	}

	// Fallback to legacy Combat component
	if (Combat)
	{
		bPressed ? Combat->Initiate_FireWeapon_Pressed() : Combat->Initiate_FireWeapon_Released();
	}
}

void AShooterCharacter::RequestReload_Implementation()
{
	if (IsDowned()) return;

	if (CharacterState)
	{
		CharacterState->RequestInterruptSprint(TEXT("Reloaded"));
	}

	if (Equipment && Equipment->GetActiveWeapon())
	{
		Equipment->GetActiveWeapon()->StartReload();
		return;
	}

	if (Combat)
	{
		Combat->Initiate_ReloadWeapon();
	}
}

void AShooterCharacter::RequestAim_Implementation(bool bPressed)
{
	if (IsDowned() || !IsValid(Combat)) return;

	// Centralized Interruption: Aiming interrupts sprint (and reveals Fox Agent)
	if (bPressed && CharacterState)
	{
		CharacterState->RequestInterruptSprint(TEXT("Aimed Weapon"));
	}

	if (bPressed)
	{
		Combat->Initiate_Aim_Pressed();
		OnAim(true);
	}
	else
	{
		Combat->Initiate_Aim_Released();
		OnAim(false);
	}
}

void AShooterCharacter::RequestCycleWeapon_Implementation(int32 Direction)
{
	if (IsDowned()) return;

	// Centralized Interruption: Cycling interrupts sprint
	if (CharacterState)
	{
		CharacterState->RequestInterruptSprint(TEXT("Cycled Weapon"));
	}

	// Smart Cycling through 4 slots (skips passives and empty/cooldown slots)
	if (Equipment)
	{
		Equipment->CycleEquipment(Direction);
	}
	else if (Combat)
	{
		Combat->Initiate_CycleWeapon();
	}
}

void AShooterCharacter::RequestSelectSlot_Implementation(int32 SlotIndex)
{
	if (IsDowned()) return;

	// Centralized Interruption: Selecting slot interrupts sprint
	if (CharacterState)
	{
		CharacterState->RequestInterruptSprint(TEXT("Selected Slot"));
	}

	if (Equipment)
	{
		Equipment->SelectSlot(SlotIndex);
	}
}

/* =========================================================================
   Universal Locomotion Requests
   ========================================================================= */

void AShooterCharacter::RequestMove(const FVector2D& MoveInput)
{
	if (Controller == nullptr) return;

	if (IsDowned())
	{
		// Car-like crawling locomotion:
		// MoveInput.Y: Forward / Backward crawl
		// MoveInput.X: Steering curve (prevents crawl strafing)
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, MoveInput.Y);

		if (FMath::Abs(MoveInput.Y) > 0.1f)
		{
			AddControllerYawInput(MoveInput.X * 0.5f);
		}
		return;
	}

	// Normal Locomotion
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const float MoveMultiplier = StatusEffect ? StatusEffect->GetEffectiveMovementMultiplier() : 1.0f;

	AddMovementInput(ForwardDirection, MoveInput.Y * MoveMultiplier);
	AddMovementInput(RightDirection, MoveInput.X * MoveMultiplier);
}

void AShooterCharacter::RequestJump()
{
	if (IsDowned()) return; // Downed characters cannot jump

	// Centralized Interruption: Jumping interrupts sprint (and reveals Fox Agent)
	if (CharacterState)
	{
		CharacterState->RequestInterruptSprint(TEXT("Jumped"));
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Jump(); // Built-in JumpMaxCount = 2 handles Bunny Scout double jump cleanly!
	}
}

void AShooterCharacter::RequestStopJumping()
{
	StopJumping();
}

void AShooterCharacter::RequestToggleCrouch()
{
	if (IsDowned()) return;

	// Centralized Interruption: Crouching interrupts sprint
	if (CharacterState)
	{
		CharacterState->RequestInterruptSprint(TEXT("Crouched"));
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AShooterCharacter::RequestSprint(bool bEnable)
{
	if (IsDowned()) return;

	bIsSprinting = bEnable;

	if (UShooterMovementComponent* SMC = GetShooterMovementComponent())
	{
		SMC->SetSprinting(bEnable);
	}

	if (CharacterState)
	{
		CharacterState->HandleSprintChanged(bEnable);
	}
}

/* =========================================================================
   State & Event Handlers
   ========================================================================= */

void AShooterCharacter::HandleDownedStateChanged(bool bNewIsDowned)
{
	if (bNewIsDowned)
	{
		// 1. Switch to Orbit Camera so the player can observe their crawling body
		SetUseThirdPersonCamera(true);

		// 2. Downed Locomotion (Crawling speed, no jumping or crouching)
		GetCharacterMovement()->MaxWalkSpeed = 120.f; // Crawl speed
		GetCharacterMovement()->bOrientRotationToMovement = true; // Car-like crawling orientation
		GetCharacterMovement()->bUseControllerDesiredRotation = false;

		// 3. Prevent weapons from firing while downed
		if (IsValid(Combat))
		{
			Combat->Initiate_FireWeapon_Released();
		}
	}
	else
	{
		// Revived: Switch back to First-Person Camera
		SetUseThirdPersonCamera(false);

		// Restore normal locomotion
		const float WalkSpeed = GetShooterMovementComponent() ? GetShooterMovementComponent()->GetConfiguredWalkSpeed() : 510.f;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
	}
}

void AShooterCharacter::HandleDamageReceived(const FDamageEventData& DamageData)
{
	if (CharacterState)
	{
		CharacterState->HandleDamageTaken();
	}
}

UShooterMovementComponent* AShooterCharacter::GetShooterMovementComponent() const
{
	return Cast<UShooterMovementComponent>(GetCharacterMovement());
}

void AShooterCharacter::SetUseThirdPersonCamera(bool bUseThirdPerson)
{
	if (!FirstPersonCamera || !ThirdPersonCamera) return;

	if (bUseThirdPerson)
	{
		FirstPersonCamera->SetActive(false);
		ThirdPersonCamera->SetActive(true);

		// Show third-person body, hide first-person arms
		GetMesh()->SetOwnerNoSee(false);
		Mesh1P->SetOwnerNoSee(true);
	}
	else
	{
		ThirdPersonCamera->SetActive(false);
		FirstPersonCamera->SetActive(true);

		// Hide third-person body, show first-person arms
		GetMesh()->SetOwnerNoSee(true);
		Mesh1P->SetOwnerNoSee(false);
	}
}

/* =========================================================================
   Existing Calculations & PlayerInterface
   ========================================================================= */

FRotator AShooterCharacter::GetFixedAimRotation() const
{
	FRotator AimRotation = GetBaseAimRotation();
	if (AimRotation.Pitch > 90.0f && !IsLocallyControlled())
	{
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AimRotation.Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AimRotation.Pitch);
	}
	return AimRotation;
}

bool AShooterCharacter::HasCurrentWeapon() const
{
	if (Equipment && Equipment->GetActiveWeapon())
	{
		return true;
	}
	return IsValid(Combat) && Combat->CurrentWeapon != nullptr;
}

void AShooterCharacter::CalculateTurnInPlaceParameters(float DeltaTime)
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir)
	{
		const FRotator CurrentAimRotation(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;

		if (TurningStatus == ETurningInPlace::NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsInAir)
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;

		const FRotator AimRotation = GetBaseAimRotation();
		const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
		TurningStatus = ETurningInPlace::NotTurning;
	}

	AO_Yaw *= -1.f;
}

void AShooterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningStatus = ETurningInPlace::Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningStatus = ETurningInPlace::Left;
	}

	if (TurningStatus != ETurningInPlace::NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.0f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 5.f)
		{
			TurningStatus = ETurningInPlace::NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AShooterCharacter::CalculateFABRIKSocketTransform()
{
	AWeapon* ActiveWeapon = Equipment ? Equipment->GetActiveWeapon() : (Combat ? Combat->CurrentWeapon.Get() : nullptr);
	if (IsValid(ActiveWeapon) && IsValid(ActiveWeapon->GetMesh3P()))
	{
		FABRIK_SocketTransform = ActiveWeapon->GetMesh3P()->GetSocketTransform("FABRIK_Socket", RTS_World);

		FVector Outlocation;
		FRotator OutRotation;
		GetMesh()->TransformToBoneSpace(
			"hand_r",
			FABRIK_SocketTransform.GetLocation(),
			FABRIK_SocketTransform.GetRotation().Rotator(),
			Outlocation,
			OutRotation);
		FABRIK_SocketTransform.SetLocation(Outlocation);
		FABRIK_SocketTransform.SetRotation(OutRotation.Quaternion());
	}
}

FName AShooterCharacter::GetWeaponAttachPoint_Implementation(const FGameplayTag& WeaponType) const
{
	checkf(Combat->WeaponData, TEXT("No Weapon Data Asset - Please fill out BP_ShooterCharacter"));
	if (const FName* AttachPoint = Combat->WeaponData->GripPoints.Find(WeaponType))
	{
		return *AttachPoint;
	}
	return NAME_None;
}

USkeletalMeshComponent* AShooterCharacter::GetMesh1P_Implementation() const
{
	return Mesh1P;
}

USkeletalMeshComponent* AShooterCharacter::GetMesh3P_Implementation() const
{
	return GetMesh();
}

void AShooterCharacter::WeaponReplicated_Implementation()
{
	if (!bWeaponFirstReplicated)
	{
		bWeaponFirstReplicated = true;
		AWeapon* ActiveWeapon = Equipment ? Equipment->GetActiveWeapon() : (Combat ? Combat->CurrentWeapon.Get() : nullptr);

		if (Combat && ActiveWeapon)
		{
			Combat->CurrentWeapon = ActiveWeapon;
		}

		OnWeaponFirstReplicated.Broadcast(ActiveWeapon, Combat ? Combat->bHitPlayer : false);
	}
}

AWeapon* AShooterCharacter::GetCurrentWeapon_Implementation()
{
	if (Equipment && Equipment->GetActiveWeapon())
	{
		return Equipment->GetActiveWeapon();
	}
	return Combat ? Combat->CurrentWeapon.Get() : nullptr;
}

int32 AShooterCharacter::GetReserveAmmo_Implementation() const
{
	return Combat ? Combat->CurrentReserveAmmo : 0;
}

void AShooterCharacter::Notify_CycleWeapon_Implementation()
{
	if (Equipment)
	{
		Equipment->CycleEquipment(1);
	}
	else if (Combat)
	{
		Combat->Notify_CycleWeapon();
	}
}

void AShooterCharacter::Notify_ReloadWeapon_Implementation()
{
	if (Combat)
	{
		Combat->Notify_ReloadWeapon();
	}
}