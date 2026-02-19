// CallOfTheMoutains - Souls-like Player Controller with Enhanced Input

#include "SoulsLikePlayerController.h"
#include "LockOnComponent.h"
#include "TargetableComponent.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "InventoryWidget.h"
#include "HotbarWidget.h"
#include "PlayerStatsWidget.h"
#include "InteractionPromptWidget.h"
#include "HealthComponent.h"
#include "ItemPickup.h"
#include "ItemTypes.h"
#include "SaveGameManager.h"
#include "SprintComponent.h"
#include "ExoMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"

ASoulsLikePlayerController::ASoulsLikePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create lock-on component on the controller
	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

	// Create save game manager component
	SaveGameManager = CreateDefaultSubobject<USaveGameManager>(TEXT("SaveGameManager"));

	// NOTE: Inventory and Equipment components are on the PAWN

	// Load input actions from /Game/Input/Actions/
	static ConstructorHelpers::FObjectFinder<UInputAction> LockOnActionFinder(TEXT("/Game/Input/Actions/IA_LockOn"));
	if (LockOnActionFinder.Succeeded())
	{
		IA_LockOn = LockOnActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DodgeActionFinder(TEXT("/Game/Input/Actions/IA_Dodge"));
	if (DodgeActionFinder.Succeeded())
	{
		IA_Dodge = DodgeActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/Actions/IA_Move"));
	if (MoveActionFinder.Succeeded())
	{
		IA_Move = MoveActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/Input/Actions/IA_Look"));
	if (LookActionFinder.Succeeded())
	{
		IA_Look = LookActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionFinder(TEXT("/Game/Input/Actions/IA_Crouch"));
	if (CrouchActionFinder.Succeeded())
	{
		IA_Crouch = CrouchActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(TEXT("/Game/Input/IMC_Default"));
	if (MappingContextFinder.Succeeded())
	{
		DefaultMappingContext = MappingContextFinder.Object;
	}
}

void ASoulsLikePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Delay widget creation to ensure everything is ready
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		CreateInventoryWidgets();
	});
}

void ASoulsLikePlayerController::CreateInventoryWidgets()
{
	if (!IsLocalController())
	{
		return;
	}

	// Get components from Pawn
	APawn* MyPawn = GetPawn();
	UInventoryComponent* InvComp = MyPawn ? MyPawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	UEquipmentComponent* EquipComp = MyPawn ? MyPawn->FindComponentByClass<UEquipmentComponent>() : nullptr;

	// Create Hotbar Widget (always visible)
	HotbarWidget = CreateWidget<UHotbarWidget>(this, UHotbarWidget::StaticClass());
	if (HotbarWidget)
	{
		HotbarWidget->AddToViewport(0);
		HotbarWidget->InitializeHotbar(EquipComp, InvComp);
	}

	// Create Inventory Widget (hidden by default)
	InventoryWidget = CreateWidget<UInventoryWidget>(this, UInventoryWidget::StaticClass());
	if (InventoryWidget)
	{
		InventoryWidget->AddToViewport(10);
		InventoryWidget->InitializeInventory(InvComp, EquipComp);
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Create Interaction Prompt Widget
	CreateInteractionPromptWidget();
}

void ASoulsLikePlayerController::CreateInteractionPromptWidget()
{
	if (!IsLocalController())
	{
		return;
	}

	InteractionPromptWidget = CreateWidget<UInteractionPromptWidget>(this, UInteractionPromptWidget::StaticClass());
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->AddToViewport(5);
	}
}

void ASoulsLikePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Set lock-on component's owner to the pawn for proper traces
	if (LockOnComponent)
	{
		LockOnComponent->SetOwnerActor(InPawn);

		// Bind to OnLockOnLost to restore rotation when target dies/goes out of range
		LockOnComponent->OnLockOnLost.AddDynamic(this, &ASoulsLikePlayerController::OnLockOnLostCallback);
	}

	// Cache spring arm
	CachedSpringArm = FindSpringArm();

	// Store original movement settings
	if (ACharacter* PossessedCharacter = Cast<ACharacter>(InPawn))
	{
		if (UCharacterMovementComponent* Movement = PossessedCharacter->GetCharacterMovement())
		{
			bOriginalOrientToMovement = Movement->bOrientRotationToMovement;
		}
	}

	// Find HealthComponent on the pawn (add via Blueprint)
	PawnHealthComponent = InPawn->FindComponentByClass<UHealthComponent>();
	if (PawnHealthComponent)
	{
		// Create stats widget after a short delay to ensure viewport is ready
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			CreateStatsWidget();
		});
	}

	// Find SprintComponent on the pawn
	PawnSprintComponent = InPawn->FindComponentByClass<USprintComponent>();

	// Find ExoMovementComponent on the pawn
	PawnExoMovementComponent = InPawn->FindComponentByClass<UExoMovementComponent>();
}

void ASoulsLikePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Lock-On - Started and Completed for hold detection
		if (IA_LockOn)
		{
			EnhancedInput->BindAction(IA_LockOn, ETriggerEvent::Started, this, &ASoulsLikePlayerController::OnLockOnStarted);
			EnhancedInput->BindAction(IA_LockOn, ETriggerEvent::Completed, this, &ASoulsLikePlayerController::OnLockOnCompleted);
		}

		// Dodge - track when shift is held
		if (IA_Dodge)
		{
			EnhancedInput->BindAction(IA_Dodge, ETriggerEvent::Started, this, &ASoulsLikePlayerController::OnDodgeKeyPressed);
			EnhancedInput->BindAction(IA_Dodge, ETriggerEvent::Completed, this, &ASoulsLikePlayerController::OnDodgeKeyReleased);
		}

		// Movement
		if (IA_Move)
		{
			EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASoulsLikePlayerController::OnMoveInput);
			EnhancedInput->BindAction(IA_Move, ETriggerEvent::Completed, this, &ASoulsLikePlayerController::OnMoveInput);
		}

		// Look
		if (IA_Look)
		{
			EnhancedInput->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ASoulsLikePlayerController::OnLookInput);
		}

		// Crouch/Slide
		if (IA_Crouch)
		{
			EnhancedInput->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ASoulsLikePlayerController::OnCrouchPressed);
			EnhancedInput->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &ASoulsLikePlayerController::OnCrouchReleased);
		}
	}
}

void ASoulsLikePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Track lock-on hold time
	if (bLockOnHeld)
	{
		LockOnHoldTime += DeltaTime;

		// Only trigger lock-on immediately if NOT already locked on
		// When locked on, we wait for release to distinguish tap (switch) from hold (release)
		if (!IsLockedOn() && LockOnHoldTime >= HoldThreshold && !bLockOnTriggeredThisHold)
		{
			bLockOnTriggeredThisHold = true;
			AcquireLockOn();
		}
	}

	// Update dodge cooldown
	if (DodgeCooldownTimer > 0.0f)
	{
		DodgeCooldownTimer -= DeltaTime;
	}

	// Update dodge
	if (bIsDodging)
	{
		UpdateDodge(DeltaTime);
	}

	// Update camera
	if (IsLockedOn())
	{
		UpdateLockedOnCamera(DeltaTime);
	}

	UpdateCameraDistance(DeltaTime);

	// Handle hotbar input (Arrow keys + I key)
	HandleHotbarInput();

	// Handle pickup detection (check for nearby pickups)
	HandlePickupDetection();

	// Handle E key for interaction/pickup
	HandleInteractionInput();

	// Debug input (T = damage, Y = heal, U = use stamina)
	HandleDebugInput();

	// Combat input (LMB = light attack, RMB = heavy attack, Q = stow)
	HandleCombatInput();

	// Sprint input (Shift hold)
	HandleSprintInput();

	// Exo movement input (jump held tracking, ledge grab, double jump)
	HandleExoMovementInput();
}

// ==================== Enhanced Input Handlers ====================

void ASoulsLikePlayerController::OnLockOnStarted(const FInputActionValue& Value)
{
	bLockOnHeld = true;
	LockOnHoldTime = 0.0f;
	bLockOnTriggeredThisHold = false;
}

void ASoulsLikePlayerController::OnLockOnCompleted(const FInputActionValue& Value)
{
	bLockOnHeld = false;

	// When locked on, handle tap vs hold on release
	if (IsLockedOn())
	{
		if (LockOnHoldTime < HoldThreshold)
		{
			// Quick tap while locked = switch targets
			SwitchTarget();
		}
		else
		{
			// Hold while locked = release lock-on
			ReleaseLockOn();
		}
	}

	LockOnHoldTime = 0.0f;
	bLockOnTriggeredThisHold = false;
}

void ASoulsLikePlayerController::OnDodgeKeyPressed(const FInputActionValue& Value)
{
	// Shift pressed - check for double tap to dodge
	// Sprint is handled by HandleSprintInput() via direct key polling for true hold behavior
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeSinceLastTap = CurrentTime - LastShiftTapTime;

	if (LastShiftTapTime > 0 && TimeSinceLastTap < DoubleTapWindow && CanDodge())
	{
		// Double tap detected - dodge in current movement direction
		StartDodge();
		LastShiftTapTime = -1.0f; // Reset to prevent triple-tap
	}
	else
	{
		// First tap - record time for double-tap detection
		LastShiftTapTime = CurrentTime;
	}
}

