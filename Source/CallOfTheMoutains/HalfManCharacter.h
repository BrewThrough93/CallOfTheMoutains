// CallOfTheMoutains - Half Man Enemy Character
// A zombie-like enemy that plays dead until player approaches, then awakens with gore effects

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HalfManCharacter.generated.h"

class UHealthComponent;
class UFootstepComponent;
class UMeleeTraceComponent;
class UTargetableComponent;
class UGoreTrailComponent;
class USphereComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class ABileProjectile;

/**
 * Half Man states - from fake dead to combat
 */
UENUM(BlueprintType)
enum class EHalfManState : uint8
{
	FakeDead,		// Initial state - appears as corpse, not targetable
	Awakening,		// Playing separation/rising animation
	Idle,			// Standing, looking for player
	Chasing,		// Pursuing player
	MeleeAttack,	// Close range melee attack
	RangedAttack,	// Bile/vomit projectile attack
	Staggered,		// Hit reaction from damage
	Dead			// Actually dead
};

/**
 * Half Man - A deceptive enemy that plays dead until the player approaches
 *
 * Features:
 * - Fake dead state that makes players paranoid about corpses
 * - Proximity-triggered awakening with dramatic gore effects
 * - Both melee and ranged (bile) attacks
 * - Leaves blood/gore trail while moving
 */
UCLASS()
class CALLOFTHEMOUTAINS_API AHalfManCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHalfManCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ==================== Components ====================

	/** Health management component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	/** Footstep sounds while moving */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFootstepComponent* FootstepComponent;

	/** Melee attack hit detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UMeleeTraceComponent* MeleeTraceComponent;

	/** Lock-on targeting support */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTargetableComponent* TargetableComponent;

	/** Proximity trigger for waking up */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* WakeTriggerSphere;

	/** Blood/gore trail while moving */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UGoreTrailComponent* GoreTrailComponent;

	// ==================== Wake Settings ====================

	/** Range at which player presence triggers awakening */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Wake")
	float WakeRange = 400.0f;

	/** Animation to play when awakening (separation effect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Wake")
	UAnimMontage* AwakeningMontage;

	/** Sound to play when awakening */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Wake")
	USoundBase* AwakeningSound;

	/** Gore particle effect for awakening */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Wake")
	UNiagaraSystem* AwakeningGoreEffect;

	/** Socket to spawn awakening gore effect at */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Wake")
	FName AwakeningEffectSocket = TEXT("spine_01");

	// ==================== Detection Settings ====================

	/** How far the Half Man can see targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Detection")
	float SightRange = 1500.0f;

	/** Field of view angle for sight detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Detection")
	float SightAngle = 120.0f;

	/** Movement speed while chasing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Movement")
	float ChaseSpeed = 300.0f;

	// ==================== Melee Combat Settings ====================

	/** Range for melee attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Melee")
	float MeleeRange = 150.0f;

	/** Damage dealt by melee attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Melee")
	float MeleeDamage = 25.0f;

	/** Cooldown between melee attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Melee")
	float MeleeAttackCooldown = 2.0f;

	/** Animation for melee attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Melee")
	UAnimMontage* MeleeAttackMontage;

	/** Sound for melee attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Melee")
	USoundBase* MeleeAttackSound;

	// ==================== Ranged Combat Settings ====================

	/** Maximum range for ranged attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged")
	float RangedRange = 800.0f;

	/** Minimum range for ranged attacks (use melee if closer) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged")
	float RangedMinRange = 200.0f;

	/** Cooldown between ranged attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged")
	float RangedAttackCooldown = 5.0f;

	/** Chance to use ranged attack when in range (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RangedAttackChance = 0.6f;

	/** Animation for ranged attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged")
	UAnimMontage* RangedAttackMontage;

	/** Sound for ranged attack windup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged")
	USoundBase* RangedAttackSound;

	/** Projectile class to spawn for ranged attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged")
	TSubclassOf<ABileProjectile> BileProjectileClass;

	/** Socket to spawn bile projectile from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat|Ranged")
	FName ProjectileSpawnSocket = TEXT("head");

	// ==================== Stagger Settings ====================

	/** How long stagger lasts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat")
	float StaggerDuration = 0.5f;

	/** Animation for hit reaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat")
	UAnimMontage* HitReactionMontage;

	/** Animation for death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat")
	UAnimMontage* DeathMontage;

	/** Sound for taking damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat")
	USoundBase* HitSound;

	/** Sound for death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HalfMan|Combat")
	USoundBase* DeathSound;

	// ==================== Current State ====================

	/** Current AI state */
	UPROPERTY(BlueprintReadOnly, Category = "HalfMan|State")
	EHalfManState CurrentState = EHalfManState::FakeDead;

	/** Current target being pursued */
	UPROPERTY(BlueprintReadOnly, Category = "HalfMan|State")
	AActor* CurrentTarget = nullptr;

	/** Has this Half Man awakened yet? */
	UPROPERTY(BlueprintReadOnly, Category = "HalfMan|State")
	bool bHasAwakened = false;

	// ==================== Public Functions ====================

	/** Force the Half Man to wake up */
	UFUNCTION(BlueprintCallable, Category = "HalfMan")
	void WakeUp();

	/** Attempt a melee attack */
	UFUNCTION(BlueprintCallable, Category = "HalfMan")
	void TryMeleeAttack();

	/** Attempt a ranged attack */
	UFUNCTION(BlueprintCallable, Category = "HalfMan")
	void TryRangedAttack();

	/** Spawn the bile projectile (called from anim notify) */
	UFUNCTION(BlueprintCallable, Category = "HalfMan")
	void SpawnBileProjectile();

	/** Get current state */
	UFUNCTION(BlueprintCallable, Category = "HalfMan")
	EHalfManState GetCurrentState() const { return CurrentState; }

	/** Check if in fake dead state */
	UFUNCTION(BlueprintCallable, Category = "HalfMan")
	bool IsFakeDead() const { return CurrentState == EHalfManState::FakeDead; }

	/** Check if currently attacking */
	UFUNCTION(BlueprintCallable, Category = "HalfMan")
	bool IsAttacking() const { return CurrentState == EHalfManState::MeleeAttack || CurrentState == EHalfManState::RangedAttack; }

