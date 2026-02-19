// CallOfTheMoutains - Lock On Component for Souls-like targeting

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

class UTargetableComponent;

/**
 * Lock On Component - Handles Souls-like target lock-on
 * Add to player character to enable lock-on targeting
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ==================== Settings ====================

	/** Maximum distance to search for targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Settings", meta = (ClampMin = "500.0"))
	float MaxLockOnDistance = 2000.0f;

	/** Maximum angle from forward to search for targets (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Settings", meta = (ClampMin = "15.0", ClampMax = "180.0"))
	float MaxLockOnAngle = 60.0f;

	/** Distance at which lock-on breaks automatically */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Settings", meta = (ClampMin = "500.0"))
	float BreakLockDistance = 2500.0f;

	/** Automatically switch to a nearby target when current target dies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Settings")
	bool bAutoRetargetOnDeath = true;

	/** How quickly the camera rotates to face the target (0 = instant) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Settings", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float LockOnRotationSpeed = 10.0f;

	/** Offset for the camera look-at point (added to target location) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Settings")
	FVector CameraTargetOffset = FVector(0.0f, 0.0f, 50.0f);

	/** Enable debug drawing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Debug")
	bool bDebugDraw = false;

	// ==================== Functions ====================

	/** Toggle lock-on (finds nearest target or releases current) */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	void ToggleLockOn();

	/** Lock onto a specific target */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	bool LockOnToTarget(AActor* Target);

	/** Release current lock-on */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	void ReleaseLockOn();

	/** Switch to next target (direction: 1 = right, -1 = left) */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	void SwitchTarget(float Direction);

	/** Check if currently locked on */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	bool IsLockedOn() const { return CurrentTarget != nullptr; }

	/** Get current target actor */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	/** Get current target component */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	UTargetableComponent* GetCurrentTargetComponent() const { return CurrentTargetComponent; }

	/** Get the world location to look at */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	FVector GetTargetLookAtLocation() const;

	/** Get rotation to face the current target */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	FRotator GetRotationToTarget() const;

	/** Set the owner actor for traces (use when component is on controller but pawn should be used) */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	void SetOwnerActor(AActor* NewOwner);

	/** Get the owner actor used for traces */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	AActor* GetTraceOwner() const;

	// ==================== Delegates ====================

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockOnStateChanged, AActor*, Target);

	/** Called when lock-on is acquired */
	UPROPERTY(BlueprintAssignable, Category = "Lock On")
	FOnLockOnStateChanged OnLockOnAcquired;

	/** Called when lock-on is lost */
	UPROPERTY(BlueprintAssignable, Category = "Lock On")
	FOnLockOnStateChanged OnLockOnLost;

private:
	UPROPERTY()
	AActor* CurrentTarget = nullptr;

	UPROPERTY()
	UTargetableComponent* CurrentTargetComponent = nullptr;

	UPROPERTY()
	AActor* OverrideOwner = nullptr;

	/** Find the best target to lock onto */
	AActor* FindBestTarget() const;

	/** Find all valid targets in range */
	TArray<AActor*> FindAllTargetsInRange() const;

	/** Check if target is still valid */
	bool IsTargetValid() const;

	/** Check if target is dead (for auto-retarget logic) */
	bool IsTargetDead() const;

	/** Try to switch to a nearby target. Returns true if switched successfully. */
	bool TrySwitchToNearbyTarget();

	/** Get the angle to a target from player's forward vector */
	float GetAngleToTarget(AActor* Target) const;

	/** Check line of sight to target */
	bool HasLineOfSightTo(AActor* Target) const;
};
