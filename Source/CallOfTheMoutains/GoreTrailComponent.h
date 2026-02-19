// CallOfTheMoutains - Gore Trail Component
// Spawns blood decals and particles while the owning actor moves

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoreTrailComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UMaterialInterface;

/**
 * Gore Trail Component - Leaves a trail of blood/gore while the owner moves
 *
 * Features:
 * - Spawns decals at regular distance intervals
 * - Optional Niagara particle trail
 * - Random decal rotation for variety
 * - Configurable decal lifetime to prevent accumulation
 */
UCLASS(ClassGroup=(Effects), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UGoreTrailComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGoreTrailComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== Decal Settings ====================

	/** Materials to use for gore decals (randomly selected) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Decals")
	TArray<UMaterialInterface*> GoreDecalMaterials;

	/** Distance traveled before spawning next decal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Decals", meta = (ClampMin = "10.0"))
	float DecalSpawnDistance = 80.0f;

	/** Size of gore decals (width, height, depth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Decals")
	FVector DecalSize = FVector(32.0f, 32.0f, 16.0f);

	/** Lifetime of decals in seconds (0 = permanent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Decals", meta = (ClampMin = "0.0"))
	float DecalLifetime = 30.0f;

	/** Random rotation range for decals (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Decals", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float RandomRotationRange = 180.0f;

	/** Offset from ground for decal spawning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Decals")
	float GroundOffset = 5.0f;

	// ==================== Particle Settings ====================

	/** Optional Niagara particle system for continuous gore trail */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Particles")
	UNiagaraSystem* GoreParticleSystem;

	/** Whether to use particle trail */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore Trail|Particles")
	bool bUseParticleTrail = false;

	// ==================== Control ====================

	/** Is the trail currently active? */
	UPROPERTY(BlueprintReadOnly, Category = "Gore Trail")
	bool bTrailActive = false;

	/** Enable or disable the gore trail */
	UFUNCTION(BlueprintCallable, Category = "Gore Trail")
	void SetTrailActive(bool bActive);

	/** Force spawn a decal at current location */
	UFUNCTION(BlueprintCallable, Category = "Gore Trail")
	void ForceSpawnDecal();

protected:
	/** Spawn a gore decal at the specified location */
	void SpawnGoreDecal(FVector Location);

	/** Trace to find ground position */
	bool TraceToGround(FVector StartLocation, FVector& OutGroundLocation, FVector& OutGroundNormal);

private:
	/** Last location where a decal was spawned */
	FVector LastSpawnLocation;

	/** Distance traveled since last decal */
	float DistanceTraveled = 0.0f;

	/** Active Niagara component for particle trail */
	UPROPERTY()
	UNiagaraComponent* ActiveParticleComponent;

	/** Has the component been initialized */
	bool bInitialized = false;
};
