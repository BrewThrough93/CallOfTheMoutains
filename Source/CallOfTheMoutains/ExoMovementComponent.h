// CallOfTheMoutains - Exo-Suit Movement Component
// Handles enhanced movement: side-step dodge, slide, double jump, ledge grab/mantle

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExoMovementComponent.generated.h"

class UHealthComponent;
class UCharacterMovementComponent;
class UAnimMontage;
class UAnimInstance;
class UCapsuleComponent;
class USpringArmComponent;

// Movement state enum
UENUM(BlueprintType)
enum class EExoMovementState : uint8
{
	None,           // No special movement active
	SideStep,       // Quick sidestep dodge (locked-on)
	Sliding,        // Slide along ground
	DoubleJumping,  // Second jump in air
	LedgeGrabbing,  // Hanging on ledge
	Mantling        // Pulling up from ledge
};

// Dodge direction enum (matches existing)
UENUM(BlueprintType)
enum class EExoDodgeDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExoMovementStateChanged, EExoMovementState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSideStepStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSideStepEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlideStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlideEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoubleJumpExecuted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLedgeGrabbed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMantleStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMantleComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLedgeReleased);

/**
 * Exo Movement Component - Enhanced movement abilities for souls-like combat
 * Features: Side-step dodge (when locked-on), slide, double jump, ledge grab/mantle
 * Integrates with HealthComponent for stamina consumption
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UExoMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExoMovementComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== Side-Step Settings ====================

	/** Distance traveled during side-step */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep")
	float SideStepDistance = 300.0f;

	/** Duration of side-step animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep")
	float SideStepDuration = 0.3f;

	/** Stamina cost for side-step */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep")
	float SideStepStaminaCost = 15.0f;

	/** Cooldown between side-steps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep")
	float SideStepCooldown = 0.15f;

	/** When i-frames start (as fraction of duration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SideStepIFrameStart = 0.0f;

	/** When i-frames end (as fraction of duration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SideStepIFrameEnd = 0.5f;

	/** Side-step left montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep|Animations")
	UAnimMontage* SideStepLeftMontage;

	/** Side-step right montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep|Animations")
	UAnimMontage* SideStepRightMontage;

	/** Side-step back montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep|Animations")
	UAnimMontage* SideStepBackMontage;

	/** Side-step forward montage (dash) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|SideStep|Animations")
	UAnimMontage* SideStepForwardMontage;

	// ==================== Slide Settings ====================

	/** Distance traveled during slide */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Slide")
	float SlideDistance = 800.0f;

	/** Duration of slide */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Slide")
	float SlideDuration = 0.7f;

	/** Stamina cost for slide */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Slide")
	float SlideStaminaCost = 20.0f;

	/** Minimum speed required to initiate slide */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Slide")
	float MinSpeedToSlide = 500.0f;

	/** Cooldown between slides */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Slide")
	float SlideCooldown = 0.3f;

	/** Capsule half-height during slide (for low profile) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Slide")
	float SlideCapsuleHalfHeight = 30.0f;

	/** Slide montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Slide|Animations")
	UAnimMontage* SlideMontage;

	// ==================== Double Jump Settings ====================

	/** Force applied for double jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|DoubleJump")
	float DoubleJumpForce = 600.0f;

	/** Stamina cost for double jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|DoubleJump")
	float DoubleJumpStaminaCost = 25.0f;

	/** Double jump montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|DoubleJump|Animations")
	UAnimMontage* DoubleJumpMontage;

	// ==================== Ledge Grab Settings ====================

	/** Maximum height above character to detect ledge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab")
	float LedgeDetectionHeight = 200.0f;

	/** Forward distance to trace for ledge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab")
	float LedgeDetectionForward = 80.0f;

	/** Minimum ledge depth (to avoid tiny ledges) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab")
	float MinLedgeDepth = 30.0f;

	/** Stamina cost to grab ledge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab")
	float LedgeGrabStaminaCost = 10.0f;

	/** Cooldown after releasing/mantling before can grab again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab")
	float LedgeGrabCooldown = 0.5f;

	/** Stamina cost to mantle up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab")
	float MantleStaminaCost = 15.0f;

	/** Duration of mantle animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab")
	float MantleDuration = 0.6f;

	/** Ledge grab enter montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab|Animations")
	UAnimMontage* LedgeGrabMontage;

	/** Mantle up montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|LedgeGrab|Animations")
	UAnimMontage* MantleMontage;

	// ==================== Debug ====================

	/** Enable visual debug drawing for ledge detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Debug")
	bool bDebugLedgeDetection = true;

	/** Enable debug logging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExoMovement|Debug")
	bool bDebugLogging = true;

	// ==================== Events ====================

	/** Called when exo movement state changes */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnExoMovementStateChanged OnExoMovementStateChanged;

	/** Called when side-step starts */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnSideStepStarted OnSideStepStarted;

	/** Called when side-step ends */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnSideStepEnded OnSideStepEnded;

	/** Called when slide starts */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnSlideStarted OnSlideStarted;

	/** Called when slide ends */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnSlideEnded OnSlideEnded;

	/** Called when double jump is executed */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnDoubleJumpExecuted OnDoubleJumpExecuted;

	/** Called when ledge is grabbed */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnLedgeGrabbed OnLedgeGrabbed;

	/** Called when mantle starts */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnMantleStarted OnMantleStarted;

	/** Called when mantle completes */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnMantleComplete OnMantleComplete;

	/** Called when ledge is released (falling) */
	UPROPERTY(BlueprintAssignable, Category = "ExoMovement|Events")
	FOnLedgeReleased OnLedgeReleased;

	// ==================== State ====================

	/** Current exo movement state */
	UPROPERTY(BlueprintReadOnly, Category = "ExoMovement|State")
	EExoMovementState CurrentState = EExoMovementState::None;

	/** Current side-step/dodge direction */
	UPROPERTY(BlueprintReadOnly, Category = "ExoMovement|State")
	EExoDodgeDirection CurrentDodgeDirection = EExoDodgeDirection::Forward;

	/** Can use double jump (resets on landing) */
	UPROPERTY(BlueprintReadOnly, Category = "ExoMovement|State")
	bool bCanDoubleJump = true;

	/** Is currently invincible (i-frames active) */
	UPROPERTY(BlueprintReadOnly, Category = "ExoMovement|State")
	bool bIsInvincible = false;

	// ==================== Side-Step Functions ====================

	/** Try to execute a side-step in the given direction. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|SideStep")
	bool TrySideStep(EExoDodgeDirection Direction);

	/** Check if can currently side-step */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement|SideStep")
	bool CanSideStep() const;

	/** Get the world direction vector for a dodge direction */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement|SideStep")
	FVector GetDirectionVector(EExoDodgeDirection Direction) const;

	// ==================== Slide Functions ====================

	/** Try to execute a slide. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|Slide")
	bool TrySlide();

	/** Check if can currently slide */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement|Slide")
	bool CanSlide() const;

	/** End slide early (e.g., hit obstacle) */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|Slide")
	void EndSlide();

	// ==================== Double Jump Functions ====================

	/** Try to execute a double jump. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|DoubleJump")
	bool TryDoubleJump();

	/** Check if can currently double jump */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement|DoubleJump")
	bool CanDoubleJump() const;

	/** Reset double jump ability (call on landing) */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|DoubleJump")
	void ResetDoubleJump();

	// ==================== Ledge Grab Functions ====================

	/** Try to grab a ledge. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|LedgeGrab")
	bool TryLedgeGrab();

	/** Check if can currently grab a ledge */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement|LedgeGrab")
	bool CanLedgeGrab() const;

	/** Detect if there's a grabbable ledge. Returns true if found. */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|LedgeGrab")
	bool DetectLedge(FVector& OutLedgeLocation, FVector& OutLedgeNormal);

	/** Try to mantle up from ledge. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|LedgeGrab")
	bool TryMantle();

	/** Release ledge and fall */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement|LedgeGrab")
	void ReleaseLedge();

	// ==================== General Functions ====================

	/** Is currently in any exo movement state? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement")
	bool IsInExoMovement() const { return CurrentState != EExoMovementState::None; }

	/** Is currently in side-step? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement")
	bool IsSideStepping() const { return CurrentState == EExoMovementState::SideStep; }

	/** Is currently sliding? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement")
	bool IsSliding() const { return CurrentState == EExoMovementState::Sliding; }

	/** Is currently grabbing ledge? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement")
	bool IsGrabbingLedge() const { return CurrentState == EExoMovementState::LedgeGrabbing; }

	/** Is currently mantling? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ExoMovement")
	bool IsMantling() const { return CurrentState == EExoMovementState::Mantling; }

	/** Force end any current exo movement state */
	UFUNCTION(BlueprintCallable, Category = "ExoMovement")
	void ForceEndCurrentState();

