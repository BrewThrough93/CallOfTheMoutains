// CallOfTheMoutains - Exo-Suit Movement Component Implementation

#include "ExoMovementComponent.h"
#include "HealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

UExoMovementComponent::UExoMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UExoMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheComponents();

	// Store original capsule height for slide restoration
	if (CapsuleComponent)
	{
		OriginalCapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	}
}

void UExoMovementComponent::CacheComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	OwnerCharacter = Cast<ACharacter>(Owner);
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
		CapsuleComponent = OwnerCharacter->GetCapsuleComponent();

		// Find spring arm for camera handling
		SpringArmComponent = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
		if (SpringArmComponent)
		{
			// Store original camera settings
			bCameraLagWasEnabled = SpringArmComponent->bEnableCameraLag;
			OriginalCameraLagSpeed = SpringArmComponent->CameraLagSpeed;
			OriginalSocketOffset = SpringArmComponent->SocketOffset;
			OriginalTargetArmLength = SpringArmComponent->TargetArmLength;

			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Found SpringArm - Lag=%s, LagSpeed=%.1f, ArmLength=%.1f"),
				bCameraLagWasEnabled ? TEXT("true") : TEXT("false"), OriginalCameraLagSpeed, OriginalTargetArmLength);
		}
	}

	// Try to find HealthComponent on owner first, then on controller
	HealthComponent = Owner->FindComponentByClass<UHealthComponent>();
	if (!HealthComponent && OwnerCharacter)
	{
		if (AController* Controller = OwnerCharacter->GetController())
		{
			HealthComponent = Controller->FindComponentByClass<UHealthComponent>();
		}
	}
}

void UExoMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update cooldowns
	if (SideStepCooldownTimer > 0.0f)
	{
		SideStepCooldownTimer -= DeltaTime;
	}
	if (SlideCooldownTimer > 0.0f)
	{
		SlideCooldownTimer -= DeltaTime;
	}
	if (LedgeGrabCooldownTimer > 0.0f)
	{
		LedgeGrabCooldownTimer -= DeltaTime;
	}

	// Update current state
	switch (CurrentState)
	{
	case EExoMovementState::SideStep:
		UpdateSideStep(DeltaTime);
		break;

	case EExoMovementState::Sliding:
		UpdateSlide(DeltaTime);
		break;

	case EExoMovementState::Mantling:
		UpdateMantle(DeltaTime);
		break;

	case EExoMovementState::LedgeGrabbing:
		// LOCK position and prevent any movement while grabbing ledge
		if (OwnerCharacter && MovementComponent && CapsuleComponent)
		{
			// Keep character at hang position - must match SnapToLedge offset
			float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
			float WallOffset = CapsuleRadius + 80.0f; // Match SnapToLedge offset
			FVector HangPosition = LedgeLocation;
			HangPosition += LedgeNormal * WallOffset;
			HangPosition.Z = LedgeLocation.Z - OriginalCapsuleHalfHeight * 0.6f;

			// Use teleport to force position regardless of physics state
			OwnerCharacter->SetActorLocation(HangPosition, false, nullptr, ETeleportType::TeleportPhysics);

			// Ensure movement stays disabled
			MovementComponent->Velocity = FVector::ZeroVector;
			MovementComponent->GravityScale = 0.0f;
		}
		break;

	default:
		// Safety: If not in special state, ensure gravity is normal
		if (MovementComponent && MovementComponent->GravityScale != 1.0f)
		{
			// Only restore if we're truly not in any exo state
			if (CurrentState == EExoMovementState::None)
			{
				MovementComponent->GravityScale = 1.0f;
				if (bDebugLogging)
				{
					UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Safety restored gravity"));
				}
			}
		}
		break;
	}
}

void UExoMovementComponent::SetState(EExoMovementState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	EExoMovementState OldState = CurrentState;
	CurrentState = NewState;

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: State changed from %d to %d"), (int32)OldState, (int32)NewState);

	OnExoMovementStateChanged.Broadcast(NewState);
}

UAnimInstance* UExoMovementComponent::GetAnimInstance() const
{
	if (OwnerCharacter)
	{
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{
			return Mesh->GetAnimInstance();
		}
	}
	return nullptr;
}

bool UExoMovementComponent::IsOnGround() const
{
	if (MovementComponent)
	{
		return MovementComponent->IsMovingOnGround();
	}
	return false;
}

bool UExoMovementComponent::IsInAir() const
{
	if (MovementComponent)
	{
		return MovementComponent->IsFalling();
	}
	return false;
}

