// CallOfTheMoutains - Day/Night Cycle Types and Enums
// Defines time periods, weather types, and related structures

#pragma once

#include "CoreMinimal.h"
#include "DayNightTypes.generated.h"

// ==================== Time Period Enums ====================

/**
 * Time periods throughout the day
 * Used for visual transitions, gameplay modifications, and ambient audio
 */
UENUM(BlueprintType)
enum class ETimePeriod : uint8
{
	Dawn			UMETA(DisplayName = "Dawn"),			// 5:00 - 7:00
	Morning			UMETA(DisplayName = "Morning"),			// 7:00 - 11:00
	Midday			UMETA(DisplayName = "Midday"),			// 11:00 - 14:00
	Afternoon		UMETA(DisplayName = "Afternoon"),		// 14:00 - 17:00
	Dusk			UMETA(DisplayName = "Dusk"),			// 17:00 - 19:00
	Evening			UMETA(DisplayName = "Evening"),			// 19:00 - 21:00
	Night			UMETA(DisplayName = "Night"),			// 21:00 - 3:00
	LateNight		UMETA(DisplayName = "Late Night")		// 3:00 - 5:00
};

// ==================== Weather Enums ====================

/**
 * Weather types for the world
 * Each affects visuals, audio, and potentially gameplay
 */
UENUM(BlueprintType)
enum class EWeatherType : uint8
{
	Clear			UMETA(DisplayName = "Clear"),			// Sunny/clear skies
	Cloudy			UMETA(DisplayName = "Cloudy"),			// Overcast but no precipitation
	LightRain		UMETA(DisplayName = "Light Rain"),		// Drizzle/mist
	HeavyRain		UMETA(DisplayName = "Heavy Rain"),		// Downpour
	Storm			UMETA(DisplayName = "Storm"),			// Thunder and lightning
	Fog				UMETA(DisplayName = "Fog"),				// Heavy fog, reduced visibility
	Snow			UMETA(DisplayName = "Snow")				// Snowfall
};

/**
 * Weather transition state
 */
UENUM(BlueprintType)
enum class EWeatherTransitionState : uint8
{
	Stable			UMETA(DisplayName = "Stable"),			// Weather is constant
	TransitioningIn	UMETA(DisplayName = "Transitioning In"),// New weather building up
	TransitioningOut UMETA(DisplayName = "Transitioning Out")// Current weather fading
};

// ==================== Gameplay Event Enums ====================

/**
 * Special events triggered by day/night cycle
 */
UENUM(BlueprintType)
enum class EDayNightEvent : uint8
{
	None			UMETA(DisplayName = "None"),
	DawnBreak		UMETA(DisplayName = "Dawn Break"),		// First light
	SunRise			UMETA(DisplayName = "Sunrise"),			// Sun appears
	NoonPeak		UMETA(DisplayName = "Noon Peak"),		// Sun at highest
	SunSet			UMETA(DisplayName = "Sunset"),			// Sun disappears
	NightFall		UMETA(DisplayName = "Nightfall"),		// Darkness descends
	Midnight		UMETA(DisplayName = "Midnight"),		// Deepest night
	MoonRise		UMETA(DisplayName = "Moon Rise"),		// Moon appears
	MoonSet			UMETA(DisplayName = "Moon Set")			// Moon disappears
};

// ==================== Structures ====================

/**
 * Time of day representation (24-hour format)
 */
USTRUCT(BlueprintType)
struct CALLOFTHEMOUTAINS_API FCOTMGameTime
{
	GENERATED_BODY()

	/** Hour (0-23) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Time", meta = (ClampMin = "0", ClampMax = "23"))
	int32 Hour = 12;

	/** Minute (0-59) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Time", meta = (ClampMin = "0", ClampMax = "59"))
	int32 Minute = 0;

	/** Day number (for tracking passage of time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Time", meta = (ClampMin = "1"))
	int32 Day = 1;

	FCOTMGameTime() : Hour(12), Minute(0), Day(1) {}
	FCOTMGameTime(int32 InHour, int32 InMinute, int32 InDay = 1) : Hour(InHour), Minute(InMinute), Day(InDay) {}

	/** Get time as a normalized 0-1 value (0 = midnight, 0.5 = noon) */
	float GetNormalizedTime() const
	{
		return (Hour * 60.0f + Minute) / 1440.0f; // 1440 = 24 * 60
	}

	/** Get time as total minutes since midnight */
	int32 GetTotalMinutes() const
	{
		return Hour * 60 + Minute;
	}

	/** Format as string HH:MM */
	FString ToString() const
	{
		return FString::Printf(TEXT("%02d:%02d"), Hour, Minute);
	}

	/** Add minutes to the time */
	void AddMinutes(int32 Minutes)
	{
		int32 TotalMinutes = GetTotalMinutes() + Minutes;
		while (TotalMinutes >= 1440)
		{
			TotalMinutes -= 1440;
			Day++;
		}
		while (TotalMinutes < 0)
		{
			TotalMinutes += 1440;
			Day--;
		}
		Hour = TotalMinutes / 60;
		Minute = TotalMinutes % 60;
	}

