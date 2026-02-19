// CallOfTheMoutains - Souls-like Player Controller with Enhanced Input

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "SoulsLikePlayerController.generated.h"

class ULockOnComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class UInventoryComponent;
class UEquipmentComponent;
class UInventoryWidget;
class UHotbarWidget;
class UPlayerStatsWidget;
class UHealthComponent;
class UInteractionPromptWidget;
class AItemPickup;
class USaveGameManager;
class USprintComponent;
class UExoMovementComponent;

/** Dodge direction for animation selection */
UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

/**
 * Souls-Like Player Controller
 * Handles lock-on (hold/press), sprint, dodge input, and camera control
 * Works with any character - just set this as your player controller
 *
 * Lock-On Controls (X key / IA_LockOn):
 * - Hold: Lock onto nearest target (when not locked)
 * - Press: Switch targets (when locked)
 * - Hold: Clear lock-on (when locked)
 *
 * Sprint/Dodge Control (Shift key / IA_Dodge):
 * - Hold: Sprint (while moving, consumes stamina)
 * - Double-tap: Dodge in movement direction (costs stamina)
 */
UCLASS()
class CALLOFTHEMOUTAINS_API ASoulsLikePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASoulsLikePlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ==================== Input Actions ====================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_LockOn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Dodge;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Look;

	/** Crouch/Slide input action */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Crouch;

	// ==================== Components ====================

	/** Lock-on targeting component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	ULockOnComponent* LockOnComponent;

	/** Save game manager component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGame")
	USaveGameManager* SaveGameManager;

	// NOTE: Inventory and Equipment components are on the PAWN, not Controller
	// Access via GetPawn()->FindComponentByClass<UInventoryComponent>()

	// ==================== UI Widgets ====================

	/** Active hotbar widget instance (always visible) */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UHotbarWidget* HotbarWidget;

	/** Active inventory widget instance (toggles with I key) */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UInventoryWidget* InventoryWidget;

	/** Player stats widget (health/stamina display) */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UPlayerStatsWidget* PlayerStatsWidget;

	/** Interaction prompt widget (shows [E] Pick up...) */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UInteractionPromptWidget* InteractionPromptWidget;

	/** Currently focused pickup (player is in range) */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	AItemPickup* CurrentFocusedPickup;

	/** Cached reference to pawn's health component */
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	UHealthComponent* PawnHealthComponent;

	/** Cached reference to pawn's sprint component */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	USprintComponent* PawnSprintComponent;

	/** Cached reference to pawn's exo movement component */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	UExoMovementComponent* PawnExoMovementComponent;

	// ==================== Lock-On Settings ====================

	/** Time threshold to distinguish press from hold (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float HoldThreshold = 0.15f;

	// ==================== Camera Settings ====================

	/** How fast camera rotates to face target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Lock On", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float CameraLockOnSpeed = 8.0f;

	/** Camera distance when locked on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Lock On")
	float LockedOnCameraDistance = 350.0f;

	/** Normal camera distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Lock On")
	float NormalCameraDistance = 400.0f;

	/** How much to look toward target (0 = player, 1 = target) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Lock On", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CameraTargetFocus = 0.4f;

	// ==================== Dodge Settings ====================

	/** Dodge distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "100.0"))
	float DodgeDistance = 600.0f;

	/** Dodge duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.1"))
	float DodgeDuration = 0.5f;

	/** Cooldown between dodges */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.0"))
	float DodgeCooldown = 0.2f;

	/** I-frame start (fraction of dodge) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IFrameStart = 0.05f;

	/** I-frame end (fraction of dodge) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IFrameEnd = 0.6f;

	// ==================== Dodge Animations ====================

	/** Dodge/roll animation (single montage, character rotates to face direction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Animations")
	UAnimMontage* DodgeMontage;

	// ==================== State ====================

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDodging = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsInvincible = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EDodgeDirection LastDodgeDirection = EDodgeDirection::Forward;

	// ==================== Functions ====================

	/** Check if currently locked on */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsLockedOn() const;

	/** Get the current target */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	AActor* GetLockOnTarget() const;

	/** Acquire lock-on to nearest target */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void AcquireLockOn();

	/** Release lock-on */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ReleaseLockOn();

	/** Switch to next target */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SwitchTarget();

	/** Can the character dodge right now */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	bool CanDodge() const;

	/** Start a dodge (call from input) */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void StartDodge();

	/** Execute dodge in direction */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void ExecuteDodge(FVector Direction);

	/** Get dodge direction based on current input */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	FVector GetDodgeDirection() const;

	/** Get dodge direction enum based on input */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	EDodgeDirection GetDodgeDirectionEnum() const;

	// ==================== Delegates ====================

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDodgeStarted, EDodgeDirection, Direction);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeEnded);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIFrameStateChanged, bool, bInvincible);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockOnChanged, AActor*, Target);

	UPROPERTY(BlueprintAssignable, Category = "Dodge")
	FOnDodgeStarted OnDodgeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dodge")
	FOnDodgeEnded OnDodgeEnded;

	UPROPERTY(BlueprintAssignable, Category = "Dodge")
	FOnIFrameStateChanged OnIFrameStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnLockOnChanged OnLockOnChanged;

