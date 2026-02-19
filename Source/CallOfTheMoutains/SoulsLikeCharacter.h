// CallOfTheMoutains - Souls-like Character with Third Person Camera and Combat

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ItemTypes.h"
#include "SoulsLikeCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ULockOnComponent;
class UInventoryComponent;
class UEquipmentComponent;
class UInteractionComponent;
class UInputMappingContext;
class UInputAction;
class UInventoryWidget;
class UHotbarWidget;
class UInteractionPromptWidget;
class UPlayerStatsWidget;
class UFaithWidget;
class AItemPickup;
class UHealthComponent;
class UFaithComponent;
class UAnimMontage;
class UExoMovementComponent;

/**
 * Souls-Like Player Character
 * Features:
 * - Third-person camera with lock-on support
 * - Dodge rolling with i-frames
 * - Lock-on targeting system
 */
UCLASS()
class CALLOFTHEMOUTAINS_API ASoulsLikeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASoulsLikeCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	virtual void Jump() override;
	virtual void Landed(const FHitResult& Hit) override;

public:
	virtual void Tick(float DeltaTime) override;

	// ==================== Components ====================

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// Lock-on is handled by SoulsLikePlayerController - use GetLockOnTarget()

	// NOTE: Inventory and Equipment components are on the CONTROLLER, not the character
	// Use GetController()->FindComponentByClass<UInventoryComponent>() to access them

	/** Interaction component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UInteractionComponent* InteractionComponent;

	/** Health and stamina component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	UHealthComponent* HealthComponent;

	/** Faith (currency) component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faith")
	UFaithComponent* FaithComponent;

	/** Exo-suit movement component (double jump, ledge grab, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UExoMovementComponent* ExoMovementComponent;

	// ==================== Hit Reaction ====================

	/** Hit reaction montage - played when taking damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	UAnimMontage* HitReactionMontage;

	/** Duration of stagger after being hit (prevents actions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float HitStaggerDuration = 0.3f;

	/** Is the character currently staggered from a hit */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsStaggered = false;

	// ==================== Input Actions ====================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LockOnAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SwitchTargetAction;

	// Combat uses direct key detection:
	// Left Mouse = Light Attack, Right Mouse = Heavy Attack
	// Q = Guard (hold), C = Stow/Draw Weapons

	// Hotbar uses direct key detection:
	// Arrow keys = use item, Ctrl+Arrow = cycle items
	// I key = toggle inventory

	// ==================== UI Widgets ====================

	/** Hotbar widget class (create Blueprint child of HotbarWidget) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UHotbarWidget> HotbarWidgetClass;

	/** Inventory widget class (create Blueprint child of InventoryWidget) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	/** Active hotbar widget instance (always visible) */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UHotbarWidget* HotbarWidget;

	/** Active inventory widget instance (toggles with I key) */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UInventoryWidget* InventoryWidget;

	/** Interaction prompt widget class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UInteractionPromptWidget> InteractionPromptWidgetClass;

	/** Active interaction prompt widget instance */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UInteractionPromptWidget* InteractionPromptWidget;

	/** Player stats widget class (health/stamina display) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UPlayerStatsWidget> PlayerStatsWidgetClass;

	/** Active player stats widget instance */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UPlayerStatsWidget* PlayerStatsWidget;

	/** Faith widget class (currency display - bottom right) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFaithWidget> FaithWidgetClass;

	/** Active faith widget instance */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UFaithWidget* FaithWidget;

	// ==================== Camera Settings ====================

	/** Base camera distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraDistance = 400.0f;

	/** Camera distance when locked on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float LockedOnCameraDistance = 350.0f;

	/** Camera height offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	FVector CameraOffset = FVector(0.0f, 0.0f, 80.0f);

	/** How fast camera adjusts to lock-on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float CameraLockOnSpeed = 8.0f;

	/** Camera lag speed for smooth following */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float CameraLagSpeed = 10.0f;

	// ==================== Camera Clipping Prevention ====================

	/** Minimum camera distance before mesh starts hiding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Clipping", meta = (ClampMin = "0.0"))
	float MinCameraDistance = 100.0f;

	/** Distance at which mesh is fully hidden */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Clipping", meta = (ClampMin = "0.0"))
	float MeshHideDistance = 50.0f;

	/** Enable mesh hiding when camera clips */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Clipping")
	bool bHideMeshOnCameraClip = true;

	/** Camera probe size (larger = earlier collision detection) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Clipping", meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float CameraProbeSize = 24.0f;

	// ==================== Functions ====================
	// Note: Dodge is handled by SoulsLikePlayerController

	/** Is locked onto a target */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsLockedOn() const;

	/** Get the current lock-on target (from controller) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	AActor* GetLockOnTarget() const;

	/** Get the lock-on look-at location (from controller) */
	FVector GetLockOnTargetLocation() const;

	/** Get current primary weapon type (from controller's EquipmentComponent) - for Animation BP */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapons")
	EWeaponType GetCurrentWeaponType() const;

	/** Get current off-hand weapon type (from controller's EquipmentComponent) - for Animation BP */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapons")
	EWeaponType GetCurrentOffHandType() const;

	/** Check if a weapon is equipped (from controller's EquipmentComponent) */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapons")
	bool HasWeaponEquipped() const;

protected:
	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartDodge(const FInputActionValue& Value);
	void ToggleLockOn(const FInputActionValue& Value);
	void SwitchTarget(const FInputActionValue& Value);

	// Hotbar input handling (direct key detection)
	void HandleHotbarInput();
	void HandleHotbarUp();
	void HandleHotbarRight();
	void HandleHotbarLeft();
	void HandleHotbarDown();
	void ToggleInventory();

	// Key state helpers
	bool IsCtrlHeld() const;

	// Interaction handling
	void HandleInteractionInput();
	void TryInteract();

	// Combat input handling (direct key polling)
	void HandleCombatInput();

	/** Called when interaction prompt should be shown/hidden (from InteractionComponent) */
	UFUNCTION()
	void OnInteractionPromptChanged(bool bShowPrompt, FText PromptText);

	/** Called when player takes damage (from HealthComponent) */
	UFUNCTION()
	void OnTakeDamage(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	/** Called when stagger ends */
	void OnStaggerEnd();

	/** Called when a pickup focus changes (player enters/exits pickup range) */
	UFUNCTION()
	void OnPickupFocusChanged(AItemPickup* Pickup, bool bFocused);

	/** Currently focused pickup (if any) */
	AItemPickup* CurrentFocusedPickup = nullptr;

	// Key state tracking (to detect press, not hold)
	bool bUpArrowWasDown = false;
	bool bDownArrowWasDown = false;
	bool bLeftArrowWasDown = false;
	bool bRightArrowWasDown = false;
	bool bIKeyWasDown = false;
	bool bEKeyWasDown = false;

	// Combat input tracking
	bool bLeftMouseWasDown = false;
	bool bRightMouseWasDown = false;
	bool bQKeyWasDown = false;
	bool bCKeyWasDown = false;

	// Inventory state
	bool bInventoryOpen = false;

	// Camera system
	void UpdateCamera(float DeltaTime);
	void UpdateLockedOnCamera(float DeltaTime);
	void UpdateFreeCamera(float DeltaTime);

	// Exo movement - ledge grab check
	void CheckLedgeGrab();

	// Camera clipping prevention
	void UpdateCameraClipping();

private:
	// Jump held state for ledge grab
	bool bJumpHeld = false;
	// Movement input cache
	FVector2D MovementInput;

	// Camera state
	FRotator TargetCameraRotation;

	// Hit stagger timer
	FTimerHandle StaggerTimerHandle;
};