void ASoulsLikePlayerController::OnDodgeKeyReleased(const FInputActionValue& Value)
{
	// Sprint stop is handled by HandleSprintInput() via direct key polling
}

void ASoulsLikePlayerController::OnMoveInput(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

void ASoulsLikePlayerController::OnLookInput(const FInputActionValue& Value)
{
	// Only allow camera control when not locked on
	if (!IsLockedOn())
	{
		FVector2D LookValue = Value.Get<FVector2D>();
		AddYawInput(LookValue.X);
		AddPitchInput(LookValue.Y);
	}
}

// ==================== Lock-On Functions ====================

bool ASoulsLikePlayerController::IsLockedOn() const
{
	return LockOnComponent && LockOnComponent->IsLockedOn();
}

AActor* ASoulsLikePlayerController::GetLockOnTarget() const
{
	return LockOnComponent ? LockOnComponent->GetCurrentTarget() : nullptr;
}

void ASoulsLikePlayerController::AcquireLockOn()
{
	if (!LockOnComponent)
	{
		return;
	}

	LockOnComponent->ToggleLockOn();

	// Update character rotation mode
	if (LockOnComponent->IsLockedOn())
	{
		if (ACharacter* ControlledCharacter = GetCharacter())
		{
			if (UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
			{
				Movement->bOrientRotationToMovement = false;
			}
		}
		OnLockOnChanged.Broadcast(GetLockOnTarget());
	}
}

void ASoulsLikePlayerController::ReleaseLockOn()
{
	if (!LockOnComponent)
	{
		return;
	}

	LockOnComponent->ReleaseLockOn();

	// Restore character rotation mode
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		if (UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = bOriginalOrientToMovement;
		}
	}

	OnLockOnChanged.Broadcast(nullptr);
}

void ASoulsLikePlayerController::SwitchTarget()
{
	if (LockOnComponent && LockOnComponent->IsLockedOn())
	{
		// Switch based on horizontal input, or default to right
		float Direction = (FMath::Abs(MoveInput.X) > 0.1f) ? FMath::Sign(MoveInput.X) : 1.0f;
		LockOnComponent->SwitchTarget(Direction);
		OnLockOnChanged.Broadcast(GetLockOnTarget());
	}
}

// ==================== Dodge Functions ====================

bool ASoulsLikePlayerController::CanDodge() const
{
	if (bIsDodging || DodgeCooldownTimer > 0.0f)
	{
		return false;
	}

	ACharacter* ControlledCharacter = GetCharacter();
	if (!ControlledCharacter)
	{
		return false;
	}

	UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement();
	if (!Movement || !Movement->IsMovingOnGround())
	{
		return false;
	}

	// Check stamina via SprintComponent
	if (PawnSprintComponent && !PawnSprintComponent->CanDodge())
	{
		return false;
	}

	return true;
}

void ASoulsLikePlayerController::StartDodge()
{
	if (CanDodge())
	{
		ExecuteDodge(GetDodgeDirection());
	}
}

EDodgeDirection ASoulsLikePlayerController::GetDodgeDirectionEnum() const
{
	// Determine direction based on input
	float ForwardInput = MoveInput.Y;
	float RightInput = MoveInput.X;

	// If minimal input, default to backward when locked on, forward otherwise
	if (FMath::Abs(ForwardInput) < 0.3f && FMath::Abs(RightInput) < 0.3f)
	{
		return IsLockedOn() ? EDodgeDirection::Backward : EDodgeDirection::Forward;
	}

	// Determine primary direction
	if (FMath::Abs(ForwardInput) >= FMath::Abs(RightInput))
	{
		return (ForwardInput >= 0) ? EDodgeDirection::Forward : EDodgeDirection::Backward;
	}
	else
	{
		return (RightInput >= 0) ? EDodgeDirection::Right : EDodgeDirection::Left;
	}
}

FVector ASoulsLikePlayerController::GetDodgeDirection() const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return FVector::ForwardVector;
	}

	float ForwardInput = MoveInput.Y;
	float RightInput = MoveInput.X;

	// If there's movement input, dodge in that direction
	if (FMath::Abs(ForwardInput) > 0.1f || FMath::Abs(RightInput) > 0.1f)
	{
		FRotator DodgeRotation;

		if (IsLockedOn() && GetLockOnTarget())
		{
			// Relative to target
			FVector ToTarget = GetLockOnTarget()->GetActorLocation() - ControlledPawn->GetActorLocation();
			ToTarget.Z = 0;
			DodgeRotation = ToTarget.Rotation();
		}
		else
		{
			// Relative to camera
			DodgeRotation = GetControlRotation();
		}

		const FRotator YawRotation(0, DodgeRotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		return (ForwardDirection * ForwardInput + RightDirection * RightInput).GetSafeNormal();
	}
	else
	{
		// No input - dodge backward if locked on, forward otherwise
		if (IsLockedOn() && GetLockOnTarget())
		{
			FVector ToTarget = GetLockOnTarget()->GetActorLocation() - ControlledPawn->GetActorLocation();
			ToTarget.Z = 0;
			return -ToTarget.GetSafeNormal();
		}
		else
		{
			return ControlledPawn->GetActorForwardVector();
		}
	}
}