protected:
	// ==================== Cached References ====================

	UPROPERTY()
	UHealthComponent* HealthComponent;

	UPROPERTY()
	UCharacterMovementComponent* MovementComponent;

	UPROPERTY()
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY()
	ACharacter* OwnerCharacter;

	UPROPERTY()
	USpringArmComponent* SpringArmComponent;

	// Camera state preservation
	bool bCameraLagWasEnabled = false;
	float OriginalCameraLagSpeed = 0.0f;
	FVector OriginalSocketOffset = FVector::ZeroVector;
	float OriginalTargetArmLength = 0.0f;

	// ==================== Internal State ====================

	// Side-step
	FVector SideStepStartLocation;
	FVector SideStepEndLocation;
	float SideStepTimer = 0.0f;
	float SideStepCooldownTimer = 0.0f;

	// Slide
	FVector SlideStartLocation;
	FVector SlideDirection;
	float SlideTimer = 0.0f;
	float SlideCooldownTimer = 0.0f;
	float OriginalCapsuleHalfHeight = 0.0f;

	// Ledge grab
	FVector LedgeLocation;
	FVector LedgeNormal;
	FVector MantleStartLocation;
	FVector MantleTargetLocation;
	float MantleTimer = 0.0f;
	float LedgeGrabCooldownTimer = 0.0f;

	// ==================== Internal Functions ====================

	/** Cache component references */
	void CacheComponents();

	/** Set the current state and broadcast event */
	void SetState(EExoMovementState NewState);

	/** Update side-step movement */
	void UpdateSideStep(float DeltaTime);

	/** End side-step state */
	void EndSideStep();

	/** Update slide movement */
	void UpdateSlide(float DeltaTime);

	/** Update mantle movement */
	void UpdateMantle(float DeltaTime);

	/** End mantle state */
	void EndMantle();

	/** Get animation instance from owner */
	UAnimInstance* GetAnimInstance() const;

	/** Snap to ledge position */
	void SnapToLedge();

	/** Restore capsule size after slide */
	void RestoreCapsuleSize();

	/** Check if movement is blocked during slide */
	bool IsSlideBlocked() const;

	/** Update i-frame state based on current progress */
	void UpdateIFrames(float Progress, float IFrameStart, float IFrameEnd);

	/** Check if character is on ground */
	bool IsOnGround() const;

	/** Check if character is in air */
	bool IsInAir() const;

	/** Preserve camera settings before ledge grab */
	void PreserveCameraState();

	/** Restore camera settings after ledge release/mantle */
	void RestoreCameraState();
};
