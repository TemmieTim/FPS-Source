// Copyright TemmieTim

#include "Weapon/Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/PlayerInterface.h"
#include "Interfaces/CombatInterface.h"
#include "Combat/HealthComponent.h"
#include "Weapon/ProjectileBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "FPS/FPS.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh1P");
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetHiddenInGame(true);

	Mesh3P = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh3P");
	Mesh3P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh3P->bReceivesDecals = false;
	Mesh3P->CastShadow = true;
	Mesh3P->SetupAttachment(Mesh1P);
	Mesh3P->SetHiddenInGame(true);

	// Baseline Defaults (Howler 15 Assault Rifle)
	WeaponID = FName("skm_ar_howler15");
	FireMode = EFireMode::Automatic;
	DeliveryMethod = EDamageDeliveryMethod::Hitscan;
	FireIntervalSeconds = 0.10f;
	MagCapacity = 30;
	Ammo = 30;
	ReloadDurationSeconds = 2.5f;
	bIsReloading = false;

	PelletCount = 1;
	bCanHeadshot = true;
	CriticalMultiplier = 1.5f;
	bIsArmorPiercing = false;

	ProjectileLaunchSpeed = 3500.f;
	ProjectileGravityScale = 1.0f;
	AimFieldOfView = 70.f;

	CurrentSpread = 0.5f;
	bIsAiming = false;
	bWantsToFire = false;
	BurstShotsRemaining = 0;

	// Legacy Bridges
	FireType = EFireType::Auto;
	FireTime = 0.10f;
	StartingCarriedAmmo = 90;
	WeaponStatus = EWeaponStatus::Idle;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	Ammo = MagCapacity;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, Ammo);
	DOREPLIFETIME(AWeapon, bIsReloading);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Recover spread when not firing
	if (!bWantsToFire && CurrentSpread > SpreadConfig.BaseSpreadDegrees)
	{
		CurrentSpread = FMath::FInterpTo(CurrentSpread, SpreadConfig.BaseSpreadDegrees, DeltaTime, SpreadConfig.SpreadRecoveryRate);
	}
}

void AWeapon::StartFire()
{
	if (bIsReloading)
	{
		CancelReload(); // Firing interrupts reload
	}

	bWantsToFire = true;

	if (Ammo <= 0)
	{
		StartReload(); // Automatic reload when empty
		return;
	}

	if (!GetWorldTimerManager().IsTimerActive(FireTimerHandle))
	{
		ExecuteFireCycle();
	}
}

void AWeapon::StopFire()
{
	bWantsToFire = false;
}

void AWeapon::ExecuteFireCycle()
{
	if (Ammo <= 0)
	{
		StartReload();
		return;
	}

	// Consume ammunition
	Ammo--;

	// Fire according to delivery method
	if (DeliveryMethod == EDamageDeliveryMethod::Hitscan)
	{
		FireHitscanRound();
	}
	else if (DeliveryMethod == EDamageDeliveryMethod::Projectile)
	{
		FireProjectileRound();
	}

	// Increase spread per shot
	CurrentSpread = FMath::Min(SpreadConfig.MaxSpreadDegrees, CurrentSpread + SpreadConfig.SpreadGrowthPerShot);

	// Handle Fire Modes
	if (FireMode == EFireMode::Burst)
	{
		BurstShotsRemaining = 2; // 3-round burst (1 fired, 2 remaining)
		GetWorldTimerManager().SetTimer(BurstTimerHandle, [this]()
			{
				if (Ammo > 0 && BurstShotsRemaining > 0)
				{
					Ammo--;
					BurstShotsRemaining--;
					DeliveryMethod == EDamageDeliveryMethod::Hitscan ? FireHitscanRound() : FireProjectileRound();
				}
			}, 0.08f, true);
	}
	else if (FireMode == EFireMode::Automatic)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, [this]()
			{
				if (bWantsToFire && Ammo > 0)
				{
					ExecuteFireCycle();
				}
			}, FireIntervalSeconds, false);
	}
}

void AWeapon::FireHitscanRound()
{
	APawn* InstigatorPawn = GetInstigator();
	if (!InstigatorPawn) return;

	FVector AimLoc;
	FRotator AimRot;
	if (InstigatorPawn->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_GetAimViewPoint(InstigatorPawn, AimLoc, AimRot);
	}
	else
	{
		InstigatorPawn->GetActorEyesViewPoint(AimLoc, AimRot);
	}

	const int32 EventID = FMath::Rand();
	TArray<FHitResult> PelletHits;

	// Loop through pellets (1 for rifles/pistols, 9-11 for shotguns)
	for (int32 Pellet = 0; Pellet < PelletCount; ++Pellet)
	{
		const float SpreadHalfAngleRad = FMath::DegreesToRadians(CurrentSpread);
		const FVector FireDir = FMath::VRandCone(AimRot.Vector(), SpreadHalfAngleRad);
		const FVector TraceEnd = AimLoc + (FireDir * 10000.f);

		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(InstigatorPawn);

		GetWorld()->LineTraceSingleByChannel(Hit, AimLoc, TraceEnd, FPSTraceChannels::ECC_Weapon, QueryParams);
		PelletHits.Add(Hit);

		// Cosmetic visual feedback locally
		PlayFireEffects(Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd, InstigatorPawn->IsLocallyControlled());
	}

	Server_FireHitscan(PelletHits, EventID);
}