void UExoMovementComponent::UpdateIFrames(float Progress, float IFrameStart, float IFrameEnd)
{
	bool bShouldBeInvincible = (Progress >= IFrameStart && Progress <= IFrameEnd);

	if (bShouldBeInvincible != bIsInvincible)
	{
		bIsInvincible = bShouldBeInvincible;
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: I-Frames %s at progress %.2f"),
			bIsInvincible ? TEXT("ACTIVE") : TEXT("ENDED"), Progress);
	}
}

void UExoMovementComponent::ForceEndCurrentState()
{
	switch (CurrentState)
	{
	case EExoMovementState::SideStep:
		EndSideStep();
		break;

	case EExoMovementState::Sliding:
		EndSlide();
		break;

	case EExoMovementState::LedgeGrabbing:
		ReleaseLedge();
		break;

	case EExoMovementState::Mantling:
		EndMantle();
		break;

	default:
		break;
	}

	bIsInvincible = false;
	SetState(EExoMovementState::None);
}

// ==================== Side-Step Implementation ====================

bool UExoMovementComponent::CanSideStep() const
{
	// Must not be in another state
	if (CurrentState != EExoMovementState::None)
	{
		return false;
	}

	// Must be on ground
	if (!IsOnGround())
	{
		return false;
	}

	// Must not be on cooldown
	if (SideStepCooldownTimer > 0.0f)
	{
		return false;
	}

	// Must have enough stamina
	if (HealthComponent && !HealthComponent->HasStamina(SideStepStaminaCost))
	{
		return false;
	}

	return true;
}

FVector UExoMovementComponent::GetDirectionVector(EExoDodgeDirection Direction) const
{
	if (!OwnerCharacter)
	{
		return FVector::ForwardVector;
	}

	FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
	FVector Forward = CharacterRotation.Vector();
	FVector Right = FRotationMatrix(CharacterRotation).GetUnitAxis(EAxis::Y);

	switch (Direction)
	{
	case EExoDodgeDirection::Forward:
		return Forward;
	case EExoDodgeDirection::Backward:
		return -Forward;
	case EExoDodgeDirection::Left:
		return -Right;
	case EExoDodgeDirection::Right:
		return Right;
	default:
		return Forward;
	}
}

bool UExoMovementComponent::TrySideStep(EExoDodgeDirection Direction)
{
	if (!CanSideStep())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Cannot side-step - preconditions not met"));
		return false;
	}

	// Consume stamina
	if (HealthComponent)
	{
		if (!HealthComponent->UseStamina(SideStepStaminaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Cannot side-step - not enough stamina"));
			return false;
		}
	}

	CurrentDodgeDirection = Direction;

	// Calculate start and end positions
	SideStepStartLocation = OwnerCharacter->GetActorLocation();
	FVector DirectionVector = GetDirectionVector(Direction);
	SideStepEndLocation = SideStepStartLocation + DirectionVector * SideStepDistance;

	// Reset timer
	SideStepTimer = 0.0f;

	// Disable movement during side-step
	if (MovementComponent)
	{
		MovementComponent->DisableMovement();
	}

	// Play appropriate montage
	UAnimMontage* MontageToPlay = nullptr;
	switch (Direction)
	{
	case EExoDodgeDirection::Left:
		MontageToPlay = SideStepLeftMontage;
		break;
	case EExoDodgeDirection::Right:
		MontageToPlay = SideStepRightMontage;
		break;
	case EExoDodgeDirection::Backward:
		MontageToPlay = SideStepBackMontage;
		break;
	case EExoDodgeDirection::Forward:
		MontageToPlay = SideStepForwardMontage;
		break;
	}

	if (MontageToPlay)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Play(MontageToPlay);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: No side-step montage set for direction %d"), (int32)Direction);
	}

	SetState(EExoMovementState::SideStep);
	OnSideStepStarted.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Side-step started - Direction: %d, Distance: %.1f"),
		(int32)Direction, SideStepDistance);

	return true;
}

void UExoMovementComponent::UpdateSideStep(float DeltaTime)
{
	SideStepTimer += DeltaTime;
	float Progress = FMath::Clamp(SideStepTimer / SideStepDuration, 0.0f, 1.0f);

	// Update i-frames
	UpdateIFrames(Progress, SideStepIFrameStart, SideStepIFrameEnd);

	// Interpolate position with easing
	float EasedProgress = FMath::InterpEaseOut(0.0f, 1.0f, Progress, 2.0f);
	FVector NewLocation = FMath::Lerp(SideStepStartLocation, SideStepEndLocation, EasedProgress);

	// Maintain Z position
	NewLocation.Z = OwnerCharacter->GetActorLocation().Z;

	OwnerCharacter->SetActorLocation(NewLocation);

	// Check if complete
	if (Progress >= 1.0f)
	{
		EndSideStep();
	}
}

