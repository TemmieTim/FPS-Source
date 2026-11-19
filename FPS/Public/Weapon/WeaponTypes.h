// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Automatic UMETA(DisplayName = "Automatic"),
	SemiAutomatic UMETA(DisplayName = "Semi-Automatic"),
	Burst UMETA(DisplayName = "Burst"),
	PumpAction UMETA(DisplayName = "Pump-Action"),
	ChargeAndRelease UMETA(DisplayName = "Charge and Release")
};

UENUM(BlueprintType)
enum class EProjectileDetonationType : uint8
{
	OnImpact UMETA(DisplayName = "Detonate On Impact"),
	TimedAfterImpact UMETA(DisplayName = "Detonate After Impact Fuse"),
	TimedAfterSpawn UMETA(DisplayName = "Detonate After Spawn Fuse"),
	DeployOnImpact UMETA(DisplayName = "Deploy Entity On Impact")
};

USTRUCT(BlueprintType)
struct FPS_API FWeaponFalloffConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Falloff")
	bool bHasFalloff = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Falloff")
	float MinRangeMetres = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Falloff")
	float MaxRangeMetres = 35.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Falloff")
	float BaseDamage = 19.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Falloff")
	float MinDamageFloor = 6.f;

	float CalculateDamageAtDistance(float DistanceUU) const
	{
		if (!bHasFalloff) return BaseDamage;

		const float DistMetres = DistanceUU / 100.f;
		if (DistMetres <= MinRangeMetres) return BaseDamage;
		if (DistMetres >= MaxRangeMetres) return MinDamageFloor;

		const float Alpha = (DistMetres - MinRangeMetres) / FMath::Max(0.01f, MaxRangeMetres - MinRangeMetres);
		return FMath::Lerp(BaseDamage, MinDamageFloor, Alpha);
	}
};

USTRUCT(BlueprintType)
struct FPS_API FWeaponSpreadConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Spread")
	float BaseSpreadDegrees = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Spread")
	float MaxSpreadDegrees = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Spread")
	float SpreadGrowthPerShot = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Spread")
	float SpreadRecoveryRate = 5.0f;
};