void AWeapon::FireProjectileRound()
{
	APawn* InstigatorPawn = GetInstigator();
	if (!InstigatorPawn || !ProjectileClass) return;

	FVector AimLoc;
	FRotator AimRot;
	if (InstigatorPawn->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_GetAimViewPoint(InstigatorPawn, AimLoc, AimRot);
	}
	else
	{
		InstigatorPawn->GetActorEyesViewPoint(AimLoc, AimRot);
	}

	const int32 EventID = FMath::Rand();
	PlayFireEffects(AimLoc + (AimRot.Vector() * 500.f), InstigatorPawn->IsLocallyControlled());

	Server_FireProjectile(AimLoc, AimRot, EventID);
}

void AWeapon::Server_FireHitscan_Implementation(const TArray<FHitResult>& Hits, int32 EventID)
{
	APawn* InstigatorPawn = GetInstigator();

	for (const FHitResult& Hit : Hits)
	{
		const FVector FXPoint = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
		Multicast_PlayFireFX(FXPoint); // Optimized Unreliable Multicast

		if (!Hit.bBlockingHit || !Hit.GetActor()) continue;

		if (UHealthComponent* TargetHealth = Hit.GetActor()->FindComponentByClass<UHealthComponent>())
		{
			FDamageEventData DamageData;
			DamageData.Instigator = InstigatorPawn;
			DamageData.DamageCauser = this;
			DamageData.Target = Hit.GetActor();
			DamageData.DeliveryMethod = EDamageDeliveryMethod::Hitscan;
			DamageData.DamageType = EDamageType::Bullet;
			DamageData.HitBoneName = Hit.BoneName;
			DamageData.DamageEventID = EventID;
			DamageData.bIsArmorPiercing = bIsArmorPiercing;

			const bool bIsHeadshot = bCanHeadshot && (Hit.BoneName == FName("head") || Hit.BoneName.ToString().Contains(TEXT("head")));
			DamageData.bIsCriticalHit = bIsHeadshot;
			DamageData.HitRegion = bIsHeadshot ? EHitRegion::Head : EHitRegion::Torso;

			const FWeaponFalloffConfig& ActiveFalloff = bIsAiming ? AimedFalloff : HipFireFalloff;
			const float Distance = FVector::Distance(Hit.TraceStart, Hit.ImpactPoint);
			float CalculatedDamage = ActiveFalloff.CalculateDamageAtDistance(Distance);

			if (bIsHeadshot)
			{
				CalculatedDamage *= CriticalMultiplier;
			}

			DamageData.BaseDamage = CalculatedDamage;
			DamageData.FinalIncomingDamage = CalculatedDamage;

			TargetHealth->ApplyDamageEvent(DamageData);
		}
	}
}

void AWeapon::Server_FireProjectile_Implementation(const FVector& Origin, const FRotator& Direction, int32 EventID)
{
	if (!ProjectileClass) return;

	Multicast_PlayFireFX(Origin + (Direction.Vector() * 300.f)); // Optimized Unreliable Multicast

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform SpawnTransform(Direction, Origin);
	if (AProjectileBase* Projectile = GetWorld()->SpawnActor<AProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams))
	{
		FDamageEventData DamageData;
		DamageData.Instigator = GetInstigator();
		DamageData.DamageCauser = this;
		DamageData.DeliveryMethod = EDamageDeliveryMethod::Projectile;
		DamageData.DamageType = EDamageType::Explosive;
		DamageData.DamageEventID = EventID;
		DamageData.BaseDamage = HipFireFalloff.BaseDamage;
		DamageData.bIsArmorPiercing = bIsArmorPiercing;

		Projectile->InitializeProjectile(DamageData, ProjectileLaunchSpeed, ProjectileGravityScale);
	}
}

void AWeapon::StartReload()
{
	if (bIsReloading || Ammo == MagCapacity) return;

	bIsReloading = true;
	Server_StartReload();

	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AWeapon::CompleteReload, ReloadDurationSeconds, false);
}