void UExoMovementComponent::EndSideStep()
{
	bIsInvincible = false;
	SideStepCooldownTimer = SideStepCooldown;

	// Restore movement
	if (MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	SetState(EExoMovementState::None);
	OnSideStepEnded.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Side-step ended"));
}

// ==================== Slide Implementation ====================

bool UExoMovementComponent::CanSlide() const
{
	// Must not be in another state
	if (CurrentState != EExoMovementState::None)
	{
		return false;
	}

	// Must be on ground
	if (!IsOnGround())
	{
		return false;
	}

	// Must not be on cooldown
	if (SlideCooldownTimer > 0.0f)
	{
		return false;
	}

	// Must be moving fast enough
	if (MovementComponent)
	{
		float CurrentSpeed = MovementComponent->Velocity.Size2D();
		if (CurrentSpeed < MinSpeedToSlide)
		{
			return false;
		}
	}

	// Must have enough stamina
	if (HealthComponent && !HealthComponent->HasStamina(SlideStaminaCost))
	{
		return false;
	}

	return true;
}

bool UExoMovementComponent::TrySlide()
{
	if (!CanSlide())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Cannot slide - preconditions not met"));
		return false;
	}

	// Consume stamina
	if (HealthComponent)
	{
		if (!HealthComponent->UseStamina(SlideStaminaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Cannot slide - not enough stamina"));
			return false;
		}
	}

	// Get slide direction from current velocity
	if (MovementComponent)
	{
		SlideDirection = MovementComponent->Velocity.GetSafeNormal2D();
		if (SlideDirection.IsNearlyZero())
		{
			SlideDirection = OwnerCharacter->GetActorForwardVector();
		}
	}
	else
	{
		SlideDirection = OwnerCharacter->GetActorForwardVector();
	}

	SlideTimer = 0.0f;

	// Shrink capsule for low profile
	if (CapsuleComponent && MovementComponent)
	{
		// Store original gravity and set to flying to prevent floor collision issues
		MovementComponent->SetMovementMode(MOVE_Flying);
		MovementComponent->GravityScale = 0.0f;

		// Shrink capsule first
		CapsuleComponent->SetCapsuleHalfHeight(SlideCapsuleHalfHeight);

		// Trace down to find floor and position correctly
		FVector CurrentLocation = OwnerCharacter->GetActorLocation();
		FHitResult FloorHit;
		FVector TraceStart = CurrentLocation;
		FVector TraceEnd = CurrentLocation - FVector(0, 0, 200.0f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(OwnerCharacter);

		FVector NewLocation = CurrentLocation;
		if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			// Position capsule so its bottom is at floor level + small buffer
			NewLocation.Z = FloorHit.Location.Z + SlideCapsuleHalfHeight + 2.0f;
		}
		else
		{
			// Fallback - move down by height difference
			float HeightDifference = OriginalCapsuleHalfHeight - SlideCapsuleHalfHeight;
			NewLocation.Z -= HeightDifference;
		}

		OwnerCharacter->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
		SlideStartLocation = NewLocation;

		// Clear velocity - we'll manually control position
		MovementComponent->Velocity = FVector::ZeroVector;

		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Slide started at Z=%.1f (floor trace)"), NewLocation.Z);
		}
	}
	else
	{
		SlideStartLocation = OwnerCharacter->GetActorLocation();
	}

	// Play slide montage
	if (SlideMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Play(SlideMontage);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: No slide montage set"));
	}

	// Rotate to face slide direction
	FRotator SlideRotation = SlideDirection.Rotation();
	SlideRotation.Pitch = 0;
	SlideRotation.Roll = 0;
	OwnerCharacter->SetActorRotation(SlideRotation);

	SetState(EExoMovementState::Sliding);
	OnSlideStarted.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Slide started"));

	return true;
}

void UExoMovementComponent::UpdateSlide(float DeltaTime)
{
	SlideTimer += DeltaTime;
	float Progress = FMath::Clamp(SlideTimer / SlideDuration, 0.0f, 1.0f);

	// Calculate slide position with deceleration
	float EasedProgress = FMath::InterpEaseOut(0.0f, 1.0f, Progress, 1.5f);
	FVector TargetLocation = SlideStartLocation + SlideDirection * SlideDistance * EasedProgress;

	// Trace down to find floor and position character correctly above it
	FHitResult FloorHit;
	FVector TraceStart = TargetLocation + FVector(0, 0, 50.0f); // Start above
	FVector TraceEnd = TargetLocation - FVector(0, 0, 200.0f); // Trace down

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		// Position capsule so its bottom is at floor level
		// Capsule center should be at floor + SlideCapsuleHalfHeight
		TargetLocation.Z = FloorHit.Location.Z + SlideCapsuleHalfHeight + 2.0f; // +2 for small buffer
	}
	else
	{
		// No floor found - maintain current Z
		TargetLocation.Z = OwnerCharacter->GetActorLocation().Z;
	}

	// Check for obstacles before moving
	if (IsSlideBlocked())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Slide blocked by obstacle"));
		EndSlide();
		return;
	}

	OwnerCharacter->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// Check if complete
	if (Progress >= 1.0f)
	{
		EndSlide();
	}
}

