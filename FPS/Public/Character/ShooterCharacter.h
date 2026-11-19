// Copyright TemmieTim

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/PlayerInterface.h"
#include "Interfaces/CombatInterface.h"
#include "ShooterTypes/ShooterTypes.h"
#include "Character/ShooterMovementComponent.h"
#include "Character/CharacterStateComponent.h"
#include "Combat/HealthComponent.h"
#include "Combat/EquipmentComponent.h"
#include "Data/HeroDataAsset.h"
#include "Ability/StatusEffectComponent.h"
#include "Ability/GadgetComponent.h"
#include "ShooterCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USkeletalMeshComponent;
class UCombatComponent;
class AWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponFirstReplicated, AWeapon*, Weapon, bool, bTargetingPlayer);

UCLASS()
class FPS_API AShooterCharacter : public ACharacter, public IPlayerInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	AShooterCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_PlayerState() override;

	/** ~Begin IPlayerInterface */
	virtual FName GetWeaponAttachPoint_Implementation(const FGameplayTag& WeaponType) const override;
	virtual USkeletalMeshComponent* GetMesh1P_Implementation() const override;
	virtual USkeletalMeshComponent* GetMesh3P_Implementation() const override;
	virtual void WeaponReplicated_Implementation() override;
	virtual AWeapon* GetCurrentWeapon_Implementation() override;
	virtual int32 GetReserveAmmo_Implementation() const override;
	virtual void Notify_CycleWeapon_Implementation() override;
	virtual void Notify_ReloadWeapon_Implementation() override;
	/** ~End IPlayerInterface */

	/** ~Begin ICombatInterface (Shared Human + AI) */
	virtual void GetAimViewPoint_Implementation(FVector& OutLocation, FRotator& OutRotation) const override;
	virtual void RequestFire_Implementation(bool bPressed) override;
	virtual void RequestAim_Implementation(bool bPressed) override;
	virtual void RequestReload_Implementation() override;
	virtual void RequestCycleWeapon_Implementation(int32 Direction) override;
	virtual void RequestSelectSlot_Implementation(int32 SlotIndex) override;
	/** ~End ICombatInterface */

	/** Universal Locomotion Requests (Used by both Player Input & AI Behavior Trees) */
	void RequestMove(const FVector2D& MoveInput);
	void RequestJump();
	void RequestStopJumping();
	void RequestToggleCrouch();
	void RequestSprint(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "FPS|Camera")
	void SetUseThirdPersonCamera(bool bUseThirdPerson);

	UFUNCTION(BlueprintCallable, Category = "FPS|Combat")
	FRotator GetFixedAimRotation() const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Combat")
	bool HasCurrentWeapon() const;

	bool HasWeaponFirstReplicated() const { return bWeaponFirstReplicated; }

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FWeaponFirstReplicated OnWeaponFirstReplicated;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|FABRIK")
	FTransform FABRIK_SocketTransform;

	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	UHealthComponent* GetHealthComponent() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	bool IsDowned() const { return Health && Health->IsDowned(); }

	UFUNCTION(BlueprintPure, Category = "FPS|State")
	UCharacterStateComponent* GetCharacterStateComponent() const { return CharacterState; }

	UFUNCTION(BlueprintPure, Category = "FPS|Movement")
	UShooterMovementComponent* GetShooterMovementComponent() const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Hero")
	void InitializeFromHeroData(const UHeroDataAsset* HeroData);

	UFUNCTION(BlueprintPure, Category = "FPS|Equipment")
	UEquipmentComponent* GetEquipmentComponent() const { return Equipment; }

	UFUNCTION(BlueprintPure, Category = "FPS|Status")
	UStatusEffectComponent* GetStatusEffectComponent() const { return StatusEffect; }

	UFUNCTION(BlueprintPure, Category = "FPS|Gadget")
	UGadgetComponent* GetGadgetComponent() const { return Gadget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Combat")
	TObjectPtr<UCombatComponent> Combat;

	/** First Person View (Arms and primary camera) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	/** External Orbit View (Used for Downed Crawling, Emotes, and Third-Person view) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Camera")
	TObjectPtr<USpringArmComponent> OrbitSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Camera")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Camera")
	float DefaultFieldOfView;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|TurnInPlace")
	float AO_Yaw;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Strafing")
	float MovementOffsetYaw;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|TurnInPlace")
	ETurningInPlace TurningStatus;

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Aim")
	void OnAim(bool bIsAiming);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Health")
	TObjectPtr<UHealthComponent> Health;

	UFUNCTION()
	void HandleDownedStateChanged(bool bNewIsDowned);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|State")
	TObjectPtr<UCharacterStateComponent> CharacterState;

	UFUNCTION()
	void HandleDamageReceived(const FDamageEventData& DamageData);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Hero")
	TObjectPtr<UHeroDataAsset> InitialHeroData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Equipment")
	TObjectPtr<UEquipmentComponent> Equipment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Status")
	TObjectPtr<UStatusEffectComponent> StatusEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Gadget")
	TObjectPtr<UGadgetComponent> Gadget;

private:
	void CalculateFABRIKSocketTransform();
	void CalculateTurnInPlaceParameters(float DeltaTime);
	void TurnInPlace(float DeltaTime);

	bool bWeaponFirstReplicated;
	bool bIsSprinting;
	FRotator StartingAimRotation;
	float InterpAO_Yaw = 0.f;
};