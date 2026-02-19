// CallOfTheMoutains - Gore Trail Component Implementation

#include "GoreTrailComponent.h"
#include "Components/DecalComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"

UGoreTrailComponent::UGoreTrailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGoreTrailComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize last spawn location
	if (GetOwner())
	{
		LastSpawnLocation = GetOwner()->GetActorLocation();
	}

	bInitialized = true;
}

void UGoreTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bTrailActive || !GetOwner())
	{
		return;
	}

	FVector CurrentLocation = GetOwner()->GetActorLocation();

	// Calculate distance moved
	float DistanceMoved = FVector::Dist(CurrentLocation, LastSpawnLocation);
	DistanceTraveled += DistanceMoved;

	// Check if we should spawn a new decal
	if (DistanceTraveled >= DecalSpawnDistance)
	{
		SpawnGoreDecal(CurrentLocation);
		DistanceTraveled = 0.0f;
		LastSpawnLocation = CurrentLocation;
	}
	else
	{
		// Update last location for next frame's distance calculation
		LastSpawnLocation = CurrentLocation;
	}
}

void UGoreTrailComponent::SetTrailActive(bool bActive)
{
	if (bTrailActive == bActive)
	{
		return;
	}

	bTrailActive = bActive;

	// Enable/disable tick
	SetComponentTickEnabled(bActive);

	if (bActive)
	{
		// Reset tracking
		if (GetOwner())
		{
			LastSpawnLocation = GetOwner()->GetActorLocation();
		}
		DistanceTraveled = 0.0f;

		// Start particle trail if configured
		if (bUseParticleTrail && GoreParticleSystem && !ActiveParticleComponent)
		{
			ActiveParticleComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				GoreParticleSystem,
				GetOwner()->GetRootComponent(),
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true
			);
		}
	}
	else
	{
		// Stop particle trail
		if (ActiveParticleComponent)
		{
			ActiveParticleComponent->Deactivate();
			ActiveParticleComponent = nullptr;
		}
	}
}

void UGoreTrailComponent::ForceSpawnDecal()
{
	if (GetOwner())
	{
		SpawnGoreDecal(GetOwner()->GetActorLocation());
	}
}

void UGoreTrailComponent::SpawnGoreDecal(FVector Location)
{
	if (GoreDecalMaterials.Num() == 0)
	{
		return;
	}

	// Trace to find ground
	FVector GroundLocation;
	FVector GroundNormal;

	if (!TraceToGround(Location, GroundLocation, GroundNormal))
	{
		// Fallback to actor location if trace fails
		GroundLocation = Location;
		GroundLocation.Z -= 90.0f; // Approximate ground
		GroundNormal = FVector::UpVector;
	}

	// Select random material
	int32 MaterialIndex = FMath::RandRange(0, GoreDecalMaterials.Num() - 1);
	UMaterialInterface* DecalMaterial = GoreDecalMaterials[MaterialIndex];

	if (!DecalMaterial)
	{
		return;
	}

	// Calculate rotation - decals project downward, so we need to orient correctly
	FRotator DecalRotation = GroundNormal.Rotation();
	DecalRotation.Pitch -= 90.0f; // Adjust for decal projection direction

	// Add random rotation around the normal
	float RandomYaw = FMath::RandRange(-RandomRotationRange, RandomRotationRange);
	DecalRotation.Yaw += RandomYaw;

	// Spawn the decal
	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
		GetWorld(),
		DecalMaterial,
		DecalSize,
		GroundLocation + FVector(0, 0, GroundOffset),
		DecalRotation,
		DecalLifetime
	);

	if (Decal)
	{
		// Configure decal
		Decal->SetFadeScreenSize(0.001f); // Keep visible at distance
	}
}

bool UGoreTrailComponent::TraceToGround(FVector StartLocation, FVector& OutGroundLocation, FVector& OutGroundNormal)
{
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FVector TraceStart = StartLocation;
	FVector TraceEnd = StartLocation - FVector(0.0f, 0.0f, 500.0f); // Trace 500 units down

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		OutGroundLocation = HitResult.ImpactPoint;
		OutGroundNormal = HitResult.ImpactNormal;
		return true;
	}

	return false;
}