bool UExoMovementComponent::IsSlideBlocked() const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	// Trace forward to check for obstacles
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start + SlideDirection * 50.0f; // Check 50 units ahead

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams);
}

void UExoMovementComponent::EndSlide()
{
	SlideCooldownTimer = SlideCooldown;

	// Restore capsule size (this also moves character up)
	RestoreCapsuleSize();

	// Restore gravity and movement mode
	if (MovementComponent)
	{
		MovementComponent->GravityScale = 1.0f;
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	// Stop montage
	if (SlideMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.2f, SlideMontage);
		}
	}

	SetState(EExoMovementState::None);
	OnSlideEnded.Broadcast();

	if (bDebugLogging)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Slide ended"));
	}
}

void UExoMovementComponent::RestoreCapsuleSize()
{
	if (CapsuleComponent && OriginalCapsuleHalfHeight > 0.0f)
	{
		float HeightDifference = OriginalCapsuleHalfHeight - SlideCapsuleHalfHeight;
		FVector CharLocation = OwnerCharacter->GetActorLocation();

		// Target location is moved UP to compensate for capsule growing
		FVector TestLocation = CharLocation + FVector(0, 0, HeightDifference);

		FCollisionShape StandingShape = FCollisionShape::MakeCapsule(
			CapsuleComponent->GetUnscaledCapsuleRadius(),
			OriginalCapsuleHalfHeight
		);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(OwnerCharacter);

		if (!GetWorld()->OverlapBlockingTestByChannel(TestLocation, FQuat::Identity, ECC_Pawn, StandingShape, QueryParams))
		{
			// Room to stand - restore capsule and move UP
			CapsuleComponent->SetCapsuleHalfHeight(OriginalCapsuleHalfHeight);
			OwnerCharacter->SetActorLocation(TestLocation);
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Capsule restored, moved up by %.1f"), HeightDifference);
		}
		else
		{
			// No room - restore capsule anyway but stay at current Z
			// (will result in slight pop, but better than staying small)
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: No room to stand after slide - forcing restore"));
			CapsuleComponent->SetCapsuleHalfHeight(OriginalCapsuleHalfHeight);
			// Still move up to avoid clipping through floor
			OwnerCharacter->SetActorLocation(TestLocation);
		}
	}
}

// ==================== Double Jump Implementation ====================

bool UExoMovementComponent::CanDoubleJump() const
{
	// Must not be in another special state (except maybe already jumping)
	if (CurrentState != EExoMovementState::None && CurrentState != EExoMovementState::DoubleJumping)
	{
		return false;
	}

	// Must be in air
	if (!IsInAir())
	{
		return false;
	}

	// Must have double jump available
	if (!bCanDoubleJump)
	{
		return false;
	}

	// Must have enough stamina
	if (HealthComponent && !HealthComponent->HasStamina(DoubleJumpStaminaCost))
	{
		return false;
	}

	return true;
}

bool UExoMovementComponent::TryDoubleJump()
{
	if (!CanDoubleJump())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Cannot double jump - preconditions not met"));
		return false;
	}

	// Consume stamina
	if (HealthComponent)
	{
		if (!HealthComponent->UseStamina(DoubleJumpStaminaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Cannot double jump - not enough stamina"));
			return false;
		}
	}

	// Use up double jump
	bCanDoubleJump = false;

	// Apply upward force
	if (MovementComponent)
	{
		// Reset vertical velocity first for consistent jump height
		FVector Velocity = MovementComponent->Velocity;
		Velocity.Z = 0.0f;
		MovementComponent->Velocity = Velocity;

		// Apply jump force
		MovementComponent->AddImpulse(FVector(0.0f, 0.0f, DoubleJumpForce), true);
	}

	// Play montage
	if (DoubleJumpMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Play(DoubleJumpMontage);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: No double jump montage set"));
	}

	SetState(EExoMovementState::DoubleJumping);
	OnDoubleJumpExecuted.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Double jump executed"));

	// Reset state after a short time (montage handles visuals)
	FTimerHandle ResetTimer;
	GetWorld()->GetTimerManager().SetTimer(
		ResetTimer,
		[this]()
		{
			if (CurrentState == EExoMovementState::DoubleJumping)
			{
				SetState(EExoMovementState::None);
			}
		},
		0.3f,
		false
	);

	return true;
}

