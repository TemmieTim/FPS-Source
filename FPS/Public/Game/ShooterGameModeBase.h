// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameModeBase.generated.h"

class AShooterCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSquadWiped);

UCLASS()
class FPS_API AShooterGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShooterGameModeBase();

	/** Registers squad member into the squad tracker */
	UFUNCTION(BlueprintCallable, Category = "FPS|Squad")
	void RegisterSquadMember(AShooterCharacter* SquadMember);

	/** Evaluates squad status; triggers Game Over only when ALL 4 squad members are downed */
	UFUNCTION(BlueprintCallable, Category = "FPS|Squad")
	void CheckSquadStatus();

	UFUNCTION(BlueprintPure, Category = "FPS|GameMode")
	bool IsSoloMode() const { return bIsSoloMode; }

	UFUNCTION(BlueprintPure, Category = "FPS|Squad")
	const TArray<AShooterCharacter*>& GetSquadMembers() const { return ActiveSquad; }

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnSquadWiped OnSquadWiped;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|GameMode")
	bool bIsSoloMode;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FPS|Squad")
	TArray<TObjectPtr<AShooterCharacter>> ActiveSquad;

	UFUNCTION()
	void HandleSquadMemberDownedStateChanged(bool bIsDowned);

	virtual void HandleSquadWipe();
};