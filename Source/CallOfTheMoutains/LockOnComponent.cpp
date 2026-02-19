// CallOfTheMoutains - Lock On Component for Souls-like targeting

#include "LockOnComponent.h"
#include "TargetableComponent.h"
#include "HealthComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Check if current target is still valid
	if (CurrentTarget && !IsTargetValid())
	{
		// If target died and auto-retarget is enabled, try to find a nearby target
		if (bAutoRetargetOnDeath && IsTargetDead())
		{
			if (!TrySwitchToNearbyTarget())
			{
				ReleaseLockOn();
			}
		}
		else
		{
			// Target became invalid for other reasons (out of range, etc.)
			ReleaseLockOn();
		}
	}

	// Debug drawing
	if (bDebugDraw && CurrentTarget && GetTraceOwner())
	{
		FVector TargetLoc = GetTargetLookAtLocation();
		DrawDebugLine(GetWorld(), GetTraceOwner()->GetActorLocation(), TargetLoc, FColor::Red, false, -1.0f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), TargetLoc, 25.0f, 12, FColor::Red, false, -1.0f, 0, 2.0f);
	}
}

void ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ReleaseLockOn();
	}
	else
	{
		AActor* BestTarget = FindBestTarget();
		if (BestTarget)
		{
			LockOnToTarget(BestTarget);
		}
	}
}

bool ULockOnComponent::LockOnToTarget(AActor* Target)
{
	if (!Target)
	{
		return false;
	}

	UTargetableComponent* TargetComp = Target->FindComponentByClass<UTargetableComponent>();
	if (!TargetComp || !TargetComp->IsTargetable())
	{
		return false;
	}

	// Release previous target
	if (CurrentTarget)
	{
		ReleaseLockOn();
	}

	// Set new target
	CurrentTarget = Target;
	CurrentTargetComponent = TargetComp;

	// Notify the target
	TargetComp->NotifyTargeted();

	// Broadcast event
	OnLockOnAcquired.Broadcast(CurrentTarget);

	return true;
}

void ULockOnComponent::ReleaseLockOn()
{
	if (CurrentTargetComponent)
	{
		CurrentTargetComponent->NotifyTargetLost();
	}

	AActor* OldTarget = CurrentTarget;
	CurrentTarget = nullptr;
	CurrentTargetComponent = nullptr;

	if (OldTarget)
	{
		OnLockOnLost.Broadcast(OldTarget);
	}
}

void ULockOnComponent::SwitchTarget(float Direction)
{
	if (!IsLockedOn() || !CurrentTarget)
	{
		return;
	}

	TArray<AActor*> AllTargets = FindAllTargetsInRange();

	// Remove current target from list
	AllTargets.Remove(CurrentTarget);

	if (AllTargets.Num() == 0)
	{
		return;
	}

	// Simple cycling: just pick the next target in the list
	// For directional switching, sort by angle to camera right vector
	AActor* TraceOwner = GetTraceOwner();
	if (!TraceOwner)
	{
		return;
	}

	FVector PlayerLocation = TraceOwner->GetActorLocation();
	FVector PlayerRight = TraceOwner->GetActorRightVector();
	FVector ToCurrentTarget = (CurrentTarget->GetActorLocation() - PlayerLocation).GetSafeNormal();

	AActor* BestTarget = AllTargets[0]; // Default to first
	float BestScore = -FLT_MAX;

	for (AActor* Target : AllTargets)
	{
		FVector ToTarget = (Target->GetActorLocation() - PlayerLocation).GetSafeNormal();

		// Score based on how much to the right (or left) this target is relative to current
		float AngleScore = FVector::DotProduct(ToTarget - ToCurrentTarget, PlayerRight) * Direction;

		// Distance tiebreaker - prefer closer
		float Distance = FVector::Dist(PlayerLocation, Target->GetActorLocation());
		float Score = AngleScore - (Distance * 0.0001f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Target;
		}
	}

	if (BestTarget)
	{
		LockOnToTarget(BestTarget);
	}
}

FVector ULockOnComponent::GetTargetLookAtLocation() const
{
	if (CurrentTargetComponent)
	{
		return CurrentTargetComponent->GetTargetLocation() + CameraTargetOffset;
	}
	return FVector::ZeroVector;
}

FRotator ULockOnComponent::GetRotationToTarget() const
{
	if (!CurrentTarget || !GetTraceOwner())
	{
		return FRotator::ZeroRotator;
	}

	FVector Start = GetTraceOwner()->GetActorLocation();
	FVector End = GetTargetLookAtLocation();

	return UKismetMathLibrary::FindLookAtRotation(Start, End);
}

void ULockOnComponent::SetOwnerActor(AActor* NewOwner)
{
	OverrideOwner = NewOwner;
}

AActor* ULockOnComponent::GetTraceOwner() const
{
	return OverrideOwner ? OverrideOwner : GetOwner();
}