void ASoulsLikePlayerController::ExecuteDodge(FVector Direction)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// If locked on and we have ExoMovementComponent, use side-step instead of roll
	if (IsLockedOn() && PawnExoMovementComponent)
	{
		// Convert direction to EExoDodgeDirection
		EDodgeDirection DodgeDir = GetDodgeDirectionEnum();
		EExoDodgeDirection ExoDir;

		switch (DodgeDir)
		{
		case EDodgeDirection::Left:
			ExoDir = EExoDodgeDirection::Left;
			break;
		case EDodgeDirection::Right:
			ExoDir = EExoDodgeDirection::Right;
			break;
		case EDodgeDirection::Backward:
			ExoDir = EExoDodgeDirection::Backward;
			break;
		case EDodgeDirection::Forward:
		default:
			ExoDir = EExoDodgeDirection::Forward;
			break;
		}

		// Stop sprinting when dodging
		if (PawnSprintComponent)
		{
			PawnSprintComponent->StopSprint();
		}

		// Reset attack combo when dodging
		if (UEquipmentComponent* EquipComp = ControlledPawn->FindComponentByClass<UEquipmentComponent>())
		{
			EquipComp->ResetCombo();
		}

		// Try side-step (handles its own stamina consumption)
		if (PawnExoMovementComponent->TrySideStep(ExoDir))
		{
			// Track dodge state for external systems
			bIsDodging = true;
			LastDodgeDirection = DodgeDir;
			OnDodgeStarted.Broadcast(LastDodgeDirection);

			// Set timer to clear dodge state when side-step ends
			float SideStepDuration = PawnExoMovementComponent->SideStepDuration;
			GetWorld()->GetTimerManager().SetTimer(
				DodgeTimerHandle,
				[this]()
				{
					bIsDodging = false;
					bIsInvincible = false;
					OnDodgeEnded.Broadcast();
				},
				SideStepDuration,
				false
			);

			UE_LOG(LogTemp, Warning, TEXT("Controller: Using side-step dodge (locked on)"));
			return;
		}
		// If side-step failed, fall through to regular dodge
	}

	// Regular roll dodge (not locked on)
	// Consume stamina for dodge
	if (PawnSprintComponent)
	{
		if (!PawnSprintComponent->ConsumeDodgeStamina())
		{
			// Not enough stamina
			return;
		}
		// Stop sprinting when dodging
		PawnSprintComponent->StopSprint();
	}

	// Reset attack combo when dodging
	if (UEquipmentComponent* EquipComp = ControlledPawn->FindComponentByClass<UEquipmentComponent>())
	{
		EquipComp->ResetCombo();
	}

	bIsDodging = true;
	DodgeTimer = 0.0f;
	DodgeStartLocation = ControlledPawn->GetActorLocation();
	DodgeEndLocation = DodgeStartLocation + Direction * DodgeDistance;

	// Store direction for animation
	LastDodgeDirection = GetDodgeDirectionEnum();

	// Rotate character to face dodge direction (always, so the forward roll goes the right way)
	ControlledPawn->SetActorRotation(Direction.Rotation());

	// Play dodge animation
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		// Disable movement during dodge
		ControlledCharacter->GetCharacterMovement()->DisableMovement();

		// Play single dodge montage (character is already rotated to face direction)
		if (DodgeMontage)
		{
			if (UAnimInstance* AnimInstance = ControlledCharacter->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Play(DodgeMontage);
			}
		}
	}

	OnDodgeStarted.Broadcast(LastDodgeDirection);
}

