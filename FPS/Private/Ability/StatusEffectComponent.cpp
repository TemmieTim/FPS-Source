// Copyright TemmieTim

#include "Ability/StatusEffectComponent.h"
#include "Net/UnrealNetwork.h"

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	EffectiveMovementMultiplier = 1.0f;
	ActiveVulnerabilityMultiplier = 1.0f;
	ActiveDamageReduction = 0.0f;
	VulnerabilityDurationRemaining = 0.0f;
}

void UStatusEffectComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatusEffectComponent, EffectiveMovementMultiplier);
	DOREPLIFETIME(UStatusEffectComponent, ActiveVulnerabilityMultiplier);
	DOREPLIFETIME(UStatusEffectComponent, ActiveDamageReduction);
}

void UStatusEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UpdateActiveSlows(DeltaTime);
		UpdateVulnerability(DeltaTime);
	}
}

void UStatusEffectComponent::ApplySlowEffect(FName SourceID, float SlowPercentage, float DurationSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	// Check if source already exists; refresh duration if so
	for (FSlowInstance& Existing : ActiveSlows)
	{
		if (Existing.SourceID == SourceID)
		{
			Existing.SlowPercentage = SlowPercentage;
			Existing.DurationRemaining = FMath::Max(Existing.DurationRemaining, DurationSeconds);
			return;
		}
	}

	FSlowInstance NewSlow;
	NewSlow.SourceID = SourceID;
	NewSlow.SlowPercentage = FMath::Clamp(SlowPercentage, 0.f, 0.95f);
	NewSlow.DurationRemaining = DurationSeconds;
	ActiveSlows.Add(NewSlow);
}

void UStatusEffectComponent::UpdateActiveSlows(float DeltaTime)
{
	float MaxSlow = 0.0f;

	for (int32 Index = ActiveSlows.Num() - 1; Index >= 0; --Index)
	{
		ActiveSlows[Index].DurationRemaining -= DeltaTime;

		if (ActiveSlows[Index].DurationRemaining <= 0.0f)
		{
			ActiveSlows.RemoveAt(Index);
		}
		else
		{
			// Find the strongest active slow (NON-ADDITIVE rule)
			MaxSlow = FMath::Max(MaxSlow, ActiveSlows[Index].SlowPercentage);
		}
	}

	const float NewMultiplier = 1.0f - MaxSlow;
	if (!FMath::IsNearlyEqual(EffectiveMovementMultiplier, NewMultiplier, 0.01f))
	{
		EffectiveMovementMultiplier = NewMultiplier;
		OnRep_StatusValues();
	}
}

void UStatusEffectComponent::ApplyVulnerability(float VulnerabilityBonus, float DurationSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	ActiveVulnerabilityMultiplier = 1.0f + FMath::Max(0.0f, VulnerabilityBonus);
	VulnerabilityDurationRemaining = DurationSeconds;
	OnRep_StatusValues();
}

void UStatusEffectComponent::SetDamageReduction(float ReductionPercentage, bool bActive)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	ActiveDamageReduction = bActive ? FMath::Clamp(ReductionPercentage, 0.0f, 0.90f) : 0.0f;
	OnRep_StatusValues();
}

void UStatusEffectComponent::UpdateVulnerability(float DeltaTime)
{
	if (VulnerabilityDurationRemaining > 0.0f)
	{
		VulnerabilityDurationRemaining -= DeltaTime;

		if (VulnerabilityDurationRemaining <= 0.0f)
		{
			ActiveVulnerabilityMultiplier = 1.0f;
			OnRep_StatusValues();
		}
	}
}

float UStatusEffectComponent::GetIncomingDamageMultiplier() const
{
	// Combine vulnerability (+50%) and damage reduction (40% off) multiplicatively
	const float DefenseFactor = 1.0f - ActiveDamageReduction;
	return ActiveVulnerabilityMultiplier * DefenseFactor;
}

void UStatusEffectComponent::OnRep_StatusValues()
{
	OnSlowStateChanged.Broadcast(EffectiveMovementMultiplier);
	OnDamageModifierChanged.Broadcast(GetIncomingDamageMultiplier());
}