void UExoMovementComponent::ResetDoubleJump()
{
	bCanDoubleJump = true;

	// Also reset state if we were double jumping
	if (CurrentState == EExoMovementState::DoubleJumping)
	{
		SetState(EExoMovementState::None);
	}

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Double jump reset"));
}

// ==================== Ledge Grab Implementation ====================

bool UExoMovementComponent::CanLedgeGrab() const
{
	// Must not be in another state
	if (CurrentState != EExoMovementState::None && CurrentState != EExoMovementState::DoubleJumping)
	{
		return false;
	}

	// Must not be on cooldown
	if (LedgeGrabCooldownTimer > 0.0f)
	{
		return false;
	}

	// Must be in air
	if (!IsInAir())
	{
		return false;
	}

	// Must have enough stamina
	if (HealthComponent && !HealthComponent->HasStamina(LedgeGrabStaminaCost))
	{
		return false;
	}

	return true;
}

bool UExoMovementComponent::DetectLedge(FVector& OutLedgeLocation, FVector& OutLedgeNormal)
{
	if (!OwnerCharacter || !CapsuleComponent)
	{
		return false;
	}

	FVector CharLocation = OwnerCharacter->GetActorLocation();
	FVector CharForward = OwnerCharacter->GetActorForwardVector();
	float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
	float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	float TraceDistance = LedgeDetectionForward + CapsuleRadius + 20.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	// ========== STEP 1: Trace at HEAD height - should HIT a wall ==========
	FVector HeadHeight = CharLocation + FVector(0, 0, CapsuleHalfHeight * 0.8f);
	FVector HeadTraceEnd = HeadHeight + CharForward * TraceDistance;

	if (bDebugLedgeDetection)
	{
		DrawDebugLine(GetWorld(), HeadHeight, HeadTraceEnd, FColor::Red, false, 0.1f, 0, 3.0f);
	}

	FHitResult HeadHit;
	bool bHeadBlocked = GetWorld()->LineTraceSingleByChannel(HeadHit, HeadHeight, HeadTraceEnd, ECC_WorldStatic, QueryParams);

	if (!bHeadBlocked)
	{
		// No wall at head height - no ledge to grab
		return false;
	}

	if (bDebugLedgeDetection)
	{
		DrawDebugSphere(GetWorld(), HeadHit.Location, 8.0f, 6, FColor::Red, false, 0.1f);
	}

	// ========== STEP 2: Trace ABOVE head - should be CLEAR (empty space) ==========
	// This confirms there's a ledge (wall below, empty above)
	float AboveHeadHeight = CapsuleHalfHeight + 50.0f; // Well above the head
	FVector AboveHead = CharLocation + FVector(0, 0, AboveHeadHeight);
	FVector AboveTraceEnd = AboveHead + CharForward * TraceDistance;

	if (bDebugLedgeDetection)
	{
		DrawDebugLine(GetWorld(), AboveHead, AboveTraceEnd, FColor::Green, false, 0.1f, 0, 3.0f);
	}

	FHitResult AboveHit;
	bool bAboveBlocked = GetWorld()->LineTraceSingleByChannel(AboveHit, AboveHead, AboveTraceEnd, ECC_WorldStatic, QueryParams);

	if (bAboveBlocked)
	{
		// Wall continues above head - no ledge, just a tall wall
		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Ledge - Wall continues above, no ledge"));
		}
		return false;
	}

	// ========== STEP 3: Trace DOWN from above to find the ledge surface ==========
	// Start from the "above" trace end point and trace down
	FVector LedgeTraceStart = AboveTraceEnd;
	FVector LedgeTraceEnd = LedgeTraceStart - FVector(0, 0, AboveHeadHeight + CapsuleHalfHeight);

	if (bDebugLedgeDetection)
	{
		DrawDebugLine(GetWorld(), LedgeTraceStart, LedgeTraceEnd, FColor::Cyan, false, 0.1f, 0, 3.0f);
	}

	FHitResult LedgeHit;
	bool bFoundLedge = GetWorld()->LineTraceSingleByChannel(LedgeHit, LedgeTraceStart, LedgeTraceEnd, ECC_WorldStatic, QueryParams);

	if (!bFoundLedge)
	{
		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Ledge - No surface found when tracing down"));
		}
		return false;
	}

	if (bDebugLedgeDetection)
	{
		DrawDebugSphere(GetWorld(), LedgeHit.Location, 12.0f, 8, FColor::Blue, false, 0.1f);
	}

	// ========== STEP 4: Verify surface is HORIZONTAL (walkable) ==========
	if (LedgeHit.ImpactNormal.Z < 0.7f)
	{
		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Ledge - Surface not horizontal (normal.Z=%.2f)"), LedgeHit.ImpactNormal.Z);
		}
		return false;
	}

	// ========== STEP 5: Verify ledge is at grabbable height ==========
	float LedgeZ = LedgeHit.Location.Z;
	float CharFeetZ = CharLocation.Z - CapsuleHalfHeight;
	float LedgeHeightAboveFeet = LedgeZ - CharFeetZ;

	// Ledge should be roughly at chest-to-overhead height
	float MinHeight = CapsuleHalfHeight * 0.5f;  // At least half body height
	float MaxHeight = CapsuleHalfHeight * 2.0f + LedgeDetectionHeight;

	if (bDebugLogging)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Ledge at height %.1f above feet (range: %.1f - %.1f)"),
			LedgeHeightAboveFeet, MinHeight, MaxHeight);
	}

	if (LedgeHeightAboveFeet < MinHeight || LedgeHeightAboveFeet > MaxHeight)
	{
		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Ledge height out of range"));
		}
		return false;
	}

	// ========== STEP 6: Verify room to stand on ledge ==========
	FVector StandLocation = LedgeHit.Location + FVector(0, 0, OriginalCapsuleHalfHeight + 5.0f);

	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, OriginalCapsuleHalfHeight);

	if (bDebugLedgeDetection)
	{
		DrawDebugCapsule(GetWorld(), StandLocation, OriginalCapsuleHalfHeight, CapsuleRadius,
			FQuat::Identity, FColor::Purple, false, 0.1f);
	}

	if (GetWorld()->OverlapBlockingTestByChannel(StandLocation, FQuat::Identity, ECC_Pawn, CapsuleShape, QueryParams))
	{
		if (bDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: No room to stand on ledge"));
		}
		return false;
	}

	// ========== SUCCESS ==========
	OutLedgeLocation = LedgeHit.Location;
	OutLedgeNormal = HeadHit.ImpactNormal; // Use wall normal from head trace

	if (bDebugLedgeDetection)
	{
		DrawDebugSphere(GetWorld(), LedgeHit.Location, 20.0f, 12, FColor::Green, false, 0.5f);
	}

	if (bDebugLogging)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: LEDGE FOUND at Z=%.1f!"), LedgeZ);
	}

	return true;
}

