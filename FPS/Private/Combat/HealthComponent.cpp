// Copyright TemmieTim

#include "Combat/HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Ability/StatusEffectComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// Default Baseline: Soldier (250 HP, 0 Shield, 0 Armor)
	MaxHealth = 250.f;
	CurrentHealth = 250.f;

	MaxShields = 0.f;
	CurrentShields = 0.f;

	MaxArmor = 0.f;
	CurrentArmor = 0.f;

	MaxOverhealth = 200.f;
	CurrentOverhealth = 0.f;

	bIsDowned = false;
	bIsInvulnerable = false;
	bIsBeingRevived = false;

	DamageRegenTimer = 0.f;
	OverhealthDecayRate = 10.f;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	BroadcastPools();
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, CurrentHealth);
	DOREPLIFETIME(UHealthComponent, MaxHealth);
	DOREPLIFETIME(UHealthComponent, CurrentShields);
	DOREPLIFETIME(UHealthComponent, MaxShields);
	DOREPLIFETIME(UHealthComponent, CurrentArmor);
	DOREPLIFETIME(UHealthComponent, MaxArmor);
	DOREPLIFETIME(UHealthComponent, CurrentOverhealth);
	DOREPLIFETIME(UHealthComponent, MaxOverhealth);
	DOREPLIFETIME(UHealthComponent, bIsDowned);
	DOREPLIFETIME(UHealthComponent, bIsInvulnerable);
	DOREPLIFETIME(UHealthComponent, bIsBeingRevived);
	DOREPLIFETIME(UHealthComponent, CurrentReviver);
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ProcessPassiveRegeneration(DeltaTime);
		ProcessOverhealthDecay(DeltaTime);
	}
}

/* =========================================================================
   Centralized Damage Pipeline (Absorption Order: Overhealth -> Shields -> Armor -> Health)
   ========================================================================= */

void UHealthComponent::ApplyDamageEvent(FDamageEventData& DamageEvent)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (bIsInvulnerable || bIsDowned) return;

	// Apply target-side status modifiers (Vulnerability bonus & Damage Reduction)
	float FinalDamage = DamageEvent.FinalIncomingDamage;
	if (UStatusEffectComponent* StatusComp = GetOwner()->FindComponentByClass<UStatusEffectComponent>())
	{
		FinalDamage *= StatusComp->GetIncomingDamageMultiplier();
	}

	float DamageRemaining = FinalDamage;
	if (DamageRemaining <= 0.f) return;

	// Reset passive regen delay upon receiving damage
	DamageRegenTimer = 0.f;

	// 1. Overhealth (Absorbs raw damage first, no mitigation) ... (rest of ApplyDamageEvent remains unchanged)
	if (CurrentOverhealth > 0.f)
	{
		const float AbsorbedByOverhealth = FMath::Min(CurrentOverhealth, DamageRemaining);
		CurrentOverhealth -= AbsorbedByOverhealth;
		DamageRemaining -= AbsorbedByOverhealth;
	}

	// 2. Shields (Absorbs raw damage second, never receives Armor mitigation)
	if (DamageRemaining > 0.f && CurrentShields > 0.f)
	{
		const float AbsorbedByShields = FMath::Min(CurrentShields, DamageRemaining);
		CurrentShields -= AbsorbedByShields;
		DamageRemaining -= AbsorbedByShields;
	}

	// 3. Armor (Conditional mitigation calculated ONCE for the portion entering Armor)
	if (DamageRemaining > 0.f && CurrentArmor > 0.f)
	{
		float DamageEnteringArmor = DamageRemaining;
		float ArmorReduction = 0.f;

		// Calculate mitigation unless weapon is Armor-Piercing
		if (!DamageEvent.bIsArmorPiercing)
		{
			// Formula: min(7.0, 0.5 * DamageEnteringArmor)
			ArmorReduction = FMath::Min(7.0f, 0.5f * DamageEnteringArmor);
		}

		const float MitigatedDamage = DamageEnteringArmor - ArmorReduction;
		const float AbsorbedByArmor = FMath::Min(CurrentArmor, MitigatedDamage);

		CurrentArmor -= AbsorbedByArmor;

		// If hit broke through remaining Armor, transition the unmitigated leftover into Health
		if (MitigatedDamage > AbsorbedByArmor)
		{
			DamageRemaining = MitigatedDamage - AbsorbedByArmor;
		}
		else
		{
			DamageRemaining = 0.f;
		}
	}

	// 4. Health (Takes remaining unmitigated damage)
	if (DamageRemaining > 0.f)
	{
		CurrentHealth = FMath::Max(0.f, CurrentHealth - DamageRemaining);

		if (CurrentHealth <= 0.f)
		{
			EnterDownedState();
		}
	}

	BroadcastPools();
	OnDamageReceived.Broadcast(DamageEvent);
}

