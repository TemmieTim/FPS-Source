// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeployableBase.generated.h"

class UHealthComponent;

UCLASS()
class FPS_API ADeployableBase : public AActor
{
	GENERATED_BODY()

public:
	ADeployableBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Health")
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Lifetime")
	float MaxLifetimeSeconds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Lifetime")
	bool bHasLifetime;

	UFUNCTION()
	virtual void HandleDowned(bool bIsDowned);

private:
	FTimerHandle LifetimeTimerHandle;
};