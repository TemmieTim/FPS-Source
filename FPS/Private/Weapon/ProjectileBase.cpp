// Copyright TemmieTim

#include "Weapon/ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Combat/HealthComponent.h"
#include "Weapon/DeployableBase.h"
#include "Engine/OverlapResult.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SphereCollision = CreateDefaultSubobject<USphereComponent>("SphereCollision");
	SphereCollision->InitSphereRadius(16.f);
	SphereCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = SphereCollision;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->UpdatedComponent = SphereCollision;
	ProjectileMovement->InitialSpeed = 3500.f;
	ProjectileMovement->MaxSpeed = 3500.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	DetonationType = EProjectileDetonationType::OnImpact;
	FuseDuration = 0.6f;
	bShouldBounce = false;
	bShouldStick = false;
	ExplosionRadiusMetres = 4.0f;
	MinSplashDamage = 55.f;
	MaxSplashDamage = 150.f;
	bHasDetonated = false;
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	SphereCollision->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);

	if (HasAuthority() && DetonationType == EProjectileDetonationType::TimedAfterSpawn)
	{
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ThisClass::Detonate, FuseDuration, false);
	}
}

void AProjectileBase::InitializeProjectile(const FDamageEventData& InDamageData, float InSpeed, float InGravityScale)
{
	StoredDamageData = InDamageData;

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InSpeed;
		ProjectileMovement->MaxSpeed = InSpeed;
		ProjectileMovement->ProjectileGravityScale = InGravityScale;
		ProjectileMovement->bShouldBounce = bShouldBounce;
	}
}

void AProjectileBase::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bHasDetonated) return;

	if (bShouldStick)
	{
		ProjectileMovement->StopMovementImmediately();
		AttachToComponent(OtherComp, FAttachmentTransformRules::KeepWorldTransform);
	}

	if (DetonationType == EProjectileDetonationType::OnImpact)
	{
		// Apply direct impact damage if hitting an actor directly
		if (OtherActor && OtherActor != GetOwner() && OtherActor != GetInstigator())
		{
			if (UHealthComponent* TargetHealth = OtherActor->FindComponentByClass<UHealthComponent>())
			{
				FDamageEventData DirectDamage = StoredDamageData;
				DirectDamage.Target = OtherActor;
				DirectDamage.FinalIncomingDamage = DirectDamage.BaseDamage;
				TargetHealth->ApplyDamageEvent(DirectDamage);
			}
		}

		Detonate();
	}
	else if (DetonationType == EProjectileDetonationType::TimedAfterImpact)
	{
		if (!GetWorldTimerManager().IsTimerActive(FuseTimerHandle))
		{
			GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ThisClass::Detonate, FuseDuration, false);
		}
	}
	else if (DetonationType == EProjectileDetonationType::DeployOnImpact)
	{
		Detonate();
	}
}

void AProjectileBase::Detonate()
{
	if (bHasDetonated) return;
	bHasDetonated = true;

	if (HasAuthority())
	{
		if (DetonationType == EProjectileDetonationType::DeployOnImpact && DeployableClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GetWorld()->SpawnActor<ADeployableBase>(DeployableClass, GetActorTransform(), SpawnParams);
		}
		else
		{
			ApplyRadialExplosionDamage();
		}
	}

	Destroy();
}

void AProjectileBase::ApplyRadialExplosionDamage()
{
	const FVector BlastOrigin = GetActorLocation();
	const float RadiusUU = ExplosionRadiusMetres * 100.f;

	TArray<FOverlapResult> Overlaps;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(RadiusUU);

	FCollisionQueryParams OverlapParams;
	OverlapParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->OverlapMultiByChannel(Overlaps, BlastOrigin, FQuat::Identity, ECC_Pawn, SphereShape, OverlapParams);
	if (!bHit) return;

	TSet<AActor*> DamagedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor)) continue;
		DamagedActors.Add(HitActor);

		// Line-of-sight check: solid cover blocks explosion damage
		FHitResult SightHit;
		FCollisionQueryParams SightParams;
		SightParams.AddIgnoredActor(this);
		SightParams.AddIgnoredActor(HitActor);

		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(SightHit, BlastOrigin, HitActor->GetActorLocation(), ECC_Visibility, SightParams);
		if (bBlocked) continue; // Occluded by solid wall

		// Calculate linear radial falloff based on distance from explosion center
		const float Distance = FVector::Distance(BlastOrigin, HitActor->GetActorLocation());
		const float Alpha = FMath::Clamp(Distance / RadiusUU, 0.f, 1.f);
		const float RadialDamage = FMath::Lerp(MaxSplashDamage, MinSplashDamage, Alpha);

		if (UHealthComponent* TargetHealth = HitActor->FindComponentByClass<UHealthComponent>())
		{
			FDamageEventData SplashDamage = StoredDamageData;
			SplashDamage.Target = HitActor;
			SplashDamage.DeliveryMethod = EDamageDeliveryMethod::Radial;
			SplashDamage.DamageType = EDamageType::Explosive;
			SplashDamage.FinalIncomingDamage = RadialDamage;
			TargetHealth->ApplyDamageEvent(SplashDamage);
		}
	}
}