	bool operator==(const FCOTMGameTime& Other) const
	{
		return Hour == Other.Hour && Minute == Other.Minute && Day == Other.Day;
	}

	bool operator!=(const FCOTMGameTime& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * Visual settings for a specific time period
 * Used to configure post-process and lighting per time of day
 */
USTRUCT(BlueprintType)
struct FTimePeriodVisuals
{
	GENERATED_BODY()

	/** Sun/directional light color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FLinearColor SunColor = FLinearColor(1.0f, 0.95f, 0.85f, 1.0f);

	/** Sun intensity multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float SunIntensity = 1.0f;

	/** Sky light intensity multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float SkyLightIntensity = 1.0f;

	/** Sky light color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FLinearColor SkyLightColor = FLinearColor(0.5f, 0.6f, 0.8f, 1.0f);

	/** Fog density multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FogDensity = 0.02f;

	/** Fog color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FLinearColor FogColor = FLinearColor(0.5f, 0.55f, 0.6f, 1.0f);

	// Post-process overrides
	/** Saturation adjustment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Saturation = 0.7f;

	/** Temperature shift (-1 cold, 0 neutral, 1 warm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Temperature = 0.0f;

	/** Exposure compensation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess", meta = (ClampMin = "-3.0", ClampMax = "3.0"))
	float ExposureCompensation = 0.0f;

	/** Vignette intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VignetteIntensity = 0.4f;
};

/**
 * Weather visual configuration
 */
USTRUCT(BlueprintType)
struct FWeatherVisuals
{
	GENERATED_BODY()

	/** Fog density multiplier for this weather */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float FogDensityMultiplier = 1.0f;

	/** Sun intensity multiplier (0 = blocked by clouds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SunIntensityMultiplier = 1.0f;

	/** Saturation adjustment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.5"))
	float SaturationMultiplier = 1.0f;

	/** Contrast adjustment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess", meta = (ClampMin = "0.5", ClampMax = "1.5"))
	float ContrastMultiplier = 1.0f;

	/** Fog/sky color tint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FLinearColor AtmosphereTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	/** Particle system for this weather (rain, snow, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
	TSoftObjectPtr<UParticleSystem> WeatherParticles;

	/** Particle spawn rate multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float ParticleIntensity = 1.0f;
};

/**
 * Gameplay modifiers for a time period
 */
USTRUCT(BlueprintType)
struct FTimePeriodGameplay
{
	GENERATED_BODY()

	/** Enemy detection range multiplier (lower at night = easier stealth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemies", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float EnemyDetectionRange = 1.0f;

	/** Enemy damage dealt multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemies", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float EnemyDamageMultiplier = 1.0f;

	/** Player stamina regen rate multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float StaminaRegenMultiplier = 1.0f;

	/** Player damage dealt multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float PlayerDamageMultiplier = 1.0f;

	/** Special enemy spawn types enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TArray<FName> EnabledSpawnTypes;

	/** Can night-only events occur? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	bool bNightEventsEnabled = false;
};

/**
 * Gameplay modifiers for weather
 */
USTRUCT(BlueprintType)
struct FWeatherGameplay
{
	GENERATED_BODY()

	/** Movement speed multiplier (slower in heavy rain/snow) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.5", ClampMax = "1.5"))
	float MovementSpeedMultiplier = 1.0f;

	/** Hearing range multiplier (rain masks sounds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float HearingRangeMultiplier = 1.0f;

	/** Vision range multiplier (fog reduces sight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float VisionRangeMultiplier = 1.0f;

	/** Fire damage multiplier (reduced in rain) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FireDamageMultiplier = 1.0f;

	/** Lightning damage multiplier (increased in storms) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float LightningDamageMultiplier = 1.0f;

	/** Stamina drain multiplier (more in extreme weather) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float StaminaDrainMultiplier = 1.0f;
};

/**
 * Ambient audio configuration for time/weather
 */
USTRUCT(BlueprintType)
struct FAmbientAudioSet
{
	GENERATED_BODY()

	/** Looping ambient sound (wind, rain, crickets, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TSoftObjectPtr<USoundBase> AmbientLoop;

	/** Volume for the ambient loop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Volume = 1.0f;

	/** One-shot sounds that can play randomly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TArray<TSoftObjectPtr<USoundBase>> RandomSounds;

	/** Minimum time between random sounds (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "1.0"))
	float MinRandomInterval = 10.0f;

	/** Maximum time between random sounds (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "1.0"))
	float MaxRandomInterval = 60.0f;

	/** Volume range for random sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float RandomSoundVolume = 0.8f;
};

// ==================== Delegates ====================

/** Called when time period changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimePeriodChanged, ETimePeriod, NewPeriod, ETimePeriod, OldPeriod);

/** Called when weather changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChanged, EWeatherType, NewWeather, EWeatherType, OldWeather);

/** Called on day/night events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayNightEvent, EDayNightEvent, Event);

/** Called every in-game hour */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHourChanged, int32, NewHour);

/** Called every in-game day */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayChanged, int32, NewDay);
