// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ShooterTypes/ShooterTypes.h"
#include "Combat/DamageTypes.h"
#include "Weapon/WeaponTypes.h"
#include "Weapon.generated.h"

class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class AProjectileBase;
enum EPhysicalSurface : int;

// Legacy Enums preserved for CombatComponent & UI backward compatibility
UENUM(BlueprintType)
enum class EFireType : uint8
{
	Auto UMETA(DisplayName = "Automatic"),
	SemiAuto UMETA(DisplayName = "SemiAutomatic")
};

UENUM(BlueprintType)
enum class EWeaponStatus : uint8
{
	Idle,
	Firing,
	Reloading,
	Cycling,
	Unequipped
};

UCLASS()
class FPS_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Universal Trigger Interface (Phase 5)
	void StartFire();
	void StopFire();
	void StartReload();
	void CancelReload();

	// Attachment & Visibility
	void AttachToOwningPawn(APawn* Pawn) const;
	void DetachFromOwningPawn();

	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	USkeletalMeshComponent* GetMesh3P() const { return Mesh3P; }
	UMaterialInstanceDynamic* GetReticleDynamicMaterialInstance();
	UMaterialInstanceDynamic* GetAmmoCounterDynamicMaterialInstance();

	// Configurable Identity & Category
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Weapon")
	FName WeaponID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Weapon")
	FGameplayTag WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Firing")
	EFireMode FireMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Firing")
	EDamageDeliveryMethod DeliveryMethod;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Firing")
	float FireIntervalSeconds;

	// Ammunition (Finite Mag, Infinite Reserve)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Ammo")
	int32 MagCapacity;

	UPROPERTY(ReplicatedUsing = OnRep_Ammo, BlueprintReadOnly, Category = "FPS|Ammo")
	int32 Ammo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Ammo")
	float ReloadDurationSeconds;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FPS|Ammo")
	bool bIsReloading;

	// Damage, Falloff & Accuracy
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	FWeaponFalloffConfig HipFireFalloff;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	FWeaponFalloffConfig AimedFalloff;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	FWeaponSpreadConfig SpreadConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	int32 PelletCount; // > 1 for Shotguns

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	bool bCanHeadshot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	float CriticalMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	bool bIsArmorPiercing;

	// Projectile Spawning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Projectile")
	float ProjectileLaunchSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Projectile")
	float ProjectileGravityScale;

	// Aiming & Presentation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Aiming")
	float AimFieldOfView;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Reticle")
	FReticleParams ReticleParams;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Icon")
	TObjectPtr<UMaterialInterface> WeaponIcon;

	void SetAiming(bool bNewAiming) { bIsAiming = bNewAiming; }

	// =========================================================================
	// Legacy Bridges (Preserved for CombatComponent compatibility)
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Legacy")
	EFireType FireType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Legacy")
	float FireTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Legacy")
	int32 StartingCarriedAmmo;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Legacy")
	EWeaponStatus WeaponStatus;

	void WeaponTrace(FHitResult& OutHit, float TraceLength);
	void Local_Fire(const FVector& ImpactPoint, const FVector& ImpactNormal, TEnumAsByte<EPhysicalSurface> ImpactSurfaceType, bool bIsFirstPerson);
	void Auth_Fire();
	void Rep_Fire(int32 AuthAmmo);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh3P;

	// 5-parameter signature matching your BP_Weapon Event FireEffects node
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Effects")
	void FireEffects(const FVector& ImpactPoint, const FVector& CrosshairTarget, const FVector& ImpactNormal, EPhysicalSurface ImpactSurfaceType, bool bIsFirstPerson);

private:
	void ExecuteFireCycle();
	void FireHitscanRound();
	void FireProjectileRound();

	UFUNCTION(Server, Reliable)
	void Server_FireHitscan(const TArray<FHitResult>& Hits, int32 EventID);

	UFUNCTION(Server, Reliable)
	void Server_FireProjectile(const FVector& Origin, const FRotator& Direction, int32 EventID);

	// The missing declaration that caused the errors:
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireFX(const FVector& ImpactPoint);

	UFUNCTION(Server, Reliable)
	void Server_StartReload();

	void CompleteReload();

	UFUNCTION()
	void OnRep_Ammo();

	float CurrentSpread;
	bool bIsAiming;
	bool bWantsToFire;
	int32 BurstShotsRemaining;

	FTimerHandle FireTimerHandle;
	FTimerHandle BurstTimerHandle;
	FTimerHandle ReloadTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Material")
	TObjectPtr<UMaterialInterface> ReticleMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "FPS|Material")
	TObjectPtr<UMaterialInterface> AmmoCounterMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynMatInst_Reticle;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynMatInst_AmmoCounter;
};