// CallOfTheMoutains - Footstep Component with Physical Surface Detection

#include "FootstepComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundBase.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/Engine.h"

UFootstepComponent::UFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFootstepComponent::BeginPlay()
{
	Super::BeginPlay();

	// Try to find movement component on owner
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		MovementComponent = Character->GetCharacterMovement();
	}

	// Initialize footstep tracking location
	if (GetOwner())
	{
		LastFootstepLocation = GetOwner()->GetActorLocation();
	}
}

void UFootstepComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Skip if auto footsteps disabled or no owner
	if (!bAutoFootsteps || !GetOwner())
	{
		return;
	}

	// Check if we have a valid movement component and are grounded
	if (!MovementComponent)
	{
		return;
	}

	// Only play footsteps when on ground and moving
	if (!MovementComponent->IsMovingOnGround())
	{
		bIsMoving = false;
		bWasMoving = false;
		return;
	}

	// Check current speed
	float CurrentSpeed = MovementComponent->Velocity.Size2D();
	bIsMoving = CurrentSpeed > 10.0f; // Small threshold to avoid jitter

	// If we just stopped, reset tracking
	if (bWasMoving && !bIsMoving)
	{
		DistanceTraveled = 0.0f;
	}

	// If we just started moving, set the start location
	if (!bWasMoving && bIsMoving)
	{
		LastFootstepLocation = GetOwner()->GetActorLocation();
		DistanceTraveled = 0.0f;
	}

	bWasMoving = bIsMoving;

	// If not moving, skip
	if (!bIsMoving)
	{
		return;
	}

	// Update crouch state from movement component
	bCrouching = MovementComponent->IsCrouching();

	// Calculate distance traveled since last footstep
	FVector CurrentLocation = GetOwner()->GetActorLocation();
	float FrameDistance = FVector::Dist2D(CurrentLocation, LastFootstepLocation);
	DistanceTraveled += FrameDistance;
	LastFootstepLocation = CurrentLocation;

	// Determine step distance based on movement state
	float RequiredDistance = WalkStepDistance;
	if (bCrouching)
	{
		RequiredDistance = CrouchStepDistance;
	}
	else if (bSprinting)
	{
		RequiredDistance = SprintStepDistance;
	}

	// Play footstep when we've traveled far enough
	if (DistanceTraveled >= RequiredDistance)
	{
		PlayFootstep(CurrentLocation, bNextFootIsRight);
		bNextFootIsRight = !bNextFootIsRight; // Alternate feet
		DistanceTraveled = 0.0f;
	}
}

void UFootstepComponent::PlayFootstep(FVector FootLocation, bool bIsRightFoot)
{
	// Check minimum time between steps
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFootstepTime < MinTimeBetweenSteps)
	{
		return;
	}
	LastFootstepTime = CurrentTime;

	// Use actor location if no foot location provided
	if (FootLocation.IsZero())
	{
		FootLocation = GetOwner()->GetActorLocation();
	}

	// Get surface type directly from trace
	CurrentSurface = TraceSurface(FootLocation);

	// Find appropriate sound set
	const FFootstepSoundSet* SoundSet = SurfaceSounds.Find(CurrentSurface);

	// Check if this sound set has any sounds (left, right, or general)
	auto HasAnySounds = [](const FFootstepSoundSet* Set) -> bool
	{
		if (!Set) return false;
		return Set->LeftFootSounds.Num() > 0 || Set->RightFootSounds.Num() > 0 || Set->FootstepSounds.Num() > 0;
	};

	if (!HasAnySounds(SoundSet))
	{
		// Use default sounds
		SoundSet = &DefaultSounds;
	}

	// Get random sound from set based on which foot
	USoundBase* Sound = GetRandomSound(*SoundSet, bIsRightFoot);
	if (!Sound)
	{
		return;
	}

	// Calculate volume and pitch
	float Volume = CalculateVolume(*SoundSet);
	float Pitch = CalculatePitch(*SoundSet);

	// Play sound at foot location
	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		FootLocation,
		Volume,
		Pitch
	);
}

