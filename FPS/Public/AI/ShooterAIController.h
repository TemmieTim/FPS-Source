// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShooterAITypes.h"
#include "ShooterAIController.generated.h"

UCLASS()
class FPS_API AShooterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AShooterAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Issues unified combat commands directly to the possessed character */
	void CommandFire(bool bPressed);
	void CommandAim(bool bPressed);
	void CommandReload();
	void CommandSelectSlot(int32 SlotIndex);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree;
};