/* =========================================================================
   Healing Pipeline (Health, Shields, Armor can be restored; Overhealth cannot)
   ========================================================================= */

void UHealthComponent::ApplyHealingEvent(FHealingEventData& HealingEvent)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (bIsDowned) return; // Downed characters can only be revived, not healed

	float AmountToApply = HealingEvent.RequestedAmount;
	if (AmountToApply <= 0.f) return;

	float Applied = 0.f;

	switch (HealingEvent.TargetPool)
	{
	case EHealthPoolType::Health:
	{
		const float Missing = MaxHealth - CurrentHealth;
		Applied = FMath::Min(Missing, AmountToApply);
		CurrentHealth += Applied;
		break;
	}
	case EHealthPoolType::Shields:
	{
		const float Missing = MaxShields - CurrentShields;
		Applied = FMath::Min(Missing, AmountToApply);
		CurrentShields += Applied;
		break;
	}
	case EHealthPoolType::Armor:
	{
		const float Missing = MaxArmor - CurrentArmor;
		Applied = FMath::Min(Missing, AmountToApply);
		CurrentArmor += Applied;
		break;
	}
	case EHealthPoolType::Overhealth:
		// Overhealth cannot be healed under any circumstances
		Applied = 0.f;
		break;
	}

	HealingEvent.ActualAmountApplied = Applied;

	if (Applied > 0.f)
	{
		// Healing does NOT reset the damage-based passive regeneration timer
		BroadcastPools();
		OnHealingReceived.Broadcast(HealingEvent);
	}
}

void UHealthComponent::AddOverhealth(float Amount, float DecayPerSecond)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDowned) return;

	CurrentOverhealth = FMath::Clamp(CurrentOverhealth + Amount, 0.f, MaxOverhealth);
	OverhealthDecayRate = FMath::Max(1.f, DecayPerSecond);

	BroadcastPools();
}

/* =========================================================================
   Passive Regeneration & Overhealth Decay
   ========================================================================= */

void UHealthComponent::ProcessPassiveRegeneration(float DeltaTime)
{
	if (bIsDowned) return;

	DamageRegenTimer += DeltaTime;
	if (DamageRegenTimer < REGEN_DAMAGE_DELAY) return;

	bool bPoolsUpdated = false;

	// 1. Health Regeneration: 10 HP/s + (5% of MaxHealth) per second
	if (CurrentHealth < MaxHealth)
	{
		const float HealthRegenRate = 10.0f + (MaxHealth * 0.05f);
		CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + (HealthRegenRate * DeltaTime));
		bPoolsUpdated = true;
	}

	// 2. Shield Regeneration: 30 Shields per second (runs independently and stacks additively)
	if (CurrentShields < MaxShields)
	{
		CurrentShields = FMath::Min(MaxShields, CurrentShields + (SHIELD_REGEN_RATE * DeltaTime));
		bPoolsUpdated = true;
	}

	// Armor and Overhealth do not passively regenerate

	if (bPoolsUpdated)
	{
		BroadcastPools();
	}
}

