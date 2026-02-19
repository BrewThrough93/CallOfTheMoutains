// CallOfTheMoutains - Health Component for player and AI

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "HealthComponent.generated.h"

class UFloatingHealthBar;
class UTargetableComponent;

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth, float, Delta, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageReceived, float, Damage, AActor*, DamageCauser, AController*, InstigatorController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeath, AActor*, KilledBy, AController*, InstigatorController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRevive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina, float, Delta);

/**
 * Health Component - Manages health, damage, and death for any actor
 * Use on player characters, enemies, destructibles, etc.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ==================== Configuration ====================

	/** Maximum health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	/** Starting health (if 0, uses MaxHealth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0"))
	float StartingHealth = 0.0f;

	/** Can this actor be damaged? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bCanBeDamaged = true;

	/** Is this actor invincible? (takes damage but can't die) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bInvincible = false;

	/** Defense/armor - reduces incoming damage by this flat amount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0"))
	float Defense = 0.0f;

	/** Damage multiplier (1.0 = normal, 0.5 = half damage, 2.0 = double damage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	// ==================== Stamina Configuration ====================

	/** Maximum stamina */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "1.0"))
	float MaxStamina = 150.0f;

	/** Starting stamina (if 0, uses MaxStamina) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StartingStamina = 0.0f;

	/** Stamina regeneration rate per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaRegenRate = 20.0f;

	/** Delay before stamina starts regenerating after use (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaRegenDelay = 1.0f;

	/** Is stamina regeneration enabled? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	bool bStaminaRegenEnabled = true;

	// ==================== Floating Health Bar ====================

	/** Show a floating health bar above this actor (for enemies/NPCs) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar")
	bool bShowFloatingHealthBar = false;

	/** Is this a boss? Bosses are excluded from floating health bars (they have custom UI) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar"))
	bool bIsBoss = false;

	/** Only show health bar when player is locked onto this enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar && !bIsBoss"))
	bool bOnlyShowWhenLockedOn = true;

	/** Offset from actor origin for the floating health bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar && !bIsBoss"))
	FVector FloatingBarOffset = FVector(0.0f, 0.0f, 100.0f);

	/** Size of the floating health bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar && !bIsBoss"))
	FVector2D FloatingBarSize = FVector2D(120.0f, 8.0f);

	/** Draw size for the widget component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar && !bIsBoss"))
	FVector2D FloatingBarDrawSize = FVector2D(150.0f, 20.0f);

	/** Always face camera? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar && !bIsBoss"))
	bool bFloatingBarFaceCamera = true;

	/** Hide bar when at full health? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar && !bIsBoss"))
	bool bHideBarAtFullHealth = false;

	/** Delay before hiding bar after reaching full health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Health Bar", meta = (EditCondition = "bShowFloatingHealthBar && !bIsBoss && bHideBarAtFullHealth", ClampMin = "0.0"))
	float HideBarDelay = 3.0f;

	// ==================== Death/Ragdoll Configuration ====================

	/** Enable ragdoll physics on death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bRagdollOnDeath = true;

	/** Stop AI behavior on death (disables AI controller) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bStopAIOnDeath = true;

	/** Disable collision on death (prevents blocking other actors) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bDisableCollisionOnDeath = false;

	/** Destroy actor after death delay (0 = never destroy) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death", meta = (ClampMin = "0.0"))
	float DestroyAfterDeathDelay = 0.0f;

	/** Apply impulse to ragdoll in direction of damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bApplyDeathImpulse = true;

	/** Strength of death impulse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death", meta = (EditCondition = "bApplyDeathImpulse", ClampMin = "0.0"))
	float DeathImpulseStrength = 5000.0f;

	// ==================== Events ====================

	/** Called when health changes (damage or heal) */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChanged OnHealthChanged;

	/** Called when damage is received (before death check) */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnDamageReceived OnDamageReceived;

	/** Called when health reaches zero */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnDeath OnDeath;

	/** Called when revived from death */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnRevive OnRevive;

	/** Called when stamina changes */
	UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
	FOnStaminaChanged OnStaminaChanged;

	// ==================== State ====================

	/** Current health */
	UPROPERTY(BlueprintReadOnly, Category = "Health|State")
	float CurrentHealth;

	/** Is this actor dead? */
	UPROPERTY(BlueprintReadOnly, Category = "Health|State")
	bool bIsDead = false;

	/** Current stamina */
	UPROPERTY(BlueprintReadOnly, Category = "Stamina|State")
	float CurrentStamina;

	// ==================== Functions ====================

	/** Apply damage to this actor. Returns actual damage dealt. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage(float Damage, AActor* DamageCauser = nullptr, AController* InstigatorController = nullptr);

	/** Heal this actor. Returns actual amount healed. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float Heal(float Amount);

	/** Heal to full health */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void HealToFull();

	/** Kill this actor instantly */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Kill(AActor* Killer = nullptr, AController* InstigatorController = nullptr);

	/** Revive this actor with specified health (default: full) */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Revive(float HealthAmount = 0.0f);

	/** Set health directly (clamped to 0-MaxHealth) */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealth(float NewHealth);

	/** Set max health (optionally scale current health proportionally) */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bScaleCurrentHealth = false);

	/** Get current health */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetHealth() const { return CurrentHealth; }

	/** Get max health */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	/** Get health as percentage (0.0 - 1.0) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetHealthPercent() const;

	/** Is this actor dead? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	/** Is this actor alive? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool IsAlive() const { return !bIsDead; }

	/** Is health at maximum? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool IsFullHealth() const { return CurrentHealth >= MaxHealth; }

	// ==================== Stamina Functions ====================

	/** Use stamina. Returns true if enough stamina was available. */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool UseStamina(float Amount);

	/** Restore stamina. Returns actual amount restored. */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	float RestoreStamina(float Amount);

	/** Restore stamina to full */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void RestoreStaminaToFull();

	/** Check if enough stamina is available */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool HasStamina(float Amount) const { return CurrentStamina >= Amount; }

	/** Get current stamina */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	float GetStamina() const { return CurrentStamina; }

	/** Get max stamina */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	/** Get stamina as percentage (0.0 - 1.0) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	float GetStaminaPercent() const;

	/** Is stamina at maximum? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool IsFullStamina() const { return CurrentStamina >= MaxStamina; }

	/** Set stamina regen enabled/disabled */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetStaminaRegenEnabled(bool bEnabled) { bStaminaRegenEnabled = bEnabled; }

	/** Set stamina directly (clamped to 0-MaxStamina) */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetStamina(float NewStamina);