protected:
	// Enhanced Input handlers
	void OnLockOnStarted(const FInputActionValue& Value);
	void OnLockOnCompleted(const FInputActionValue& Value);
	void OnDodgeKeyPressed(const FInputActionValue& Value);
	void OnDodgeKeyReleased(const FInputActionValue& Value);
	void OnMoveInput(const FInputActionValue& Value);
	void OnLookInput(const FInputActionValue& Value);

	// Camera
	void UpdateLockedOnCamera(float DeltaTime);
	void UpdateCameraDistance(float DeltaTime);

	// Dodge
	void UpdateDodge(float DeltaTime);
	void EndDodge();

	// Helpers
	USpringArmComponent* FindSpringArm() const;

	// Hotbar/Inventory input (direct key detection)
	void HandleHotbarInput();
	void HandleHotbarUp();
	void HandleHotbarRight();
	void HandleHotbarLeft();
	void HandleHotbarDown();
	void ToggleInventory();
	bool IsCtrlHeld() const;

	// Widget creation (delayed to ensure viewport is ready)
	void CreateInventoryWidgets();
	void CreateStatsWidget();
	void CreateInteractionPromptWidget();

	// Pickup/Interaction handling
	void HandlePickupDetection();
	void HandleInteractionInput();
	void TryPickupItem();

	/** Called when pickup focus changes */
	UFUNCTION()
	void OnPickupFocusChanged(AItemPickup* Pickup, bool bFocused);

	/** Called when lock-on is lost (target died, went out of range, etc.) */
	UFUNCTION()
	void OnLockOnLostCallback(AActor* LostTarget);

	// Debug input handling
	void HandleDebugInput();

	// Combat input handling (LMB, RMB, Q, C)
	void HandleCombatInput();

	// Sprint input handling (Shift hold = sprint)
	void HandleSprintInput();

	// Crouch/Slide input handlers
	void OnCrouchPressed(const FInputActionValue& Value);
	void OnCrouchReleased(const FInputActionValue& Value);

	// Exo movement - jump/ledge grab handling (works with any character)
	void HandleExoMovementInput();
	void CheckLedgeGrab();
	void HandleDoubleJump();

private:
	// Combat input key state tracking
	bool bLeftMouseWasDown = false;
	bool bRightMouseWasDown = false;
	bool bQKeyWasDown = false;
	bool bCKeyWasDown = false;

private:
	UPROPERTY()
	USpringArmComponent* CachedSpringArm;

	// Input state
	FVector2D MoveInput = FVector2D::ZeroVector;

	// Lock-on state
	float LockOnHoldTime = 0.0f;
	bool bLockOnHeld = false;
	bool bLockOnTriggeredThisHold = false;

	// Dodge state - double tap shift detection
	float DodgeTimer = 0.0f;
	float DodgeCooldownTimer = 0.0f;
	float DoubleTapWindow = 0.3f; // Time window for double tap shift

	// Double-tap shift tracking
	float LastShiftTapTime = -1.0f;
	bool bShiftWasDown = false;

	// Jump tracking for ledge grab
	bool bJumpHeld = false;

	FVector DodgeStartLocation;
	FVector DodgeEndLocation;
	FTimerHandle DodgeTimerHandle;

	// Original settings
	bool bOriginalOrientToMovement = true;

	// Hotbar key state tracking (detect press, not hold)
	bool bUpArrowWasDown = false;
	bool bDownArrowWasDown = false;
	bool bLeftArrowWasDown = false;
	bool bRightArrowWasDown = false;
	bool bIKeyWasDown = false;
	bool bEKeyWasDown = false;
	bool bInventoryOpen = false;
};
