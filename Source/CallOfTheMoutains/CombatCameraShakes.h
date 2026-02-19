// CallOfTheMoutains - Combat Camera Shake Classes
// Pre-configured camera shakes for combat feedback using UE5 pattern-based system

#pragma once

#include "CoreMinimal.h"
#include "Shakes/LegacyCameraShake.h"
#include "CombatCameraShakes.generated.h"

/**
 * Light hit camera shake - subtle feedback for light attacks
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API ULightHitCameraShake : public ULegacyCameraShake
{
	GENERATED_BODY()

public:
	ULightHitCameraShake()
	{
		OscillationDuration = 0.15f;
		OscillationBlendInTime = 0.02f;
		OscillationBlendOutTime = 0.1f;

		RotOscillation.Pitch.Amplitude = 0.5f;
		RotOscillation.Pitch.Frequency = 30.0f;
		RotOscillation.Yaw.Amplitude = 0.3f;
		RotOscillation.Yaw.Frequency = 25.0f;
		RotOscillation.Roll.Amplitude = 0.2f;
		RotOscillation.Roll.Frequency = 25.0f;

		LocOscillation.X.Amplitude = 0.5f;
		LocOscillation.X.Frequency = 30.0f;
		LocOscillation.Y.Amplitude = 0.5f;
		LocOscillation.Y.Frequency = 30.0f;
		LocOscillation.Z.Amplitude = 1.0f;
		LocOscillation.Z.Frequency = 30.0f;
	}
};

/**
 * Medium hit camera shake - solid impact feedback
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API UMediumHitCameraShake : public ULegacyCameraShake
{
	GENERATED_BODY()

public:
	UMediumHitCameraShake()
	{
		OscillationDuration = 0.25f;
		OscillationBlendInTime = 0.03f;
		OscillationBlendOutTime = 0.15f;

		RotOscillation.Pitch.Amplitude = 1.5f;
		RotOscillation.Pitch.Frequency = 25.0f;
		RotOscillation.Yaw.Amplitude = 1.0f;
		RotOscillation.Yaw.Frequency = 20.0f;
		RotOscillation.Roll.Amplitude = 0.5f;
		RotOscillation.Roll.Frequency = 20.0f;

		LocOscillation.X.Amplitude = 1.5f;
		LocOscillation.X.Frequency = 25.0f;
		LocOscillation.Y.Amplitude = 1.5f;
		LocOscillation.Y.Frequency = 25.0f;
		LocOscillation.Z.Amplitude = 2.5f;
		LocOscillation.Z.Frequency = 25.0f;

		FOVOscillation.Amplitude = 1.0f;
		FOVOscillation.Frequency = 20.0f;
	}
};

/**
 * Heavy hit camera shake - powerful impact feedback
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API UHeavyHitCameraShake : public ULegacyCameraShake
{
	GENERATED_BODY()

public:
	UHeavyHitCameraShake()
	{
		OscillationDuration = 0.35f;
		OscillationBlendInTime = 0.02f;
		OscillationBlendOutTime = 0.2f;

		RotOscillation.Pitch.Amplitude = 3.0f;
		RotOscillation.Pitch.Frequency = 20.0f;
		RotOscillation.Yaw.Amplitude = 2.0f;
		RotOscillation.Yaw.Frequency = 15.0f;
		RotOscillation.Roll.Amplitude = 1.0f;
		RotOscillation.Roll.Frequency = 15.0f;

		LocOscillation.X.Amplitude = 3.0f;
		LocOscillation.X.Frequency = 20.0f;
		LocOscillation.Y.Amplitude = 3.0f;
		LocOscillation.Y.Frequency = 20.0f;
		LocOscillation.Z.Amplitude = 5.0f;
		LocOscillation.Z.Frequency = 20.0f;

		FOVOscillation.Amplitude = 2.0f;
		FOVOscillation.Frequency = 15.0f;
	}
};

/**
 * Devastating hit camera shake - maximum impact for critical hits, ripostes
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API UDevastatingHitCameraShake : public ULegacyCameraShake
{
	GENERATED_BODY()

public:
	UDevastatingHitCameraShake()
	{
		OscillationDuration = 0.5f;
		OscillationBlendInTime = 0.02f;
		OscillationBlendOutTime = 0.3f;

		RotOscillation.Pitch.Amplitude = 5.0f;
		RotOscillation.Pitch.Frequency = 18.0f;
		RotOscillation.Yaw.Amplitude = 3.0f;
		RotOscillation.Yaw.Frequency = 12.0f;
		RotOscillation.Roll.Amplitude = 2.0f;
		RotOscillation.Roll.Frequency = 12.0f;

		LocOscillation.X.Amplitude = 5.0f;
		LocOscillation.X.Frequency = 18.0f;
		LocOscillation.Y.Amplitude = 5.0f;
		LocOscillation.Y.Frequency = 18.0f;
		LocOscillation.Z.Amplitude = 8.0f;
		LocOscillation.Z.Frequency = 18.0f;

		FOVOscillation.Amplitude = 3.0f;
		FOVOscillation.Frequency = 12.0f;
	}
};

/**
 * Damage taken camera shake - feedback when player receives damage
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API UDamageTakenCameraShake : public ULegacyCameraShake
{
	GENERATED_BODY()

public:
	UDamageTakenCameraShake()
	{
		OscillationDuration = 0.3f;
		OscillationBlendInTime = 0.01f;
		OscillationBlendOutTime = 0.2f;

		// More violent, forward-back motion (emphasize pitch for head snapping back)
		RotOscillation.Pitch.Amplitude = 4.0f;
		RotOscillation.Pitch.Frequency = 15.0f;
		RotOscillation.Yaw.Amplitude = 1.0f;
		RotOscillation.Yaw.Frequency = 20.0f;
		RotOscillation.Roll.Amplitude = 1.0f;
		RotOscillation.Roll.Frequency = 20.0f;

		LocOscillation.X.Amplitude = 4.0f;
		LocOscillation.X.Frequency = 20.0f;
		LocOscillation.Y.Amplitude = 2.0f;
		LocOscillation.Y.Frequency = 25.0f;
		LocOscillation.Z.Amplitude = 3.0f;
		LocOscillation.Z.Frequency = 25.0f;

		FOVOscillation.Amplitude = 2.0f;
		FOVOscillation.Frequency = 20.0f;
	}
};

/**
 * Parry camera shake - sharp, impactful feedback for successful parry
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API UParryCameraShake : public ULegacyCameraShake
{
	GENERATED_BODY()

public:
	UParryCameraShake()
	{
		OscillationDuration = 0.2f;
		OscillationBlendInTime = 0.01f;
		OscillationBlendOutTime = 0.15f;

		// Sharp, quick movement (emphasize yaw for weapon deflection feel)
		RotOscillation.Pitch.Amplitude = 1.5f;
		RotOscillation.Pitch.Frequency = 30.0f;
		RotOscillation.Yaw.Amplitude = 3.0f;
		RotOscillation.Yaw.Frequency = 30.0f;
		RotOscillation.Roll.Amplitude = 1.0f;
		RotOscillation.Roll.Frequency = 30.0f;

		LocOscillation.X.Amplitude = 2.0f;
		LocOscillation.X.Frequency = 35.0f;
		LocOscillation.Y.Amplitude = 4.0f;
		LocOscillation.Y.Frequency = 35.0f;
		LocOscillation.Z.Amplitude = 2.0f;
		LocOscillation.Z.Frequency = 35.0f;

		FOVOscillation.Amplitude = 1.5f;
		FOVOscillation.Frequency = 25.0f;
	}
};
