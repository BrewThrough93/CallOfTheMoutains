// CallOfTheMoutains - Souls-like Character with Third Person Camera and Combat

#include "SoulsLikeCharacter.h"
#include "SoulsLikePlayerController.h"
#include "LockOnComponent.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "InteractionComponent.h"
#include "ItemPickup.h"
#include "InventoryWidget.h"
#include "HotbarWidget.h"
#include "InteractionPromptWidget.h"
#include "PlayerStatsWidget.h"
#include "HealthComponent.h"
#include "ItemTypes.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ExoMovementComponent.h"

ASoulsLikeCharacter::ASoulsLikeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Enable capsule overlap events for item pickup detection
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// Load input actions from /Game/Input/Actions/
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(TEXT("/Game/Input/IMC_Default"));
	if (MappingContextFinder.Succeeded())
	{
		DefaultMappingContext = MappingContextFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/Actions/IA_Move"));
	if (MoveActionFinder.Succeeded())
	{
		MoveAction = MoveActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/Input/Actions/IA_Look"));
	if (LookActionFinder.Succeeded())
	{
		LookAction = LookActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionFinder(TEXT("/Game/Input/Actions/IA_Jump"));
	if (JumpActionFinder.Succeeded())
	{
		JumpAction = JumpActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DodgeActionFinder(TEXT("/Game/Input/Actions/IA_Dodge"));
	if (DodgeActionFinder.Succeeded())
	{
		DodgeAction = DodgeActionFinder.Object;
	}

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	// Don't rotate character with controller
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = CameraLagSpeed;
	CameraBoom->SocketOffset = CameraOffset;

	// Create follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Lock-on is handled by SoulsLikePlayerController
	// Character gets LockOnComponent from Controller via GetLockOnComponent()

	// NOTE: Inventory and Equipment components are on the CONTROLLER, not here

	// Create interaction component
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));

	// Create health component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// Create exo movement component
	ExoMovementComponent = CreateDefaultSubobject<UExoMovementComponent>(TEXT("ExoMovementComponent"));
}

void ASoulsLikeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}

		// Get components from Controller
		UInventoryComponent* InvComp = PlayerController->FindComponentByClass<UInventoryComponent>();
		UEquipmentComponent* EquipComp = PlayerController->FindComponentByClass<UEquipmentComponent>();

		// Create Hotbar Widget (always visible)
		TSubclassOf<UHotbarWidget> HotbarClass = HotbarWidgetClass;
		if (!HotbarClass)
		{
			HotbarClass = UHotbarWidget::StaticClass();
		}
		HotbarWidget = CreateWidget<UHotbarWidget>(PlayerController, HotbarClass);
		if (HotbarWidget)
		{
			HotbarWidget->AddToViewport(0);
			HotbarWidget->InitializeHotbar(EquipComp, InvComp);
		}

		// Create Inventory Widget (hidden by default)
		TSubclassOf<UInventoryWidget> InvClass = InventoryWidgetClass;
		if (!InvClass)
		{
			InvClass = UInventoryWidget::StaticClass();
		}
		InventoryWidget = CreateWidget<UInventoryWidget>(PlayerController, InvClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport(10);
			InventoryWidget->InitializeInventory(InvComp, EquipComp);
			InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		// Create Interaction Prompt Widget (hidden by default)
		TSubclassOf<UInteractionPromptWidget> PromptClass = InteractionPromptWidgetClass;
		if (!PromptClass)
		{
			PromptClass = UInteractionPromptWidget::StaticClass();
		}
		InteractionPromptWidget = CreateWidget<UInteractionPromptWidget>(PlayerController, PromptClass);
		if (InteractionPromptWidget)
		{
			InteractionPromptWidget->AddToViewport(5);
		}

		// Create Player Stats Widget (health/stamina display - top right)
		TSubclassOf<UPlayerStatsWidget> StatsClass = PlayerStatsWidgetClass;
		if (!StatsClass)
		{
			StatsClass = UPlayerStatsWidget::StaticClass();
		}
		PlayerStatsWidget = CreateWidget<UPlayerStatsWidget>(PlayerController, StatsClass);
		if (PlayerStatsWidget)
		{
			PlayerStatsWidget->AddToViewport(1);
			PlayerStatsWidget->InitializeStats(HealthComponent);
		}
	}

	// Bind to interaction component events
	if (InteractionComponent)
	{
		InteractionComponent->OnInteractionPromptChanged.AddDynamic(this, &ASoulsLikeCharacter::OnInteractionPromptChanged);
	}

	// Bind to health component for hit reactions
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ASoulsLikeCharacter::OnTakeDamage);
	}
}

void ASoulsLikeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Movement
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASoulsLikeCharacter::Move);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASoulsLikeCharacter::Move);
		}

		// Camera look
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASoulsLikeCharacter::Look);
		}

		// Jump - use our override for double jump and ledge grab
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASoulsLikeCharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		// Note: bJumpHeld tracking is done via direct key polling in Tick

		// Combat (LMB, RMB, Q, C) uses direct key polling in Tick -> HandleCombatInput()
		// Hotbar (Arrow keys, I) uses direct key polling in Tick -> HandleHotbarInput()
		// Lock-on is handled by SoulsLikePlayerController
	}
}

void ASoulsLikeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update camera
	UpdateCamera(DeltaTime);

	// Handle hotbar input (Arrow keys + I key)
	HandleHotbarInput();

	// Handle interaction input (E key)
	HandleInteractionInput();

	// Handle combat input (LMB, RMB, Q, C)
	HandleCombatInput();

	// Track jump held for ledge grab (direct key polling)
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		bJumpHeld = PC->IsInputKeyDown(EKeys::SpaceBar);
	}

	// Check for ledge grab while holding jump in air
	CheckLedgeGrab();

	// Note: Dodge is handled by SoulsLikePlayerController
}

void ASoulsLikeCharacter::Move(const FInputActionValue& Value)
{
	MovementInput = Value.Get<FVector2D>();

	// If grabbing ledge, handle ledge-specific input
	if (ExoMovementComponent && ExoMovementComponent->IsGrabbingLedge())
	{
		// Backward input releases ledge
		if (MovementInput.Y < -0.5f)
		{
			ExoMovementComponent->ReleaseLedge();
			UE_LOG(LogTemp, Warning, TEXT("Player: Released ledge (backward input)"));
		}
		// Forward input initiates mantle
		else if (MovementInput.Y > 0.5f)
		{
			ExoMovementComponent->TryMantle();
		}
		return; // Don't process normal movement while on ledge
	}

	// Check if dodging (from controller)
	if (ASoulsLikePlayerController* SoulsController = Cast<ASoulsLikePlayerController>(GetController()))
	{
		if (SoulsController->bIsDodging)
		{
			return;
		}
	}

	if (Controller)
	{
		// Get movement direction based on camera/lock-on
		FRotator ControlRotation;

		if (IsLockedOn() && GetLockOnTarget())
		{
			// When locked on, move relative to target direction
			FVector ToTarget = GetLockOnTarget()->GetActorLocation() - GetActorLocation();
			ToTarget.Z = 0;
			ControlRotation = ToTarget.Rotation();
		}
		else
		{
			// Normal camera-relative movement
			ControlRotation = Controller->GetControlRotation();
		}

		const FRotator YawRotation(0, ControlRotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementInput.Y);
		AddMovementInput(RightDirection, MovementInput.X);
	}
}