void ASoulsLikePlayerController::UpdateDodge(float DeltaTime)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		EndDodge();
		return;
	}

	DodgeTimer += DeltaTime;
	float Alpha = FMath::Clamp(DodgeTimer / DodgeDuration, 0.0f, 1.0f);

	// Easing for smooth dodge
	float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);

	// Update position
	FVector NewLocation = FMath::Lerp(DodgeStartLocation, DodgeEndLocation, EasedAlpha);
	NewLocation.Z = ControlledPawn->GetActorLocation().Z;
	ControlledPawn->SetActorLocation(NewLocation, true);

	// Update i-frames
	bool bShouldBeInvincible = (Alpha >= IFrameStart && Alpha <= IFrameEnd);
	if (bShouldBeInvincible != bIsInvincible)
	{
		bIsInvincible = bShouldBeInvincible;
		OnIFrameStateChanged.Broadcast(bIsInvincible);
	}

	if (DodgeTimer >= DodgeDuration)
	{
		EndDodge();
	}
}

void ASoulsLikePlayerController::EndDodge()
{
	bIsDodging = false;
	bIsInvincible = false;
	DodgeCooldownTimer = DodgeCooldown;

	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	OnDodgeEnded.Broadcast();
}

// ==================== Camera ====================

void ASoulsLikePlayerController::UpdateLockedOnCamera(float DeltaTime)
{
	if (!LockOnComponent || !LockOnComponent->GetCurrentTarget())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	FVector PlayerLocation = ControlledPawn->GetActorLocation();
	FVector TargetLocation = LockOnComponent->GetCurrentTarget()->GetActorLocation();

	// Get target's targetable component for proper offset
	UTargetableComponent* TargetComp = LockOnComponent->GetCurrentTargetComponent();
	if (TargetComp)
	{
		TargetLocation = TargetComp->GetTargetLocation();
	}

	// Camera looks at a point between player and target at a reasonable height
	FVector MidPoint = (PlayerLocation + TargetLocation) * 0.5f;

	// Camera height should be at eye level, not target height
	float CameraHeight = PlayerLocation.Z + 60.0f; // Eye level offset
	FVector LookAtPoint = FVector(MidPoint.X, MidPoint.Y, CameraHeight);

	// Calculate camera rotation - look at the midpoint but with controlled pitch
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLocation + FVector(0, 0, 60), LookAtPoint);

	// Clamp pitch to prevent extreme angles
	LookAtRotation.Pitch = FMath::Clamp(LookAtRotation.Pitch, -30.0f, 30.0f);

	// Smooth camera rotation
	FRotator CurrentRotation = GetControlRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, LookAtRotation, DeltaTime, CameraLockOnSpeed);
	SetControlRotation(NewRotation);

	// Rotate character to face target (only yaw)
	FVector ToTarget = TargetLocation - PlayerLocation;
	ToTarget.Z = 0;
	if (!ToTarget.IsNearlyZero())
	{
		FRotator CharacterRotation = ToTarget.Rotation();
		FRotator CurrentCharRot = ControlledPawn->GetActorRotation();
		FRotator NewCharRot = FMath::RInterpTo(CurrentCharRot, FRotator(0, CharacterRotation.Yaw, 0), DeltaTime, 10.0f);
		ControlledPawn->SetActorRotation(NewCharRot);
	}
}

void ASoulsLikePlayerController::UpdateCameraDistance(float DeltaTime)
{
	if (!CachedSpringArm)
	{
		CachedSpringArm = FindSpringArm();
		if (!CachedSpringArm)
		{
			return;
		}
	}

	float TargetDistance = IsLockedOn() ? LockedOnCameraDistance : NormalCameraDistance;

	CachedSpringArm->TargetArmLength = FMath::FInterpTo(
		CachedSpringArm->TargetArmLength,
		TargetDistance,
		DeltaTime,
		CameraLockOnSpeed
	);
}

USpringArmComponent* ASoulsLikePlayerController::FindSpringArm() const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return nullptr;
	}

	return ControlledPawn->FindComponentByClass<USpringArmComponent>();
}

// ==================== Hotbar/Inventory Input ====================

bool ASoulsLikePlayerController::IsCtrlHeld() const
{
	return IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
}

void ASoulsLikePlayerController::HandleHotbarInput()
{
	// Up Arrow
	bool bUpDown = IsInputKeyDown(EKeys::Up);
	if (bUpDown && !bUpArrowWasDown)
	{
		HandleHotbarUp();
	}
	bUpArrowWasDown = bUpDown;

	// Down Arrow
	bool bDownDown = IsInputKeyDown(EKeys::Down);
	if (bDownDown && !bDownArrowWasDown)
	{
		HandleHotbarDown();
	}
	bDownArrowWasDown = bDownDown;

	// Left Arrow
	bool bLeftDown = IsInputKeyDown(EKeys::Left);
	if (bLeftDown && !bLeftArrowWasDown)
	{
		HandleHotbarLeft();
	}
	bLeftArrowWasDown = bLeftDown;

	// Right Arrow
	bool bRightDown = IsInputKeyDown(EKeys::Right);
	if (bRightDown && !bRightArrowWasDown)
	{
		HandleHotbarRight();
	}
	bRightArrowWasDown = bRightDown;

	// I Key - Toggle Inventory
	bool bIDown = IsInputKeyDown(EKeys::I);
	if (bIDown && !bIKeyWasDown)
	{
		ToggleInventory();
	}
	bIKeyWasDown = bIDown;
}

