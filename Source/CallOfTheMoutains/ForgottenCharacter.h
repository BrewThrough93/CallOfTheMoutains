// CallOfTheMoutains - The Forgotten Enemy Character
// Basic zombie-like enemy for souls-like combat

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ForgottenCharacter.generated.h"

class UHealthComponent;
class UFootstepComponent;
class UMeleeTraceComponent;
class UTargetableComponent;
class UAnimMontage;
class USoundBase;

/** Combat state for the Forgotten */
UENUM(BlueprintType)
enum class EForgottenState : uint8
{
	Idle,
	Patrolling,
	Chasing,
	Attacking,
	Staggered,
	Dead
};

/**
 * The Forgotten - Basic zombie enemy
 * Slow-moving, pursues player on sight/sound, attacks in melee range
 */
UCLASS()
class CALLOFTHEMOUTAINS_API AForgottenCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AForgottenCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ==================== Components ====================

	/** Health component for damage/death */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	/** Footstep sounds */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFootstepComponent* FootstepComponent;

	/** Melee attack traces */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UMeleeTraceComponent* MeleeTraceComponent;

	/** Targetable component for lock-on system */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTargetableComponent* TargetableComponent;

	// ==================== Movement Settings ====================

	/** Normal patrol/idle movement speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Movement")
	float PatrolSpeed = 100.0f;

	/** Chase speed when pursuing player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Movement")
	float ChaseSpeed = 200.0f;

	// ==================== Combat Settings ====================

	/** Distance to start attacking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Combat")
	float AttackRange = 150.0f;

	/** Minimum time between attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Combat")
	float AttackCooldown = 2.0f;

	/** Damage dealt per attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Combat")
	float AttackDamage = 20.0f;

	/** Time stunned after taking damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Combat")
	float StaggerDuration = 0.5f;

	// ==================== Detection Settings ====================

	/** How far can see the player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Detection")
	float SightRange = 1000.0f;

	/** Field of view angle (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Detection")
	float SightAngle = 90.0f;

	/** How far can hear the player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Detection")
	float HearingRange = 500.0f;

	/** How long to chase after losing sight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Detection")
	float ChaseMemoryDuration = 5.0f;

	// ==================== Animations ====================

	/** Attack montage (basic swing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Animations")
	UAnimMontage* AttackMontage;

	/** Hit reaction montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Animations")
	UAnimMontage* HitReactionMontage;

	/** Death montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Animations")
	UAnimMontage* DeathMontage;

	// ==================== Audio ====================

	/** Sound when spotting player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Audio")
	USoundBase* AlertSound;

	/** Sound when attacking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Audio")
	USoundBase* AttackSound;

	/** Ambient groaning sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Audio")
	TArray<USoundBase*> AmbientSounds;

	/** Time between ambient sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgotten|Audio")
	float AmbientSoundInterval = 8.0f;

	// ==================== State ====================

	/** Current AI state */
	UPROPERTY(BlueprintReadOnly, Category = "Forgotten|State")
	EForgottenState CurrentState = EForgottenState::Idle;

	/** Current target (usually the player) */
	UPROPERTY(BlueprintReadOnly, Category = "Forgotten|State")
	AActor* CurrentTarget;

	/** Last known target location */
	UPROPERTY(BlueprintReadOnly, Category = "Forgotten|State")
	FVector LastKnownTargetLocation;

	// ==================== Functions ====================

	/** Attempt to attack the target */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	void TryAttack();

	/** Called when attack animation reaches damage point */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	void OnAttackHit();

	/** Called when attack animation ends */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	void OnAttackEnd();

	/** Check if can see the target */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	bool CanSeeTarget(AActor* Target) const;

	/** Check if target is in attack range */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	bool IsInAttackRange() const;

	/** Set the current state */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	void SetState(EForgottenState NewState);

	/** Get distance to current target */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	float GetDistanceToTarget() const;

	/** Alert this enemy to a location (for hearing) */
	UFUNCTION(BlueprintCallable, Category = "Forgotten")
	void AlertToLocation(FVector Location);

	/** Override to forward damage to HealthComponent */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// Health component callbacks
	UFUNCTION()
	void OnTakeDamage(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void OnDeath(AActor* KilledBy, AController* InstigatorController);

	// State updates
	void UpdateIdle(float DeltaTime);
	void UpdateChasing(float DeltaTime);
	void UpdateAttacking(float DeltaTime);
	void UpdateStaggered(float DeltaTime);

	// Helpers
	void LookForTarget();
	void MoveTowardTarget(float DeltaTime);
	void PlayAmbientSound();
	APawn* GetPlayerPawn() const;

private:
	// Timers
	float AttackCooldownTimer = 0.0f;
	float StaggerTimer = 0.0f;
	float ChaseMemoryTimer = 0.0f;
	float AmbientSoundTimer = 0.0f;

	// Timer handles
	FTimerHandle AttackTimerHandle;

	// State flags
	bool bIsAttacking = false;
	bool bIsDead = false;

	// Attack timer callbacks
	void OnAttackTimerHit();
	void OnAttackTimerEnd();
};