void ASoulsLikeCharacter::Look(const FInputActionValue& Value)
{
	// Don't allow free look when locked on
	if (IsLockedOn())
	{
		return;
	}

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASoulsLikeCharacter::StartDodge(const FInputActionValue& Value)
{
	// Dodge is handled by SoulsLikePlayerController
}

void ASoulsLikeCharacter::ToggleLockOn(const FInputActionValue& Value)
{
	// Lock-on is now handled by SoulsLikePlayerController
	// This function kept for Blueprint compatibility but does nothing
}

void ASoulsLikeCharacter::SwitchTarget(const FInputActionValue& Value)
{
	// Target switching is now handled by SoulsLikePlayerController
	// This function kept for Blueprint compatibility but does nothing
}

bool ASoulsLikeCharacter::IsLockedOn() const
{
	if (ASoulsLikePlayerController* SoulsController = Cast<ASoulsLikePlayerController>(GetController()))
	{
		return SoulsController->IsLockedOn();
	}
	return false;
}

AActor* ASoulsLikeCharacter::GetLockOnTarget() const
{
	if (ASoulsLikePlayerController* SoulsController = Cast<ASoulsLikePlayerController>(GetController()))
	{
		return SoulsController->GetLockOnTarget();
	}
	return nullptr;
}

FVector ASoulsLikeCharacter::GetLockOnTargetLocation() const
{
	if (ASoulsLikePlayerController* SoulsController = Cast<ASoulsLikePlayerController>(GetController()))
	{
		if (SoulsController->LockOnComponent)
		{
			return SoulsController->LockOnComponent->GetTargetLookAtLocation();
		}
	}
	return FVector::ZeroVector;
}

EWeaponType ASoulsLikeCharacter::GetCurrentWeaponType() const
{
	if (ASoulsLikePlayerController* SoulsController = Cast<ASoulsLikePlayerController>(GetController()))
	{
		if (UEquipmentComponent* EquipComp = SoulsController->FindComponentByClass<UEquipmentComponent>())
		{
			return EquipComp->CurrentPrimaryWeaponType;
		}
	}
	return EWeaponType::None;
}

EWeaponType ASoulsLikeCharacter::GetCurrentOffHandType() const
{
	if (ASoulsLikePlayerController* SoulsController = Cast<ASoulsLikePlayerController>(GetController()))
	{
		if (UEquipmentComponent* EquipComp = SoulsController->FindComponentByClass<UEquipmentComponent>())
		{
			return EquipComp->CurrentOffHandWeaponType;
		}
	}
	return EWeaponType::None;
}

bool ASoulsLikeCharacter::HasWeaponEquipped() const
{
	return GetCurrentWeaponType() != EWeaponType::None;
}

void ASoulsLikeCharacter::UpdateCamera(float DeltaTime)
{
	if (IsLockedOn())
	{
		UpdateLockedOnCamera(DeltaTime);
	}
	else
	{
		UpdateFreeCamera(DeltaTime);
	}
}

void ASoulsLikeCharacter::UpdateLockedOnCamera(float DeltaTime)
{
	AActor* Target = GetLockOnTarget();
	if (!Target)
	{
		return;
	}

	// Adjust camera distance
	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		LockedOnCameraDistance,
		DeltaTime,
		CameraLockOnSpeed
	);

	// Calculate rotation to look at target
	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector TargetLocation = GetLockOnTargetLocation();

	// We want camera to look at a point between player and target
	FVector PlayerLocation = GetActorLocation();
	FVector MidPoint = FMath::Lerp(PlayerLocation, TargetLocation, 0.3f);
	MidPoint.Z = TargetLocation.Z; // Keep height at target level

	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLocation, MidPoint);

	// Smoothly interpolate camera rotation
	FRotator CurrentRotation = Controller->GetControlRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, LookAtRotation, DeltaTime, CameraLockOnSpeed);

	Controller->SetControlRotation(NewRotation);

	// Rotate character to face target
	FVector ToTarget = TargetLocation - PlayerLocation;
	ToTarget.Z = 0;
	FRotator CharacterRotation = ToTarget.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), CharacterRotation, DeltaTime, 10.0f));
}

void ASoulsLikeCharacter::UpdateFreeCamera(float DeltaTime)
{
	// Return camera to normal distance
	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		CameraDistance,
		DeltaTime,
		CameraLockOnSpeed
	);
}

// ==================== Hotbar Input Handlers ====================
// Direct key detection: Arrow keys = use, Ctrl+Arrow = cycle, I = inventory

bool ASoulsLikeCharacter::IsCtrlHeld() const
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC)
	{
		return PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);
	}
	return false;
}

void ASoulsLikeCharacter::HandleHotbarInput()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;

	// Up Arrow
	bool bUpDown = PC->IsInputKeyDown(EKeys::Up);
	if (bUpDown && !bUpArrowWasDown)
	{
		HandleHotbarUp();
	}
	bUpArrowWasDown = bUpDown;

	// Down Arrow
	bool bDownDown = PC->IsInputKeyDown(EKeys::Down);
	if (bDownDown && !bDownArrowWasDown)
	{
		HandleHotbarDown();
	}
	bDownArrowWasDown = bDownDown;

	// Left Arrow
	bool bLeftDown = PC->IsInputKeyDown(EKeys::Left);
	if (bLeftDown && !bLeftArrowWasDown)
	{
		HandleHotbarLeft();
	}
	bLeftArrowWasDown = bLeftDown;

	// Right Arrow
	bool bRightDown = PC->IsInputKeyDown(EKeys::Right);
	if (bRightDown && !bRightArrowWasDown)
	{
		HandleHotbarRight();
	}
	bRightArrowWasDown = bRightDown;

	// I Key - Toggle Inventory
	bool bIDown = PC->IsInputKeyDown(EKeys::I);
	if (bIDown && !bIKeyWasDown)
	{
		ToggleInventory();
	}
	bIKeyWasDown = bIDown;
}

void ASoulsLikeCharacter::HandleHotbarUp()
{
	UEquipmentComponent* EquipComp = GetController() ? GetController()->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	if (IsCtrlHeld())
	{
		EquipComp->CycleHotbarNext(EHotbarSlot::Consumable);
	}
	else
	{
		EquipComp->UseConsumable();
	}
}