bool UExoMovementComponent::TryLedgeGrab()
{
	if (!CanLedgeGrab())
	{
		return false;
	}

	FVector DetectedLedgeLocation;
	FVector DetectedLedgeNormal;

	if (!DetectLedge(DetectedLedgeLocation, DetectedLedgeNormal))
	{
		return false;
	}

	// Consume stamina
	if (HealthComponent)
	{
		if (!HealthComponent->UseStamina(LedgeGrabStaminaCost))
		{
			return false;
		}
	}

	LedgeLocation = DetectedLedgeLocation;
	LedgeNormal = DetectedLedgeNormal;

	// Snap to ledge
	SnapToLedge();

	// Play ledge grab montage
	if (LedgeGrabMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Play(LedgeGrabMontage);
		}
	}

	SetState(EExoMovementState::LedgeGrabbing);
	OnLedgeGrabbed.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Ledge grabbed at location (%.1f, %.1f, %.1f)"),
		LedgeLocation.X, LedgeLocation.Y, LedgeLocation.Z);

	return true;
}

void UExoMovementComponent::SnapToLedge()
{
	if (!OwnerCharacter || !MovementComponent || !CapsuleComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ExoMovement: SnapToLedge - missing required components!"));
		return;
	}

	// Preserve camera state before modifying character position
	PreserveCameraState();

	float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();

	// Position character hanging from ledge:
	// - Horizontally: offset from wall by capsule radius PLUS extra space
	// - Vertically: hands at ledge height (so character center is below ledge)
	FVector HangPosition = LedgeLocation;
	// IMPORTANT: Push character OUT from wall significantly to avoid clipping
	// Need large offset because LedgeLocation is ON the ledge surface, not at wall face
	float WallOffset = CapsuleRadius + 80.0f; // Large offset to clear wall geometry
	HangPosition += LedgeNormal * WallOffset;
	HangPosition.Z = LedgeLocation.Z - OriginalCapsuleHalfHeight * 0.6f; // Hands at ledge level

	// Use teleport to avoid physics interference
	OwnerCharacter->SetActorLocation(HangPosition, false, nullptr, ETeleportType::TeleportPhysics);

	// Face the wall (looking into the ledge)
	FRotator FaceRotation = (-LedgeNormal).Rotation();
	FaceRotation.Pitch = 0;
	FaceRotation.Roll = 0;
	OwnerCharacter->SetActorRotation(FaceRotation);

	// CRITICAL: Completely disable movement to prevent any interference
	MovementComponent->DisableMovement();
	MovementComponent->StopMovementImmediately();
	MovementComponent->GravityScale = 0.0f;
	MovementComponent->Velocity = FVector::ZeroVector;

	// Store target location for mantle - standing ON the ledge
	MantleTargetLocation = LedgeLocation + FVector(0, 0, OriginalCapsuleHalfHeight + 5.0f);
	// Move forward onto the ledge (away from the wall edge)
	MantleTargetLocation -= LedgeNormal * (CapsuleRadius + 20.0f);

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Snapped to ledge at (%.1f, %.1f, %.1f), mantle target at (%.1f, %.1f, %.1f)"),
		HangPosition.X, HangPosition.Y, HangPosition.Z,
		MantleTargetLocation.X, MantleTargetLocation.Y, MantleTargetLocation.Z);
}

