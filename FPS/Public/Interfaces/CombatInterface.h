// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class FPS_API ICombatInterface
{
	GENERATED_BODY()

public:
	/**
	 * Returns the authoritative viewpoint for aiming and projectile/hitscan traces.
	 * Human: active First-Person Camera or Third-Person Orbit Camera.
	 * Bot AI: focal point / eyes view location and rotation.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Combat")
	void GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	/** Universal Action Requests (Usable identically by Player Input and AI Behavior Tree tasks) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Combat")
	void RequestFire(bool bPressed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Combat")
	void RequestAim(bool bPressed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Combat")
	void RequestReload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Combat")
	void RequestCycleWeapon(int32 Direction);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Combat")
	void RequestSelectSlot(int32 SlotIndex);
};