// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/EquipmentTypes.h"
#include "HeroDataAsset.generated.h"

UCLASS(BlueprintType)
class FPS_API UHeroDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Permanent internal identifier (e.g. Hero_Soldier, Hero_Scout)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Identity")
	FName InternalHeroID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Identity")
	FText DisplayName;

	// Defensive Attributes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Health")
	float BaseHealth = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Health")
	float BaseShields = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Health")
	float BaseArmor = 0.f;

	// Movement Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Movement")
	float WalkSpeed = 510.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Movement")
	float SprintSpeed = 660.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Movement")
	float JumpZVelocity = 570.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Movement")
	bool bCanDoubleJump = false; // Bunny Scout passive

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Movement")
	bool bHasSprintInvisibility = false; // Fox Agent passive

	// Character Presentation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Presentation")
	TObjectPtr<USkeletalMesh> FirstPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Presentation")
	TSubclassOf<UAnimInstance> FirstPersonAnimClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Presentation")
	TObjectPtr<USkeletalMesh> ThirdPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Presentation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimClass;

	// Default 4-Slot Loadout (Slot 1 = Index 0, Slot 2 = Index 1, etc.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Loadout")
	TArray<FHeroEquipmentSlotConfig> DefaultLoadout;
};