bool UExoMovementComponent::TryMantle()
{
	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: TryMantle called, CurrentState=%d"), (int32)CurrentState);

	if (CurrentState != EExoMovementState::LedgeGrabbing)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: TryMantle FAILED - not in LedgeGrabbing state"));
		return false;
	}

	if (!OwnerCharacter || !MovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: TryMantle FAILED - missing character or movement"));
		return false;
	}

	// Check stamina for mantle (skip check if no HealthComponent)
	if (HealthComponent)
	{
		if (!HealthComponent->HasStamina(MantleStaminaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Cannot mantle - not enough stamina"));
			return false;
		}
		HealthComponent->UseStamina(MantleStaminaCost);
	}

	MantleTimer = 0.0f;
	MantleStartLocation = OwnerCharacter->GetActorLocation();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Mantle starting from (%.1f, %.1f, %.1f) to target (%.1f, %.1f, %.1f)"),
		MantleStartLocation.X, MantleStartLocation.Y, MantleStartLocation.Z,
		MantleTargetLocation.X, MantleTargetLocation.Y, MantleTargetLocation.Z);

	// CRITICAL: Disable movement component to prevent interference during mantle
	MovementComponent->DisableMovement();
	MovementComponent->StopMovementImmediately();
	MovementComponent->Velocity = FVector::ZeroVector;

	// Stop ledge grab montage and play mantle montage
	if (LedgeGrabMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, LedgeGrabMontage);
		}
	}

	if (MantleMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Play(MantleMontage);
		}
	}

	SetState(EExoMovementState::Mantling);
	OnMantleStarted.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Mantle started - movement disabled"));

	return true;
}

void UExoMovementComponent::UpdateMantle(float DeltaTime)
{
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("ExoMovement: UpdateMantle - NO OWNER CHARACTER!"));
		EndMantle();
		return;
	}

	MantleTimer += DeltaTime;
	float Progress = FMath::Clamp(MantleTimer / MantleDuration, 0.0f, 1.0f);

	// Calculate target position above ledge (where we rise to)
	FVector AboveLedge = LedgeLocation + FVector(0, 0, OriginalCapsuleHalfHeight + 20.0f);

	FVector NewLocation;

	// Simple arc movement: up to ledge level, then forward onto ledge
	// Phase 1 (0-0.6): Move up to ledge height
	// Phase 2 (0.6-1.0): Move forward onto ledge

	if (Progress < 0.6f)
	{
		// Moving up - interpolate from start to above the ledge
		float UpProgress = Progress / 0.6f;
		float EasedUp = FMath::InterpEaseOut(0.0f, 1.0f, UpProgress, 2.0f);
		NewLocation = FMath::Lerp(MantleStartLocation, AboveLedge, EasedUp);
	}
	else
	{
		// Moving forward onto ledge
		float ForwardProgress = (Progress - 0.6f) / 0.4f;
		float EasedForward = FMath::InterpEaseOut(0.0f, 1.0f, ForwardProgress, 2.0f);
		NewLocation = FMath::Lerp(AboveLedge, MantleTargetLocation, EasedForward);
	}

	// Debug logging every few frames
	static int32 FrameCount = 0;
	FrameCount++;
	if (bDebugLogging && (FrameCount % 10 == 0))
	{
		FVector CurrentLoc = OwnerCharacter->GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Mantle progress=%.2f, Current Z=%.1f, Target Z=%.1f, New Z=%.1f"),
			Progress, CurrentLoc.Z, MantleTargetLocation.Z, NewLocation.Z);
	}

	// Force set location - use teleport to avoid collision interference
	bool bSuccess = OwnerCharacter->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (!bSuccess && bDebugLogging)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: SetActorLocation FAILED during mantle!"));
	}

	// Clear any velocity that might have accumulated
	if (MovementComponent)
	{
		MovementComponent->Velocity = FVector::ZeroVector;
	}

	if (Progress >= 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Mantle complete, calling EndMantle"));
		EndMantle();
	}
}