protected:
	/** Called when health reaches zero - handles death logic */
	virtual void HandleDeath(AActor* Killer, AController* InstigatorController);

	/** Calculate actual damage after defense and multipliers */
	virtual float CalculateDamage(float RawDamage) const;

	/** Timer for stamina regeneration delay */
	FTimerHandle StaminaRegenTimerHandle;

	/** Start stamina regeneration after delay */
	void StartStaminaRegen();

	/** Update stamina regeneration (called by tick) */
	void UpdateStaminaRegen(float DeltaTime);

	/** Is stamina currently regenerating? */
	bool bIsRegeneratingStamina = false;

	// ==================== Floating Health Bar ====================

	/** The widget component for floating health bar */
	UPROPERTY()
	UWidgetComponent* FloatingHealthBarComponent;

	/** Reference to the health bar widget */
	UPROPERTY()
	UFloatingHealthBar* FloatingHealthBarWidget;

	/** Create the floating health bar widget component */
	void CreateFloatingHealthBar();

	/** Update the floating health bar display */
	void UpdateFloatingHealthBar();

	/** Timer handle for hiding bar at full health */
	FTimerHandle HideBarTimerHandle;

	/** Timer handle for destroying actor after death */
	FTimerHandle DestroyAfterDeathTimerHandle;

	/** Hide the floating bar (called after delay) */
	void HideFloatingBar();

	/** Show the floating bar */
	void ShowFloatingBar();

	/** Enable ragdoll physics on the owner's mesh */
	void EnableRagdoll();

	/** Stop AI controller behavior */
	void StopAIBehavior();

	/** Destroy owner actor (called after death delay) */
	void DestroyOwnerActor();

	/** Cached last damage causer for death impulse direction */
	UPROPERTY()
	AActor* LastDamageCauser = nullptr;

	/** Cached reference to targetable component (for lock-on events) */
	UPROPERTY()
	UTargetableComponent* CachedTargetableComponent;

	/** Called when lock-on state changes */
	UFUNCTION()
	void OnLockOnStateChanged(bool bIsLockedOn);

	/** Is this actor currently locked on by player? */
	bool bIsCurrentlyLockedOn = false;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