void ASoulsLikePlayerController::HandleHotbarUp()
{
	APawn* MyPawn = GetPawn();
	UEquipmentComponent* EquipComp = MyPawn ? MyPawn->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	// Up = Special/Spell slot
	if (IsCtrlHeld())
	{
		EquipComp->CycleHotbarNext(EHotbarSlot::Special);
	}
	else
	{
		EquipComp->UseSpecialItem();
	}
}

void ASoulsLikePlayerController::HandleHotbarRight()
{
	APawn* MyPawn = GetPawn();
	UEquipmentComponent* EquipComp = MyPawn ? MyPawn->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	// Right = Primary Weapon slot
	// Ctrl+Right cycles through equipped primary weapons
	if (IsCtrlHeld())
	{
		EquipComp->CyclePrimaryWeapon();
	}
}

void ASoulsLikePlayerController::HandleHotbarLeft()
{
	APawn* MyPawn = GetPawn();
	UEquipmentComponent* EquipComp = MyPawn ? MyPawn->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	// Left = Off-hand slot
	// Ctrl+Left cycles through equipped off-hand items
	if (IsCtrlHeld())
	{
		EquipComp->CycleOffHand();
	}
}

void ASoulsLikePlayerController::HandleHotbarDown()
{
	APawn* MyPawn = GetPawn();
	UEquipmentComponent* EquipComp = MyPawn ? MyPawn->FindComponentByClass<UEquipmentComponent>() : nullptr;
	if (!EquipComp) return;

	// Down = Consumable slot
	if (IsCtrlHeld())
	{
		EquipComp->CycleHotbarNext(EHotbarSlot::Consumable);
	}
	else
	{
		EquipComp->UseConsumable();
	}
}

void ASoulsLikePlayerController::ToggleInventory()
{
	bInventoryOpen = !bInventoryOpen;

	// Show/hide inventory widget
	if (InventoryWidget)
	{
		if (bInventoryOpen)
		{
			InventoryWidget->SetVisibility(ESlateVisibility::Visible);
			InventoryWidget->RefreshAll();
		}
		else
		{
			InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Toggle mouse cursor and input mode
	if (bInventoryOpen)
	{
		SetShowMouseCursor(true);
		SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());
	}
}

void ASoulsLikePlayerController::CreateStatsWidget()
{
	if (!IsLocalController() || !PawnHealthComponent)
	{
		return;
	}

	PlayerStatsWidget = CreateWidget<UPlayerStatsWidget>(this, UPlayerStatsWidget::StaticClass());
	if (PlayerStatsWidget)
	{
		PlayerStatsWidget->AddToViewport(1);
		PlayerStatsWidget->InitializeStats(PawnHealthComponent);
	}
}

// ==================== Pickup/Interaction Handling ====================

void ASoulsLikePlayerController::HandlePickupDetection()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		// Clear focus if no pawn
		if (CurrentFocusedPickup)
		{
			OnPickupFocusChanged(CurrentFocusedPickup, false);
		}
		return;
	}

	// Get all overlapping actors on the pawn
	TArray<AActor*> OverlappingActors;
	ControlledPawn->GetOverlappingActors(OverlappingActors, AItemPickup::StaticClass());

	// Find the closest pickup
	AItemPickup* ClosestPickup = nullptr;
	float ClosestDistance = FLT_MAX;
	FVector PawnLocation = ControlledPawn->GetActorLocation();

	for (AActor* Actor : OverlappingActors)
	{
		AItemPickup* Pickup = Cast<AItemPickup>(Actor);
		if (Pickup && !Pickup->IsCollected())
		{
			float Distance = FVector::Dist(PawnLocation, Pickup->GetActorLocation());
			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestPickup = Pickup;
			}
		}
	}

	// Update focus state
	if (ClosestPickup != CurrentFocusedPickup)
	{
		// Lost focus on previous pickup
		if (CurrentFocusedPickup)
		{
			OnPickupFocusChanged(CurrentFocusedPickup, false);
		}

		// Gained focus on new pickup
		if (ClosestPickup)
		{
			OnPickupFocusChanged(ClosestPickup, true);
		}
	}
}

void ASoulsLikePlayerController::OnPickupFocusChanged(AItemPickup* Pickup, bool bFocused)
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