void UHealthComponent::ProcessOverhealthDecay(float DeltaTime)
{
	if (CurrentOverhealth > 0.f)
	{
		CurrentOverhealth = FMath::Max(0.f, CurrentOverhealth - (OverhealthDecayRate * DeltaTime));
		BroadcastPools();
	}
}

/* =========================================================================
   Downed & Revival Mechanics
   ========================================================================= */

void UHealthComponent::EnterDownedState()
{
	bIsDowned = true;
	bIsInvulnerable = true;
	bIsBeingRevived = false;
	CurrentReviver = nullptr;

	CurrentHealth = 0.f;
	CurrentShields = 0.f;
	CurrentArmor = 0.f;
	CurrentOverhealth = 0.f;

	OnDownedStateChanged.Broadcast(true);
}

bool UHealthComponent::ReserveRevival(AActor* Reviver)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!bIsDowned || bIsBeingRevived || !Reviver) return false;

	bIsBeingRevived = true;
	CurrentReviver = Reviver;
	return true;
}

void UHealthComponent::CancelRevival(AActor* Reviver)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (CurrentReviver.Get() == Reviver)
	{
		bIsBeingRevived = false;
		CurrentReviver = nullptr;
	}
}

void UHealthComponent::CompleteRevival()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!bIsDowned) return;

	// Restore Health, Shields, and Armor to 100% max values (Overhealth is not restored)
	CurrentHealth = MaxHealth;
	CurrentShields = MaxShields;
	CurrentArmor = MaxArmor;
	CurrentOverhealth = 0.f;

	bIsDowned = false;
	bIsBeingRevived = false;
	CurrentReviver = nullptr;
	bIsInvulnerable = true;

	// Grant 1.5 seconds of invulnerability upon standing up
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PostReviveInvulnTimerHandle, this, &UHealthComponent::EndPostReviveInvulnerability, POST_REVIVE_INVULN_DURATION, false);
	}

	BroadcastPools();
	OnDownedStateChanged.Broadcast(false);
}

void UHealthComponent::EndPostReviveInvulnerability()
{
	bIsInvulnerable = false;
}

/* =========================================================================
   Pool Management & Passive Stat Injection
   ========================================================================= */

void UHealthComponent::SetPoolMaxValues(float InMaxHealth, float InMaxShields, float InMaxArmor, bool bResetToMax)
{
	MaxHealth = FMath::Max(1.f, InMaxHealth);
	MaxShields = FMath::Max(0.f, InMaxShields);
	MaxArmor = FMath::Max(0.f, InMaxArmor);

	if (bResetToMax)
	{
		CurrentHealth = MaxHealth;
		CurrentShields = MaxShields;
		CurrentArmor = MaxArmor;
		CurrentOverhealth = 0.f;
	}
	else
	{
		CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
		CurrentShields = FMath::Min(CurrentShields, MaxShields);
		CurrentArmor = FMath::Min(CurrentArmor, MaxArmor);
	}

	BroadcastPools();
}

void UHealthComponent::ModifyMaxArmorBonus(float ArmorDelta)
{
	MaxArmor = FMath::Max(0.f, MaxArmor + ArmorDelta);
	CurrentArmor = FMath::Clamp(CurrentArmor + ArmorDelta, 0.f, MaxArmor);
	BroadcastPools();
}

void UHealthComponent::ModifyMaxShieldBonus(float ShieldDelta)
{
	MaxShields = FMath::Max(0.f, MaxShields + ShieldDelta);
	CurrentShields = FMath::Clamp(CurrentShields + ShieldDelta, 0.f, MaxShields);
	BroadcastPools();
}

void UHealthComponent::OnRep_HealthPools()
{
	BroadcastPools();
}

void UHealthComponent::OnRep_DownedState()
{
	OnDownedStateChanged.Broadcast(bIsDowned);
}

void UHealthComponent::BroadcastPools()
{
	OnHealthPoolsChanged.Broadcast(CurrentHealth, MaxHealth, CurrentShields, MaxShields, CurrentArmor, MaxArmor, CurrentOverhealth, MaxOverhealth);
}