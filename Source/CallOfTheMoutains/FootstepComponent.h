// CallOfTheMoutains - Footstep Component with Physical Surface Detection

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Chaos/ChaosEngineInterface.h"
#include "FootstepComponent.generated.h"

class USoundBase;
class UCharacterMovementComponent;

/**
 * Footstep sound entry mapping a physical surface to sounds
 */
USTRUCT(BlueprintType)
struct FFootstepSoundSet
{
	GENERATED_BODY()

	/** Left foot sounds (randomly chosen) - if empty, uses FootstepSounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	TArray<USoundBase*> LeftFootSounds;

	/** Right foot sounds (randomly chosen) - if empty, uses FootstepSounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	TArray<USoundBase*> RightFootSounds;

	/** Fallback sounds if left/right not specified (randomly chosen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	TArray<USoundBase*> FootstepSounds;

	/** Volume multiplier for this surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float VolumeMultiplier = 1.0f;

	/** Pitch variation (random range +/-) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float PitchVariation = 0.1f;
};

/**
 * Footstep Component - Plays surface-appropriate footstep sounds
 * Attach to any character (player, NPC, AI) and either:
 * - Enable bAutoFootsteps for distance-based footsteps (no anim notify needed)
 * - Call PlayFootstep from animation notifies for precise foot sync
 *
 * Uses EPhysicalSurface directly from Project Settings > Physics > Physical Surface
 */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UFootstepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFootstepComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ==================== Auto Footstep Settings ====================

	/** Enable automatic footstep sounds based on movement (no animation notify needed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Auto")
	bool bAutoFootsteps = true;

	/** Distance traveled before playing a footstep (walking) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Auto", meta = (ClampMin = "30.0", ClampMax = "200.0", EditCondition = "bAutoFootsteps"))
	float WalkStepDistance = 120.0f;

	/** Distance traveled before playing a footstep (sprinting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Auto", meta = (ClampMin = "50.0", ClampMax = "300.0", EditCondition = "bAutoFootsteps"))
	float SprintStepDistance = 150.0f;

	/** Distance traveled before playing a footstep (crouching) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Auto", meta = (ClampMin = "20.0", ClampMax = "150.0", EditCondition = "bAutoFootsteps"))
	float CrouchStepDistance = 80.0f;

	// ==================== Sound Mappings ====================

	/** Map of physical surface types to footstep sounds (uses Project Settings surfaces) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Sounds")
	TMap<TEnumAsByte<EPhysicalSurface>, FFootstepSoundSet> SurfaceSounds;

	/** Default sounds when surface has no mapping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Sounds")
	FFootstepSoundSet DefaultSounds;

	// ==================== Settings ====================

	/** Base volume for all footsteps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Settings", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BaseVolume = 1.0f;

	/** Volume multiplier when crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrouchVolumeMultiplier = 0.5f;

	/** Volume multiplier when sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Settings", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float SprintVolumeMultiplier = 1.3f;

	/** How far down to trace for surface detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Settings", meta = (ClampMin = "10.0", ClampMax = "500.0"))
	float TraceDistance = 100.0f;

	/** Minimum time between footstep sounds (prevents spam) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinTimeBetweenSteps = 0.25f;

	/** Enable debug drawing of traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Debug")
	bool bDebugTrace = false;

	// ==================== Functions ====================

	/**
	 * Play a footstep sound based on current surface
	 * Call this from animation notifies on foot contact
	 * @param FootLocation - World location of the foot (optional, uses actor location if zero)
	 * @param bIsRightFoot - Which foot (for L/R sound variation)
	 */
	UFUNCTION(BlueprintCallable, Category = "Footstep")
	void PlayFootstep(FVector FootLocation = FVector::ZeroVector, bool bIsRightFoot = true);

	/**
	 * Play a landing sound (heavier impact)
	 * Call this when character lands from a jump/fall
	 * @param ImpactVelocity - How hard the landing was (affects volume)
	 */
	UFUNCTION(BlueprintCallable, Category = "Footstep")
	void PlayLandingSound(float ImpactVelocity = 0.0f);

	/** Get the physical surface type at a location */
	UFUNCTION(BlueprintCallable, Category = "Footstep")
	EPhysicalSurface GetSurfaceAtLocation(FVector Location);

	/** Set whether character is crouching (affects volume) */
	UFUNCTION(BlueprintCallable, Category = "Footstep")
	void SetCrouching(bool bIsCrouching) { bCrouching = bIsCrouching; }

	/** Set whether character is sprinting (affects volume and step distance) */
	UFUNCTION(BlueprintCallable, Category = "Footstep")
	void SetSprinting(bool bIsSprinting) { bSprinting = bIsSprinting; }

	/** Get current detected surface */
	UFUNCTION(BlueprintCallable, Category = "Footstep")
	EPhysicalSurface GetCurrentSurface() const { return CurrentSurface; }

private:
	UPROPERTY()
	UCharacterMovementComponent* MovementComponent;

	bool bCrouching = false;
	bool bSprinting = false;
	float LastFootstepTime = 0.0f;
	EPhysicalSurface CurrentSurface = SurfaceType_Default;

	// Auto footstep tracking
	FVector LastFootstepLocation = FVector::ZeroVector;
	float DistanceTraveled = 0.0f;
	bool bIsMoving = false;
	bool bWasMoving = false;
	bool bNextFootIsRight = true;

	/** Perform surface trace and return the physical surface */
	EPhysicalSurface TraceSurface(FVector StartLocation);

	/** Get a random sound from a sound set for the specified foot */
	USoundBase* GetRandomSound(const FFootstepSoundSet& SoundSet, bool bIsRightFoot = true);

	/** Calculate final volume based on movement state */
	float CalculateVolume(const FFootstepSoundSet& SoundSet);

	/** Calculate pitch with variation */
	float CalculatePitch(const FFootstepSoundSet& SoundSet);
};