void UFootstepComponent::PlayLandingSound(float ImpactVelocity)
{
	FVector Location = GetOwner()->GetActorLocation();
	CurrentSurface = TraceSurface(Location);

	// Find appropriate sound set
	const FFootstepSoundSet* SoundSet = SurfaceSounds.Find(CurrentSurface);

	// Check if this sound set has any sounds
	auto HasAnySounds = [](const FFootstepSoundSet* Set) -> bool
	{
		if (!Set) return false;
		return Set->LeftFootSounds.Num() > 0 || Set->RightFootSounds.Num() > 0 || Set->FootstepSounds.Num() > 0;
	};

	if (!HasAnySounds(SoundSet))
	{
		SoundSet = &DefaultSounds;
	}

	// Landing uses both feet, so just pick randomly
	USoundBase* Sound = GetRandomSound(*SoundSet, FMath::RandBool());
	if (!Sound)
	{
		return;
	}

	// Landing is louder based on impact velocity
	float VelocityMultiplier = FMath::GetMappedRangeValueClamped(
		FVector2D(300.0f, 1000.0f),
		FVector2D(1.0f, 1.5f),
		FMath::Abs(ImpactVelocity)
	);

	float Volume = BaseVolume * SoundSet->VolumeMultiplier * VelocityMultiplier;
	float Pitch = CalculatePitch(*SoundSet) * 0.9f; // Slightly lower pitch for landing

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		Location,
		Volume,
		Pitch
	);
}

EPhysicalSurface UFootstepComponent::GetSurfaceAtLocation(FVector Location)
{
	return TraceSurface(Location);
}

EPhysicalSurface UFootstepComponent::TraceSurface(FVector StartLocation)
{
	FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bReturnPhysicalMaterial = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);

	if (bDebugTrace)
	{
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			bHit ? FColor::Green : FColor::Red,
			false,
			1.0f,
			0,
			2.0f
		);

		if (bHit)
		{
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 8, FColor::Yellow, false, 1.0f);
		}
	}

	if (bHit && HitResult.PhysMaterial.IsValid())
	{
		return HitResult.PhysMaterial->SurfaceType;
	}

	return SurfaceType_Default;
}

USoundBase* UFootstepComponent::GetRandomSound(const FFootstepSoundSet& SoundSet, bool bIsRightFoot)
{
	// First try to use foot-specific sounds, with fallbacks
	const TArray<USoundBase*>* SoundsToUse = nullptr;

	if (bIsRightFoot)
	{
		// Right foot: try right -> left -> general
		if (SoundSet.RightFootSounds.Num() > 0)
		{
			SoundsToUse = &SoundSet.RightFootSounds;
		}
		else if (SoundSet.LeftFootSounds.Num() > 0)
		{
			SoundsToUse = &SoundSet.LeftFootSounds;
		}
		else if (SoundSet.FootstepSounds.Num() > 0)
		{
			SoundsToUse = &SoundSet.FootstepSounds;
		}
	}
	else
	{
		// Left foot: try left -> right -> general
		if (SoundSet.LeftFootSounds.Num() > 0)
		{
			SoundsToUse = &SoundSet.LeftFootSounds;
		}
		else if (SoundSet.RightFootSounds.Num() > 0)
		{
			SoundsToUse = &SoundSet.RightFootSounds;
		}
		else if (SoundSet.FootstepSounds.Num() > 0)
		{
			SoundsToUse = &SoundSet.FootstepSounds;
		}
	}

	if (!SoundsToUse || SoundsToUse->Num() == 0)
	{
		return nullptr;
	}

	int32 Index = FMath::RandRange(0, SoundsToUse->Num() - 1);
	return (*SoundsToUse)[Index];
}

float UFootstepComponent::CalculateVolume(const FFootstepSoundSet& SoundSet)
{
	float Volume = BaseVolume * SoundSet.VolumeMultiplier;

	// Apply movement state modifiers
	if (bCrouching)
	{
		Volume *= CrouchVolumeMultiplier;
	}
	else if (bSprinting)
	{
		Volume *= SprintVolumeMultiplier;
	}

	// Check movement component for additional state
	if (MovementComponent)
	{
		// Walking slowly = quieter
		float Speed = MovementComponent->Velocity.Size2D();
		float MaxSpeed = MovementComponent->MaxWalkSpeed;
		if (MaxSpeed > 0.0f && Speed < MaxSpeed * 0.5f)
		{
			Volume *= FMath::GetMappedRangeValueClamped(
				FVector2D(0.0f, MaxSpeed * 0.5f),
				FVector2D(0.5f, 1.0f),
				Speed
			);
		}
	}

	return FMath::Clamp(Volume, 0.0f, 2.0f);
}

float UFootstepComponent::CalculatePitch(const FFootstepSoundSet& SoundSet)
{
	float BasePitch = 1.0f;
	float Variation = SoundSet.PitchVariation;

	return BasePitch + FMath::FRandRange(-Variation, Variation);
}
