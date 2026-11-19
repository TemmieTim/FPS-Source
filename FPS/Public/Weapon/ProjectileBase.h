// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/DamageTypes.h"
#include "Weapon/WeaponTypes.h"
#include "ProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class ADeployableBase;

UCLASS()
class FPS_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AProjectileBase();

	void InitializeProjectile(const FDamageEventData& InDamageData, float InSpeed, float InGravityScale = 1.0f);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Collision")
	TObjectPtr<USphereComponent> SphereCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Movement")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// Projectile Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Detonation")
	EProjectileDetonationType DetonationType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Detonation")
	float FuseDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Detonation")
	bool bShouldBounce;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Detonation")
	bool bShouldStick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Explosion")
	float ExplosionRadiusMetres;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Explosion")
	float MinSplashDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Explosion")
	float MaxSplashDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Deployable")
	TSubclassOf<ADeployableBase> DeployableClass;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual void Detonate();

	FDamageEventData StoredDamageData;

private:
	void ApplyRadialExplosionDamage();
	FTimerHandle FuseTimerHandle;
	bool bHasDetonated;
};