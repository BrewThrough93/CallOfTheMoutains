// CallOfTheMoutains - Sprint Component Implementation

#include "SprintComponent.h"
#include "HealthComponent.h"
#include "EquipmentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"

USprintComponent::USprintComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USprintComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheComponents();

	// Store original FOV
	if (PlayerController)
	{
		OriginalFOV = PlayerController->PlayerCameraManager ?
			PlayerController->PlayerCameraManager->GetFOVAngle() : 90.0f;
		CurrentTargetFOV = OriginalFOV;
	}

	// Initialize current speed
	CurrentSpeed = BaseWalkSpeed;

	// Apply initial speed to movement component
	if (MovementComponent)
	{
		MovementComponent->MaxWalkSpeed = CurrentSpeed;
	}
}

void USprintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateSprint(DeltaTime);
	UpdateSpeed(DeltaTime);
	UpdateCameraFOV(DeltaTime);
}

void USprintComponent::CacheComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Get components from owner (should be on the character/pawn)
	HealthComponent = Owner->FindComponentByClass<UHealthComponent>();
	EquipmentComponent = Owner->FindComponentByClass<UEquipmentComponent>();

	// Get movement component
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		MovementComponent = Character->GetCharacterMovement();
	}

	// Get player controller
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		PlayerController = Cast<APlayerController>(Pawn->GetController());
	}
}

void USprintComponent::StartSprint()
{
	bSprintInputHeld = true;
}

void USprintComponent::StopSprint()
{
	bSprintInputHeld = false;
}

bool USprintComponent::CanSprint() const
{
	// Can't sprint if exhausted
	if (bIsExhausted)
	{
		return false;
	}

	// Need minimum stamina
	if (HealthComponent)
	{
		float CurrentStamina = HealthComponent->GetStamina();
		if (CurrentStamina < MinStaminaToSprint)
		{
			return false;
		}
	}

	// Need to be moving
	if (!HasMovementInput())
	{
		return false;
	}

	// Need movement component and to be on ground
	if (MovementComponent)
	{
		if (!MovementComponent->IsMovingOnGround())
		{
			return false;
		}
	}

	return true;
}

bool USprintComponent::CanDodge() const
{
	if (!HealthComponent)
	{
		return true; // If no health component, allow dodge
	}

	return HealthComponent->GetStamina() >= DodgeStaminaCost;
}

bool USprintComponent::ConsumeDodgeStamina()
{
	if (!CanDodge())
	{
		return false;
	}

	if (HealthComponent)
	{
		HealthComponent->UseStamina(DodgeStaminaCost);
	}

	return true;
}

float USprintComponent::GetCurrentMaxSpeed() const
{
	return CurrentSpeed;
}

float USprintComponent::GetTargetSpeed() const
{
	float TargetSpeed = BaseWalkSpeed;

	// Apply weapon stow modifier
	if (EquipmentComponent)
	{
		if (EquipmentComponent->AreWeaponsStowed())
		{
			TargetSpeed *= WeaponsStowedSpeedBonus;
		}
		else
		{
			TargetSpeed *= WeaponsDrawnSpeedPenalty;
		}
	}

	// Apply sprint modifier if sprinting
	if (bIsSprinting)
	{
		TargetSpeed *= SprintSpeedMultiplier;
	}

	return TargetSpeed;
}

bool USprintComponent::IsMoving() const
{
	if (!MovementComponent)
	{
		return false;
	}

	return MovementComponent->Velocity.SizeSquared2D() > 100.0f;
}

bool USprintComponent::HasMovementInput() const
{
	if (!MovementComponent)
	{
		return false;
	}

	// Check if there's acceleration (input being applied)
	FVector Acceleration = MovementComponent->GetCurrentAcceleration();
	return Acceleration.SizeSquared2D() > 100.0f;
}

void USprintComponent::UpdateSprint(float DeltaTime)
{
	// Handle exhaustion cooldown
	if (bIsExhausted)
	{
		ExhaustionTimer -= DeltaTime;
		if (ExhaustionTimer <= 0.0f)
		{
			bIsExhausted = false;
			ExhaustionTimer = 0.0f;
		}
	}

	// Check if we should be sprinting
	bool bShouldSprint = bSprintInputHeld && CanSprint();

	// Update sprint state
	if (bShouldSprint && !bIsSprinting)
	{
		SetSprintState(true);
	}
	else if (!bShouldSprint && bIsSprinting)
	{
		SetSprintState(false);
	}

	// Consume stamina while sprinting - only if actually moving
	if (bIsSprinting && HealthComponent && IsMoving())
	{
		float StaminaCost = SprintStaminaCostPerSecond * DeltaTime;
		HealthComponent->UseStamina(StaminaCost);

		// Check for exhaustion
		if (HealthComponent->GetStamina() <= 0.0f)
		{
			bIsExhausted = true;
			ExhaustionTimer = ExhaustionCooldown;
			SetSprintState(false);
			OnSprintExhausted.Broadcast();
		}
	}
}

void USprintComponent::UpdateSpeed(float DeltaTime)
{
	if (!MovementComponent)
	{
		return;
	}

	// Get target speed based on current state
	float TargetSpeed = GetTargetSpeed();

	// Interpolate current speed toward target
	CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, SpeedInterpSpeed);

	// Apply to movement component
	MovementComponent->MaxWalkSpeed = CurrentSpeed;
}

void USprintComponent::UpdateCameraFOV(float DeltaTime)
{
	if (!bSprintFOVEffect || !PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	// Determine target FOV
	float TargetFOV = bIsSprinting ? (OriginalFOV + SprintFOVIncrease) : OriginalFOV;

	// Interpolate current target FOV
	CurrentTargetFOV = FMath::FInterpTo(CurrentTargetFOV, TargetFOV, DeltaTime, FOVInterpSpeed);

	// Apply FOV
	PlayerController->PlayerCameraManager->SetFOV(CurrentTargetFOV);
}

void USprintComponent::SetSprintState(bool bNewState)
{
	if (bIsSprinting == bNewState)
	{
		return;
	}

	bIsSprinting = bNewState;
	OnSprintStateChanged.Broadcast(bIsSprinting);
}