protected:
	// ==================== State Update Functions ====================

	void UpdateFakeDead(float DeltaTime);
	void UpdateAwakening(float DeltaTime);
	void UpdateIdle(float DeltaTime);
	void UpdateChasing(float DeltaTime);
	void UpdateMeleeAttack(float DeltaTime);
	void UpdateRangedAttack(float DeltaTime);
	void UpdateStaggered(float DeltaTime);

	// ==================== State Transitions ====================

	void SetState(EHalfManState NewState);
	void OnStateEnter(EHalfManState NewState);
	void OnStateExit(EHalfManState OldState);

	// ==================== Detection ====================

	bool CanSeeTarget(AActor* Target) const;
	void LookForTarget();
	APawn* GetPlayerPawn() const;
	float GetDistanceToTarget() const;

	// ==================== Combat Helpers ====================

	void OnMeleeAttackHit();
	void OnMeleeAttackEnd();
	void OnRangedAttackEnd();
	void FaceTarget();

	// ==================== Health Callbacks ====================

	UFUNCTION()
	void OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void OnDeath(AActor* KilledBy, AController* InstigatorController);

	// ==================== Wake Trigger ====================

	UFUNCTION()
	void OnWakeTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	// ==================== VFX/SFX ====================

	void PlayAwakeningEffects();
	void SpawnGoreEffect(FVector Location);

private:
	// Timers
	float MeleeCooldownTimer = 0.0f;
	float RangedCooldownTimer = 0.0f;
	float StaggerTimer = 0.0f;
	float AwakeningTimer = 0.0f;
	float PostAwakeningGraceTimer = 0.0f;  // Grace period after awakening to skip sight checks

	// State flags
	bool bIsAttacking = false;
	bool bIsDead = false;

	// Timer handles
	FTimerHandle MeleeHitTimerHandle;
	FTimerHandle AttackEndTimerHandle;
};