void UExoMovementComponent::EndMantle()
{
	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: EndMantle called"));

	if (!MovementComponent || !OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("ExoMovement: EndMantle - missing MovementComponent or OwnerCharacter!"));
		SetState(EExoMovementState::None);
		return;
	}

	// Stop any playing montages
	if (MantleMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.2f, MantleMontage);
		}
	}

	// Position character at final target location
	OwnerCharacter->SetActorLocation(MantleTargetLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// CRITICAL: Restore movement component fully
	MovementComponent->GravityScale = 1.0f;
	MovementComponent->Velocity = FVector::ZeroVector;

	// Re-enable movement first
	MovementComponent->SetMovementMode(MOVE_Walking);

	// Force a physics update to ensure ground detection
	MovementComponent->UpdateComponentVelocity();

	// Check if we're actually on ground
	FHitResult GroundHit;
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start - FVector(0, 0, 50.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	bool bOnGround = GetWorld()->LineTraceSingleByChannel(GroundHit, Start, End, ECC_WorldStatic, QueryParams);

	if (bOnGround)
	{
		// Snap to ground surface
		FVector GroundLocation = GroundHit.Location + FVector(0, 0, CapsuleComponent ? CapsuleComponent->GetUnscaledCapsuleHalfHeight() : 88.0f);
		OwnerCharacter->SetActorLocation(GroundLocation, false, nullptr, ETeleportType::TeleportPhysics);
		MovementComponent->SetMovementMode(MOVE_Walking);
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Mantle complete - snapped to ground at Z=%.1f"), GroundLocation.Z);
	}
	else
	{
		// No ground - fall
		MovementComponent->SetMovementMode(MOVE_Falling);
		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Mantle complete - no ground, falling"));
	}

	// Reset double jump since we're effectively landing
	ResetDoubleJump();

	// Set cooldown to prevent immediate re-grab
	LedgeGrabCooldownTimer = LedgeGrabCooldown;

	// Restore camera settings
	RestoreCameraState();

	// Update state LAST
	SetState(EExoMovementState::None);
	OnMantleComplete.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Mantle fully complete - movement mode: %d, gravity: %.1f"),
		(int32)MovementComponent->MovementMode.GetValue(), MovementComponent->GravityScale);
}

void UExoMovementComponent::ReleaseLedge()
{
	if (CurrentState != EExoMovementState::LedgeGrabbing)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: ReleaseLedge called"));

	// Stop ledge grab montage
	if (LedgeGrabMontage)
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.2f, LedgeGrabMontage);
		}
	}

	// CRITICAL: Fully restore movement component
	if (MovementComponent)
	{
		MovementComponent->GravityScale = 1.0f;
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Falling);

		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Restored - mode=Falling, gravity=1.0"));
	}

	// Set cooldown to prevent immediate re-grab
	LedgeGrabCooldownTimer = LedgeGrabCooldown;

	SetState(EExoMovementState::None);
	OnLedgeReleased.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Ledge released - player should now fall"));

	// Restore camera settings
	RestoreCameraState();
}

// ==================== Camera State Management ====================

void UExoMovementComponent::PreserveCameraState()
{
	if (!SpringArmComponent)
	{
		// Try to find it again if we missed it
		if (OwnerCharacter)
		{
			SpringArmComponent = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
		}
	}

	if (SpringArmComponent)
	{
		// Store current settings
		bCameraLagWasEnabled = SpringArmComponent->bEnableCameraLag;
		OriginalCameraLagSpeed = SpringArmComponent->CameraLagSpeed;
		OriginalSocketOffset = SpringArmComponent->SocketOffset;
		OriginalTargetArmLength = SpringArmComponent->TargetArmLength;

		// Disable camera lag during ledge grab to prevent camera going inside character
		SpringArmComponent->bEnableCameraLag = false;

		// Optionally increase arm length slightly to pull camera back
		SpringArmComponent->TargetArmLength = OriginalTargetArmLength + 50.0f;

		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Camera state preserved - disabled lag, extended arm"));
	}
}

void UExoMovementComponent::RestoreCameraState()
{
	if (SpringArmComponent)
	{
		// Restore original camera settings
		SpringArmComponent->bEnableCameraLag = bCameraLagWasEnabled;
		SpringArmComponent->CameraLagSpeed = OriginalCameraLagSpeed;
		SpringArmComponent->SocketOffset = OriginalSocketOffset;
		SpringArmComponent->TargetArmLength = OriginalTargetArmLength;

		UE_LOG(LogTemp, Warning, TEXT("ExoMovement: Camera state restored - lag=%s, armLength=%.1f"),
			bCameraLagWasEnabled ? TEXT("true") : TEXT("false"), OriginalTargetArmLength);
	}
}