AActor* ULockOnComponent::FindBestTarget() const
{
	TArray<AActor*> AllTargets = FindAllTargetsInRange();

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	AActor* TraceOwner = GetTraceOwner();
	if (!TraceOwner)
	{
		return nullptr;
	}

	FVector PlayerLocation = TraceOwner->GetActorLocation();
	FVector PlayerForward = TraceOwner->GetActorForwardVector();

	// If we have a camera, use its forward instead
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(TraceOwner))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
		{
			PlayerForward = PC->GetControlRotation().Vector();
		}
	}

	for (AActor* Target : AllTargets)
	{
		UTargetableComponent* TargetComp = Target->FindComponentByClass<UTargetableComponent>();
		if (!TargetComp)
		{
			continue;
		}

		FVector ToTarget = Target->GetActorLocation() - PlayerLocation;
		float Distance = ToTarget.Size();
		ToTarget.Normalize();

		// Calculate angle from forward
		float DotProduct = FVector::DotProduct(PlayerForward, ToTarget);
		float Angle = FMath::Acos(DotProduct) * (180.0f / PI);

		// Skip if outside max angle
		if (Angle > MaxLockOnAngle)
		{
			continue;
		}

		// Score: prioritize centered targets, then closer ones
		float AngleScore = 1.0f - (Angle / MaxLockOnAngle);
		float DistanceScore = 1.0f - (Distance / MaxLockOnDistance);
		float PriorityScore = TargetComp->TargetPriority / 100.0f;

		float TotalScore = (AngleScore * 0.5f) + (DistanceScore * 0.3f) + (PriorityScore * 0.2f);

		if (TotalScore > BestScore)
		{
			BestScore = TotalScore;
			BestTarget = Target;
		}
	}

	return BestTarget;
}

TArray<AActor*> ULockOnComponent::FindAllTargetsInRange() const
{
	TArray<AActor*> ValidTargets;

	AActor* TraceOwner = GetTraceOwner();
	if (!TraceOwner)
	{
		return ValidTargets;
	}

	FVector PlayerLocation = TraceOwner->GetActorLocation();

	// Use ObjectType query to find pawns AND world dynamic actors (like Endless Front BearCharacter does)
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(MaxLockOnDistance);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(TraceOwner);
	QueryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		PlayerLocation,
		FQuat::Identity,
		ObjectParams,
		Shape,
		QueryParams);

	if (bHit)
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (!OverlapActor)
			{
				continue;
			}

			// Check for targetable component
			UTargetableComponent* TargetComp = OverlapActor->FindComponentByClass<UTargetableComponent>();
			if (!TargetComp || !TargetComp->IsTargetable())
			{
				continue;
			}

			ValidTargets.Add(OverlapActor);
		}
	}

	return ValidTargets;
}

bool ULockOnComponent::IsTargetValid() const
{
	if (!CurrentTarget || !IsValid(CurrentTarget))
	{
		return false;
	}

	if (!CurrentTargetComponent || !CurrentTargetComponent->IsTargetable())
	{
		return false;
	}

	AActor* TraceOwner = GetTraceOwner();
	if (!TraceOwner)
	{
		return false;
	}

	// Check distance
	float Distance = FVector::Dist(TraceOwner->GetActorLocation(), CurrentTarget->GetActorLocation());
	if (Distance > BreakLockDistance)
	{
		return false;
	}

	return true;
}

float ULockOnComponent::GetAngleToTarget(AActor* Target) const
{
	AActor* TraceOwner = GetTraceOwner();
	if (!Target || !TraceOwner)
	{
		return 180.0f;
	}

	FVector ToTarget = (Target->GetActorLocation() - TraceOwner->GetActorLocation()).GetSafeNormal();
	FVector Forward = TraceOwner->GetActorForwardVector();

	float DotProduct = FVector::DotProduct(Forward, ToTarget);
	return FMath::Acos(DotProduct) * (180.0f / PI);
}

bool ULockOnComponent::HasLineOfSightTo(AActor* Target) const
{
	AActor* TraceOwner = GetTraceOwner();
	if (!Target || !TraceOwner)
	{
		return false;
	}

	FVector Start = TraceOwner->GetActorLocation() + FVector(0, 0, 50.0f); // Eye level
	FVector End = Target->GetActorLocation() + FVector(0, 0, 50.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(TraceOwner);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(Target);

	// Also ignore current target so we can switch to nearby targets
	if (CurrentTarget)
	{
		QueryParams.AddIgnoredActor(CurrentTarget);
	}

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// No hit means clear line of sight
	return !bHit;
}

bool ULockOnComponent::IsTargetDead() const
{
	if (!CurrentTarget || !IsValid(CurrentTarget))
	{
		return true; // Treat destroyed actors as dead
	}

	// Check if target has a HealthComponent and is dead
	UHealthComponent* HealthComp = CurrentTarget->FindComponentByClass<UHealthComponent>();
	if (HealthComp && HealthComp->IsDead())
	{
		return true;
	}

	// Also check if targetable component is no longer targetable (could indicate death)
	if (CurrentTargetComponent && !CurrentTargetComponent->IsTargetable())
	{
		return true;
	}

	return false;
}

bool ULockOnComponent::TrySwitchToNearbyTarget()
{
	AActor* TraceOwner = GetTraceOwner();
	if (!TraceOwner)
	{
		return false;
	}

	// Get all valid targets in range (excluding the current one)
	TArray<AActor*> AllTargets = FindAllTargetsInRange();
	AllTargets.Remove(CurrentTarget);

	if (AllTargets.Num() == 0)
	{
		return false;
	}

	// Find the closest target to the current target's position (not to the player)
	// This gives a more natural "next enemy" feel
	FVector CurrentTargetLocation = CurrentTarget ? CurrentTarget->GetActorLocation() : TraceOwner->GetActorLocation();

	AActor* ClosestTarget = nullptr;
	float ClosestDistance = FLT_MAX;

	for (AActor* Target : AllTargets)
	{
		float Distance = FVector::Dist(CurrentTargetLocation, Target->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestTarget = Target;
		}
	}

	if (ClosestTarget)
	{
		return LockOnToTarget(ClosestTarget);
	}

	return false;
}