void ASoulsLikePlayerController::HandleInteractionInput()
{
	// Don't allow interaction while inventory is open
	if (bInventoryOpen)
	{
		return;
	}

	// E Key - Interact/Pickup
	bool bEDown = IsInputKeyDown(EKeys::E);
	if (bEDown && !bEKeyWasDown)
	{
		TryPickupItem();
	}
	bEKeyWasDown = bEDown;
}

void ASoulsLikePlayerController::TryPickupItem()
{
	if (!CurrentFocusedPickup)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// Try to pick up the item
	if (CurrentFocusedPickup->TryPickup(ControlledPawn))
	{
		// Refresh inventory widget if open
		if (InventoryWidget && bInventoryOpen)
		{
			InventoryWidget->RefreshAll();
		}

		// Refresh hotbar to show new items
		if (HotbarWidget)
		{
			HotbarWidget->UpdateAllSlots();
		}

		// The pickup will destroy itself, clear our reference
		CurrentFocusedPickup = nullptr;

		// Hide the prompt
		if (InteractionPromptWidget)
		{
			InteractionPromptWidget->HidePrompt();
		}
	}
}

void ASoulsLikePlayerController::HandleDebugInput()
{
	if (!PawnHealthComponent) return;

	// T key - take 20 damage
	static bool bTWasDown = false;
	bool bTDown = IsInputKeyDown(EKeys::T);
	if (bTDown && !bTWasDown)
	{
		PawnHealthComponent->TakeDamage(20.0f, nullptr, nullptr);
	}
	bTWasDown = bTDown;

	// Y key - heal 20
	static bool bYWasDown = false;
	bool bYDown = IsInputKeyDown(EKeys::Y);
	if (bYDown && !bYWasDown)
	{
		PawnHealthComponent->Heal(20.0f);
	}
	bYWasDown = bYDown;

	// U key - use 30 stamina
	static bool bUWasDown = false;
	bool bUDown = IsInputKeyDown(EKeys::U);
	if (bUDown && !bUWasDown)
	{
		PawnHealthComponent->UseStamina(30.0f);
	}
	bUWasDown = bUDown;
}

// ==================== Combat Input ====================

void ASoulsLikePlayerController::HandleCombatInput()
{
	// Don't allow combat while inventory is open or dodging
	if (bInventoryOpen || bIsDodging)
	{
		return;
	}

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	UEquipmentComponent* EquipComp = MyPawn->FindComponentByClass<UEquipmentComponent>();
	if (!EquipComp)
	{
		return;
	}

	// Left Mouse Button - Light Attack
	bool bLeftMouseDown = IsInputKeyDown(EKeys::LeftMouseButton);
	if (bLeftMouseDown && !bLeftMouseWasDown)
	{
		EquipComp->LightAttack();
	}
	bLeftMouseWasDown = bLeftMouseDown;

	// Right Mouse Button - Heavy Attack (tap) or Guard (hold)
	bool bRightMouseDown = IsInputKeyDown(EKeys::RightMouseButton);
	if (bRightMouseDown && !bRightMouseWasDown)
	{
		// Start heavy attack on press
		EquipComp->HeavyAttack();
	}
	bRightMouseWasDown = bRightMouseDown;

	// Q Key - Guard/Block (hold)
	bool bQDown = IsInputKeyDown(EKeys::Q);
	if (bQDown && !bQKeyWasDown)
	{
		EquipComp->StartGuard();
	}
	else if (!bQDown && bQKeyWasDown)
	{
		EquipComp->StopGuard();
	}
	bQKeyWasDown = bQDown;

	// C Key - Toggle Weapon Stow
	bool bCDown = IsInputKeyDown(EKeys::C);
	if (bCDown && !bCKeyWasDown)
	{
		EquipComp->ToggleWeaponStow();
	}
	bCKeyWasDown = bCDown;
}

// ==================== Sprint Input ====================

void ASoulsLikePlayerController::HandleSprintInput()
{
	if (!PawnSprintComponent)
	{
		return;
	}

	// Direct key polling for true hold-to-sprint behavior
	// This ensures sprint state always matches whether shift is held
	bool bShiftDown = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

	// Don't sprint while dodging
	if (bIsDodging)
	{
		bShiftWasDown = bShiftDown;
		return;
	}

	// Update sprint based on current key state (true hold behavior)
	if (bShiftDown)
	{
		PawnSprintComponent->StartSprint();
	}
	else
	{
		PawnSprintComponent->StopSprint();
	}

	bShiftWasDown = bShiftDown;
}

// ==================== Lock-On Callbacks ====================

