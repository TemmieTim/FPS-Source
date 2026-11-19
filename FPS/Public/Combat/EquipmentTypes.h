// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "EquipmentTypes.generated.h"

class AWeapon;

UENUM(BlueprintType)
enum class EEquipmentCategory : uint8
{
	None UMETA(DisplayName = "None"),
	Primary UMETA(DisplayName = "Primary Weapon"),
	Secondary UMETA(DisplayName = "Secondary Weapon"),
	Melee UMETA(DisplayName = "Melee Weapon"),
	Gadget UMETA(DisplayName = "Gadget"),
	Passive UMETA(DisplayName = "Passive Equipment")
};

/** Static configuration for a slot inside a Hero Data Asset */
USTRUCT(BlueprintType)
struct FPS_API FHeroEquipmentSlotConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Equipment")
	EEquipmentCategory Category = EEquipmentCategory::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Equipment")
	FName EquipmentID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Equipment")
	TSubclassOf<AWeapon> WeaponClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Equipment")
	bool bIsPassive = false;
};

/** Runtime state of an individual equipment slot on the character */
USTRUCT(BlueprintType)
struct FPS_API FEquipmentSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Equipment")
	EEquipmentCategory Category = EEquipmentCategory::None;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Equipment")
	FName EquipmentID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Equipment")
	TObjectPtr<AWeapon> SpawnedWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Equipment")
	bool bIsPassive = false;

	bool IsSelectable() const
	{
		if (Category == EEquipmentCategory::None || bIsPassive) return false;
		if (Category == EEquipmentCategory::Primary || Category == EEquipmentCategory::Secondary || Category == EEquipmentCategory::Melee)
		{
			return SpawnedWeapon != nullptr;
		}
		// Gadget availability will query charges/cooldown in Phase 6
		return SpawnedWeapon != nullptr;
	}
};