// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "DamageTypes.generated.h"

UENUM(BlueprintType)
enum class EDamageDeliveryMethod : uint8
{
	Hitscan UMETA(DisplayName = "Hitscan"),
	Projectile UMETA(DisplayName = "Projectile"),
	MeleeTrace UMETA(DisplayName = "Melee Trace"),
	Radial UMETA(DisplayName = "Radial / Area of Effect"),
	Environmental UMETA(DisplayName = "Environmental"),
	Direct UMETA(DisplayName = "Direct")
};

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Bullet UMETA(DisplayName = "Bullet"),
	Explosive UMETA(DisplayName = "Explosive"),
	Melee UMETA(DisplayName = "Melee"),
	Environmental UMETA(DisplayName = "Environmental"),
	Energy UMETA(DisplayName = "Energy")
};

UENUM(BlueprintType)
enum class EDamageSourceCategory : uint8
{
	Weapon UMETA(DisplayName = "Weapon"),
	Gadget UMETA(DisplayName = "Gadget"),
	Ability UMETA(DisplayName = "Ability"),
	WorldHazard UMETA(DisplayName = "World / Hazard")
};

UENUM(BlueprintType)
enum class EHitRegion : uint8
{
	Default UMETA(DisplayName = "Default"),
	Head UMETA(DisplayName = "Head"),
	Torso UMETA(DisplayName = "Torso"),
	Limbs UMETA(DisplayName = "Limbs"),
	Weakpoint UMETA(DisplayName = "Weakpoint")
};

UENUM(BlueprintType)
enum class EHealthPoolType : uint8
{
	Health UMETA(DisplayName = "Health"),
	Shields UMETA(DisplayName = "Shields"),
	Armor UMETA(DisplayName = "Armor"),
	Overhealth UMETA(DisplayName = "Overhealth")
};

/**
 * Immutable damage event passed through the centralized damage pipeline.
 * Attribution remains valid even if the causer weapon or projectile is destroyed.
 */
USTRUCT(BlueprintType)
struct FPS_API FDamageEventData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	TWeakObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	TWeakObjectPtr<AActor> DamageCauser = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	TWeakObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	EDamageSourceCategory SourceCategory = EDamageSourceCategory::Weapon;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	EDamageType DamageType = EDamageType::Bullet;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	EDamageDeliveryMethod DeliveryMethod = EDamageDeliveryMethod::Hitscan;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	EHitRegion HitRegion = EHitRegion::Default;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	FName HitBoneName = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	bool bIsCriticalHit = false;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	bool bIsArmorPiercing = false;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	float BaseDamage = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	float FinalIncomingDamage = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Damage")
	int32 DamageEventID = 0;
};

/**
 * Structured healing event.
 */
USTRUCT(BlueprintType)
struct FPS_API FHealingEventData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	TWeakObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	TWeakObjectPtr<AActor> HealingSource = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	TWeakObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	EDamageSourceCategory SourceCategory = EDamageSourceCategory::Weapon;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	EHealthPoolType TargetPool = EHealthPoolType::Health;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	float RequestedAmount = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	float ActualAmountApplied = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "FPS|Healing")
	int32 HealingEventID = 0;
};