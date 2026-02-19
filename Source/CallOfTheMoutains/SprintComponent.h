// CallOfTheMoutains - Sprint Component
// Handles sprinting, stamina consumption, and movement speed modifiers

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SprintComponent.generated.h"

class UHealthComponent;
class UEquipmentComponent;
class UCharacterMovementComponent;
class USpringArmComponent;

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSprintStateChanged, bool, bIsSprinting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSprintExhausted);

/**
 * Sprint Component - Handles sprinting with stamina consumption
 * Integrates with HealthComponent for stamina and EquipmentComponent for weapon state
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API USprintComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USprintComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== Speed Settings ====================

	/** Base walk speed (when not sprinting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Speed")
	float BaseWalkSpeed = 350.0f;

	/** Sprint speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Speed", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float SprintSpeedMultiplier = 2.0f;

	/** Speed bonus when weapons are stowed (multiplier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Speed", meta = (ClampMin = "1.0", ClampMax = "1.5"))
	float WeaponsStowedSpeedBonus = 1.15f;

	/** Speed penalty when weapons are drawn (multiplier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Speed", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float WeaponsDrawnSpeedPenalty = 1.0f;

	/** How quickly speed changes (interpolation speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Speed", meta = (ClampMin = "1.0"))
	float SpeedInterpSpeed = 8.0f;

	// ==================== Stamina Settings ====================

	/** Stamina cost per second while sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Stamina", meta = (ClampMin = "0.0"))
	float SprintStaminaCostPerSecond = 8.0f;

	/** Minimum stamina required to start sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Stamina", meta = (ClampMin = "0.0"))
	float MinStaminaToSprint = 10.0f;

	/** Stamina cost for dodging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Stamina", meta = (ClampMin = "0.0"))
	float DodgeStaminaCost = 20.0f;

	/** Cooldown after exhaustion before sprinting is allowed again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Stamina", meta = (ClampMin = "0.0"))
	float ExhaustionCooldown = 1.0f;

	// ==================== Camera Effects ====================

	/** Enable FOV change when sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Camera")
	bool bSprintFOVEffect = true;

	/** FOV increase when sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Camera", meta = (ClampMin = "0.0", ClampMax = "30.0", EditCondition = "bSprintFOVEffect"))
	float SprintFOVIncrease = 10.0f;

	/** How quickly FOV changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint|Camera", meta = (ClampMin = "1.0", EditCondition = "bSprintFOVEffect"))
	float FOVInterpSpeed = 6.0f;

	// ==================== Events ====================

	/** Called when sprint state changes */
	UPROPERTY(BlueprintAssignable, Category = "Sprint|Events")
	FOnSprintStateChanged OnSprintStateChanged;

	/** Called when stamina is exhausted while sprinting */
	UPROPERTY(BlueprintAssignable, Category = "Sprint|Events")
	FOnSprintExhausted OnSprintExhausted;

	// ==================== State ====================

	/** Is currently sprinting */
	UPROPERTY(BlueprintReadOnly, Category = "Sprint|State")
	bool bIsSprinting = false;

	/** Is exhausted (can't sprint temporarily) */
	UPROPERTY(BlueprintReadOnly, Category = "Sprint|State")
	bool bIsExhausted = false;

	/** Is sprint input being held */
	UPROPERTY(BlueprintReadOnly, Category = "Sprint|State")
	bool bSprintInputHeld = false;

	// ==================== Functions ====================

	/** Start sprinting (call when sprint key pressed) */
	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StartSprint();

	/** Stop sprinting (call when sprint key released) */
	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StopSprint();

	/** Check if can currently sprint */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprint")
	bool CanSprint() const;

	/** Check if can dodge (has enough stamina) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprint")
	bool CanDodge() const;

	/** Consume stamina for dodge. Returns true if successful */
	UFUNCTION(BlueprintCallable, Category = "Sprint")
	bool ConsumeDodgeStamina();

	/** Get current movement speed (based on all modifiers) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprint")
	float GetCurrentMaxSpeed() const;

	/** Get the target speed based on current state */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprint")
	float GetTargetSpeed() const;

	/** Is the player currently moving? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sprint")
	bool IsMoving() const;

protected:
	/** Cached component references */
	UPROPERTY()
	UHealthComponent* HealthComponent;

	UPROPERTY()
	UEquipmentComponent* EquipmentComponent;

	UPROPERTY()
	UCharacterMovementComponent* MovementComponent;

	UPROPERTY()
	APlayerController* PlayerController;

	/** Original FOV (stored on begin play) */
	float OriginalFOV = 90.0f;

	/** Current target FOV */
	float CurrentTargetFOV = 90.0f;

	/** Exhaustion cooldown timer */
	float ExhaustionTimer = 0.0f;

	/** Current interpolated speed */
	float CurrentSpeed = 350.0f;

	/** Cache component references */
	void CacheComponents();

	/** Update sprint state and stamina consumption */
	void UpdateSprint(float DeltaTime);

	/** Update movement speed */
	void UpdateSpeed(float DeltaTime);

	/** Update camera FOV effect */
	void UpdateCameraFOV(float DeltaTime);

	/** Check if player is actually moving (has velocity) */
	bool HasMovementInput() const;

	/** Set sprint state and broadcast event */
	void SetSprintState(bool bNewState);
};