void AWeapon::CancelReload()
{
	if (!bIsReloading) return;

	bIsReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle); // Progress resets to 0%
}

void AWeapon::Server_StartReload_Implementation()
{
	bIsReloading = true;
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AWeapon::CompleteReload, ReloadDurationSeconds, false);
}

void AWeapon::CompleteReload()
{
	bIsReloading = false;
	Ammo = MagCapacity; // Infinite reserve ammo refills to full capacity!
}

void AWeapon::OnRep_Ammo()
{
	if (DynMatInst_AmmoCounter)
	{
		DynMatInst_AmmoCounter->SetScalarParameterValue(FName("Rounds_Current"), Ammo);
	}
}

void AWeapon::Multicast_PlayFireFX_Implementation(const FVector& ImpactPoint)
{
	// Do not replay on the locally controlled player (they already predicted it locally!)
	if (GetInstigator() && GetInstigator()->IsLocallyControlled())
	{
		return;
	}

	// Remote clients play the visual/audio effects (muzzle flash, tracer, impact)
	PlayFireEffects(ImpactPoint, false);
}

void AWeapon::AttachToOwningPawn(APawn* Pawn) const
{
	if (!Pawn || !Pawn->Implements<UPlayerInterface>()) return;

	const FName AttachPoint = IPlayerInterface::Execute_GetWeaponAttachPoint(Pawn, WeaponType);
	USkeletalMeshComponent* PawnMesh1P = IPlayerInterface::Execute_GetMesh1P(Pawn);
	USkeletalMeshComponent* PawnMesh3P = IPlayerInterface::Execute_GetMesh3P(Pawn);

	if (PawnMesh1P)
	{
		Mesh1P->AttachToComponent(PawnMesh1P, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
	}
	if (PawnMesh3P)
	{
		Mesh3P->AttachToComponent(PawnMesh3P, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
	}

	if (Pawn->IsLocallyControlled())
	{
		Mesh1P->SetHiddenInGame(false);
		Mesh3P->SetHiddenInGame(true);
	}
	else
	{
		Mesh1P->SetHiddenInGame(true);
		Mesh3P->SetHiddenInGame(false);
	}
}

void AWeapon::DetachFromOwningPawn()
{
	Mesh1P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	Mesh1P->SetHiddenInGame(true);

	Mesh3P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	Mesh3P->SetHiddenInGame(true);
}

UMaterialInstanceDynamic* AWeapon::GetReticleDynamicMaterialInstance()
{
	if (!IsValid(DynMatInst_Reticle) && ReticleMaterial)
	{
		DynMatInst_Reticle = UMaterialInstanceDynamic::Create(ReticleMaterial, this);
	}
	return DynMatInst_Reticle;
}

UMaterialInstanceDynamic* AWeapon::GetAmmoCounterDynamicMaterialInstance()
{
	if (!IsValid(DynMatInst_AmmoCounter) && AmmoCounterMaterial)
	{
		DynMatInst_AmmoCounter = UMaterialInstanceDynamic::Create(AmmoCounterMaterial, this);
	}
	return DynMatInst_AmmoCounter;
}

/* =========================================================================
   Legacy Implementation Bridges (CombatComponent support)
   ========================================================================= */

void AWeapon::WeaponTrace(FHitResult& OutHit, float TraceLength)
{
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(this);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_PhysicsBody, ECR_Block);

	APawn* InstigatorPawn = GetInstigator();
	if (!IsValid(InstigatorPawn)) return;

	FVector AimLocation;
	FRotator AimRotation;

	if (InstigatorPawn->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_GetAimViewPoint(InstigatorPawn, AimLocation, AimRotation);
	}
	else
	{
		InstigatorPawn->GetActorEyesViewPoint(AimLocation, AimRotation);
	}

	const FVector Start = AimLocation;
	const FVector End = Start + (AimRotation.Vector() * TraceLength);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		FPSTraceChannels::ECC_Weapon,
		FCollisionShape::MakeSphere(5.f),
		QueryParams,
		ResponseParams);

	if (!bHit)
	{
		OutHit.ImpactPoint = End;
	}
}

void AWeapon::Local_Fire(const FVector& ImpactPoint, const FVector& ImpactNormal, TEnumAsByte<EPhysicalSurface> ImpactSurfaceType, bool bIsFirstPerson)
{
	PlayFireEffects(ImpactPoint, bIsFirstPerson);
	if (GetInstigator() && GetInstigator()->IsLocallyControlled())
	{
		Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	}
}

void AWeapon::Auth_Fire()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
}

void AWeapon::Rep_Fire(int32 AuthAmmo)
{
	if (GetInstigator() && GetInstigator()->IsLocallyControlled())
	{
		Ammo = AuthAmmo;
	}
}