void ASoulsLikeCharacter::HandleHotbarRight()
{
	UEquipmentComponent* EquipComp = GetController() ? GetController()->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	if (IsCtrlHeld())
	{
		EquipComp->CyclePrimaryWeapon();
	}
}

void ASoulsLikeCharacter::HandleHotbarLeft()
{
	UEquipmentComponent* EquipComp = GetController() ? GetController()->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	if (IsCtrlHeld())
	{
		EquipComp->CycleOffHand();
	}
}

void ASoulsLikeCharacter::HandleHotbarDown()
{
	UEquipmentComponent* EquipComp = GetController() ? GetController()->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	if (IsCtrlHeld())
	{
		EquipComp->CycleHotbarNext(EHotbarSlot::Special);
	}
	else
	{
		EquipComp->UseSpecialItem();
	}
}

void ASoulsLikeCharacter::ToggleInventory()
{
	bInventoryOpen = !bInventoryOpen;

	APlayerController* PC = Cast<APlayerController>(Controller);

	// Show/hide inventory widget
	if (InventoryWidget)
	{
		if (bInventoryOpen)
		{
			InventoryWidget->SetVisibility(ESlateVisibility::Visible);
			InventoryWidget->RefreshAll();

			// Set keyboard focus to the widget
			if (PC)
			{
				InventoryWidget->SetKeyboardFocus();
			}
		}
		else
		{
			InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Toggle mouse cursor and input mode
	if (PC)
	{
		if (bInventoryOpen)
		{
			PC->SetShowMouseCursor(false); // Don't need mouse, using keyboard
			// Use GameAndUI so widget can tick and poll input
			FInputModeGameAndUI InputMode;
			if (InventoryWidget)
			{
				InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
			}
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(true);
			PC->SetInputMode(InputMode);
		}
		else
		{
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

// ==================== Interaction Handlers ====================

void ASoulsLikeCharacter::HandleInteractionInput()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;

	// Don't allow interaction while inventory is open
	if (bInventoryOpen) return;

	// E Key - Interact
	bool bEDown = PC->IsInputKeyDown(EKeys::E);
	if (bEDown && !bEKeyWasDown)
	{
		TryInteract();
	}
	bEKeyWasDown = bEDown;
}

void ASoulsLikeCharacter::TryInteract()
{
	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void ASoulsLikeCharacter::OnInteractionPromptChanged(bool bShowPrompt, FText PromptText)
{
	if (!InteractionPromptWidget)
	{
		return;
	}

	if (bShowPrompt)
	{
		InteractionPromptWidget->ShowPrompt(PromptText);
	}
	else
	{
		InteractionPromptWidget->HidePrompt();
	}
}

void ASoulsLikeCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// Check if we overlapped with a pickup
	AItemPickup* Pickup = Cast<AItemPickup>(OtherActor);
	if (Pickup)
	{
		OnPickupFocusChanged(Pickup, true);
	}
}

void ASoulsLikeCharacter::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	// Check if we left a pickup
	AItemPickup* Pickup = Cast<AItemPickup>(OtherActor);
	if (Pickup && Pickup == CurrentFocusedPickup)
	{
		OnPickupFocusChanged(Pickup, false);
	}
}

void ASoulsLikeCharacter::OnPickupFocusChanged(AItemPickup* Pickup, bool bFocused)
{
	if (bFocused)
	{
		CurrentFocusedPickup = Pickup;

		// Show interaction prompt
		if (InteractionPromptWidget && Pickup)
		{
			FText Prompt = Pickup->GetPickupPrompt();
			InteractionPromptWidget->ShowPrompt(Prompt);
		}
	}
	else
	{
		CurrentFocusedPickup = nullptr;

		// Hide interaction prompt
		if (InteractionPromptWidget)
		{
			InteractionPromptWidget->HidePrompt();
		}
	}
}

// ==================== Combat Input (Direct Key Polling) ====================

void ASoulsLikeCharacter::HandleCombatInput()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}

	// Don't process combat while inventory is open
	if (bInventoryOpen) return;

	// Check if dodging
	ASoulsLikePlayerController* SoulsPC = Cast<ASoulsLikePlayerController>(Controller);
	bool bDodging = SoulsPC && SoulsPC->bIsDodging;

	UEquipmentComponent* EquipComp = PC->FindComponentByClass<UEquipmentComponent>();
	if (!EquipComp)
	{
		return;
	}

	// Get input states
	bool bLMB = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	bool bRMB = PC->IsInputKeyDown(EKeys::RightMouseButton);
	bool bQ = PC->IsInputKeyDown(EKeys::Q);
	bool bC = PC->IsInputKeyDown(EKeys::C);

	// Left Mouse Button - Light Attack
	if (bLMB && !bLeftMouseWasDown && !bDodging)
	{
		EquipComp->LightAttack();
	}
	bLeftMouseWasDown = bLMB;

	// Right Mouse Button - Heavy Attack
	if (bRMB && !bRightMouseWasDown && !bDodging)
	{
		EquipComp->HeavyAttack();
	}
	bRightMouseWasDown = bRMB;

	// Q Key - Guard (hold)
	if (bQ && !bQKeyWasDown)
	{
		EquipComp->StartGuard();
	}
	else if (!bQ && bQKeyWasDown)
	{
		EquipComp->StopGuard();
	}
	bQKeyWasDown = bQ;

	// C Key - Stow/Draw Weapons
	if (bC && !bCKeyWasDown && !bDodging)
	{
		EquipComp->ToggleWeaponStow();
	}
	bCKeyWasDown = bC;
}

// ==================== Hit Reaction ====================

void ASoulsLikeCharacter::OnTakeDamage(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	// Only react to damage (negative delta), not healing
	if (Delta >= 0.0f)
	{
		return;
	}

	// Don't react if already dead
	if (HealthComponent && HealthComponent->IsDead())
	{
		return;
	}

	// Check if dodging (has i-frames)
	if (ASoulsLikePlayerController* SoulsController = Cast<ASoulsLikePlayerController>(GetController()))
	{
		if (SoulsController->bIsDodging && SoulsController->bIsInvincible)
		{
			return; // Don't play hit reaction during dodge i-frames
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Player: OnTakeDamage - Health: %.1f/%.1f, Delta: %.1f"), CurrentHealth, MaxHealth, Delta);

	// Enter stagger state
	bIsStaggered = true;

	// Play hit reaction montage
	if (HitReactionMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(HitReactionMontage);
			UE_LOG(LogTemp, Warning, TEXT("Player: Playing hit reaction montage"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Player: No HitReactionMontage set!"));
	}

	// Set timer to end stagger
	GetWorld()->GetTimerManager().SetTimer(
		StaggerTimerHandle,
		this,
		&ASoulsLikeCharacter::OnStaggerEnd,
		HitStaggerDuration,
		false
	);
}

void ASoulsLikeCharacter::OnStaggerEnd()
{
	bIsStaggered = false;
	UE_LOG(LogTemp, Warning, TEXT("Player: Stagger ended"));
}

// ==================== Exo Movement - Jump Overrides ====================

void ASoulsLikeCharacter::Jump()
{
	// Check if we can double jump (already in air and have ExoMovementComponent)
	if (ExoMovementComponent && GetCharacterMovement()->IsFalling())
	{
		// Try double jump
		if (ExoMovementComponent->TryDoubleJump())
		{
			UE_LOG(LogTemp, Warning, TEXT("Player: Double jump executed"));
			return;
		}
	}

	// If on ledge and jump pressed, try to mantle
	if (ExoMovementComponent && ExoMovementComponent->IsGrabbingLedge())
	{
		if (ExoMovementComponent->TryMantle())
		{
			UE_LOG(LogTemp, Warning, TEXT("Player: Mantle initiated from ledge"));
			return;
		}
	}

	// Normal jump
	Super::Jump();
}

void ASoulsLikeCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Reset double jump when landing
	if (ExoMovementComponent)
	{
		ExoMovementComponent->ResetDoubleJump();
		UE_LOG(LogTemp, Warning, TEXT("Player: Landed - double jump reset"));
	}
}

void ASoulsLikeCharacter::CheckLedgeGrab()
{
	// Only check if we have ExoMovementComponent
	if (!ExoMovementComponent)
	{
		return;
	}

	// Check if jump is held
	if (!bJumpHeld)
	{
		return;
	}

	// Must be in air
	if (!GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// Not already grabbing
	if (ExoMovementComponent->IsGrabbingLedge() || ExoMovementComponent->IsMantling())
	{
		return;
	}

	// Debug: Show we're checking for ledge
	if (ExoMovementComponent->bDebugLogging)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player: Checking for ledge grab (Jump held, in air)"));
	}

	// Try to grab a ledge
	if (ExoMovementComponent->TryLedgeGrab())
	{
		UE_LOG(LogTemp, Warning, TEXT("Player: Ledge grabbed while jumping"));
	}
}