void ASoulsLikePlayerController::OnLockOnLostCallback(AActor* LostTarget)
{
	// This is called when the LockOnComponent loses its target
	// (target died, went out of range, became non-targetable, etc.)
	// We need to restore the character's rotation settings

	// Restore character rotation mode
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		if (UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = bOriginalOrientToMovement;
		}
	}

	// Broadcast lock-on changed with nullptr to notify UI
	OnLockOnChanged.Broadcast(nullptr);
}

// ==================== Crouch/Slide Input ====================

void ASoulsLikePlayerController::OnCrouchPressed(const FInputActionValue& Value)
{
	// If sprinting and we have ExoMovementComponent, try to slide
	if (PawnSprintComponent && PawnSprintComponent->bIsSprinting && PawnExoMovementComponent)
	{
		if (PawnExoMovementComponent->TrySlide())
		{
			// Successfully started slide - stop sprint
			PawnSprintComponent->StopSprint();
			UE_LOG(LogTemp, Warning, TEXT("Controller: Slide initiated from sprint"));
		}
	}
	// Otherwise, could implement crouch here if desired
}

void ASoulsLikePlayerController::OnCrouchReleased(const FInputActionValue& Value)
{
	// Could handle crouch release here if implementing crouch stance
}

// ==================== Exo Movement Input ====================

void ASoulsLikePlayerController::HandleExoMovementInput()
{
	// Track jump held state via direct key polling
	bJumpHeld = IsInputKeyDown(EKeys::SpaceBar);

	// If grabbing ledge, handle ledge-specific input
	if (PawnExoMovementComponent && PawnExoMovementComponent->IsGrabbingLedge())
	{
		// S key = release ledge and fall
		if (IsInputKeyDown(EKeys::S))
		{
			UE_LOG(LogTemp, Warning, TEXT("Controller: S pressed - releasing ledge"));
			PawnExoMovementComponent->ReleaseLedge();
			return;
		}

		// W key = mantle up
		if (IsInputKeyDown(EKeys::W))
		{
			UE_LOG(LogTemp, Warning, TEXT("Controller: W pressed - trying mantle"));
			PawnExoMovementComponent->TryMantle();
			return;
		}
	}

	// Check for ledge grab while jump is held and in air
	CheckLedgeGrab();

	// Handle double jump on space press (not hold)
	HandleDoubleJump();
}

void ASoulsLikePlayerController::CheckLedgeGrab()
{
	// Need ExoMovementComponent on pawn
	if (!PawnExoMovementComponent)
	{
		return;
	}

	// Must be holding jump
	if (!bJumpHeld)
	{
		return;
	}

	// Get the character
	ACharacter* MyCharacter = GetCharacter();
	if (!MyCharacter)
	{
		return;
	}

	UCharacterMovementComponent* Movement = MyCharacter->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// Must be falling (in air)
	if (!Movement->IsFalling())
	{
		return;
	}

	// Not already grabbing or mantling
	if (PawnExoMovementComponent->IsGrabbingLedge() || PawnExoMovementComponent->IsMantling())
	{
		return;
	}

	// Debug logging
	if (PawnExoMovementComponent->bDebugLogging)
	{
		UE_LOG(LogTemp, Warning, TEXT("Controller: Checking for ledge (Jump held, in air)"));
	}

	// Try to grab ledge
	if (PawnExoMovementComponent->TryLedgeGrab())
	{
		UE_LOG(LogTemp, Warning, TEXT("Controller: Ledge grabbed!"));
	}
}

void ASoulsLikePlayerController::HandleDoubleJump()
{
	// Need ExoMovementComponent on pawn
	if (!PawnExoMovementComponent)
	{
		return;
	}

	// Track space bar press (not hold)
	static bool bSpaceWasDown = false;
	bool bSpaceDown = IsInputKeyDown(EKeys::SpaceBar);

	// Only on press, not hold
	if (bSpaceDown && !bSpaceWasDown)
	{
		// FIRST: Check if grabbing ledge - mantle takes priority
		if (PawnExoMovementComponent->IsGrabbingLedge())
		{
			UE_LOG(LogTemp, Warning, TEXT("Controller: Space pressed while on ledge - trying mantle"));
			PawnExoMovementComponent->TryMantle();
		}
		else
		{
			// Not on ledge - check for double jump
			ACharacter* MyCharacter = GetCharacter();
			if (MyCharacter)
			{
				UCharacterMovementComponent* Movement = MyCharacter->GetCharacterMovement();

				// If in air (falling), try double jump
				if (Movement && Movement->IsFalling())
				{
					if (PawnExoMovementComponent->CanDoubleJump())
					{
						PawnExoMovementComponent->TryDoubleJump();
					}
				}
			}
		}
	}

	bSpaceWasDown = bSpaceDown;
}
