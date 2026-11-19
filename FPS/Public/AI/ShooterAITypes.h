// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "ShooterAITypes.generated.h"

UENUM(BlueprintType)
enum class EAICombatRole : uint8
{
	GeneralSupport UMETA(DisplayName = "General Support"),
	AggressiveAssault UMETA(DisplayName = "Aggressive Assault"),
	PrecisionOverwatch UMETA(DisplayName = "Precision Overwatch"),
	SustainedSuppression UMETA(DisplayName = "Sustained Suppression"),
	Flanker UMETA(DisplayName = "Flanker"),
	HealerSupport UMETA(DisplayName = "Healer Support")
};

namespace ShooterBlackboardKeys
{
	const FName TargetActor = FName("TargetActor");
	const FName TargetLocation = FName("TargetLocation");
	const FName SquadLeader = FName("SquadLeader");
	const FName CombatRole = FName("CombatRole");
	const FName IsDowned = FName("IsDowned");
	const FName ReviveTarget = FName("ReviveTarget");
}