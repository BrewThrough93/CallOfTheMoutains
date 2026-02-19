// CallOfTheMoutains - Weather System Component
// Manages weather states, transitions, and visual effects

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DayNightTypes.h"
#include "WeatherSystem.generated.h"

class UParticleSystemComponent;
class UAudioComponent;

/**
 * Weather transition configuration
 */
USTRUCT(BlueprintType)
struct FWeatherTransition
{
	GENERATED_BODY()

	/** Weather types that can transition from this weather */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	TArray<EWeatherType> PossibleNextWeathers;

	/** Weight/probability for each next weather (should match array above) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	TArray<float> TransitionWeights;

	/** Minimum time this weather lasts (real seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (ClampMin = "30.0"))
	float MinDuration = 120.0f;

	/** Maximum time this weather lasts (real seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (ClampMin = "60.0"))
	float MaxDuration = 600.0f;

	/** Time to transition to next weather (real seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (ClampMin = "5.0"))
	float TransitionDuration = 30.0f;
};

/**
 * Weather System Component
 *
 * Manages weather states, transitions, and visual/audio effects.
 * Attach to the DayNightManager actor.
 *
 * Features:
 * - Randomized weather with weighted transitions
 * - Particle effects for rain, snow, etc.
 * - Integration with lighting and post-process
 * - Weather-based gameplay modifiers
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UWeatherSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeatherSystem();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ==================== Configuration ====================

	/** Starting weather */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Initial")
	EWeatherType StartingWeather = EWeatherType::Clear;

	/** Is weather changing enabled? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings")
	bool bWeatherChangeEnabled = true;

	/** Global weather change probability multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float WeatherChangeProbability = 1.0f;

	/** Transition rules for each weather type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Transitions")
	TMap<EWeatherType, FWeatherTransition> WeatherTransitions;

	/** Visual settings for each weather type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Visuals")
	TMap<EWeatherType, FWeatherVisuals> WeatherVisuals;

	/** Gameplay modifiers for each weather type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Gameplay")
	TMap<EWeatherType, FWeatherGameplay> WeatherGameplay;

	// ==================== Particle References ====================

	/** Rain particle system (spawned at player location) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Particles")
	UParticleSystem* RainParticles;

	/** Heavy rain particle system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Particles")
	UParticleSystem* HeavyRainParticles;

	/** Storm particle system (rain + lightning flashes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Particles")
	UParticleSystem* StormParticles;

	/** Snow particle system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Particles")
	UParticleSystem* SnowParticles;

	/** Fog particle system (ground fog) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Particles")
	UParticleSystem* FogParticles;

	// ==================== Audio References ====================

	/** Rain ambient sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
	USoundBase* RainSound;

	/** Heavy rain ambient sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
	USoundBase* HeavyRainSound;

	/** Thunder sound cues (randomly selected) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
	TArray<USoundBase*> ThunderSounds;

	/** Wind sound for storms */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
	USoundBase* StormWindSound;

	/** Snow/blizzard wind sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
	USoundBase* SnowWindSound;

	// ==================== Events ====================

	/** Called when weather changes */
	UPROPERTY(BlueprintAssignable, Category = "Weather|Events")
	FOnWeatherChanged OnWeatherChanged;

	// ==================== State Getters ====================

	/** Get current weather type */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather")
	EWeatherType GetCurrentWeather() const { return CurrentWeather; }

	/** Get weather we're transitioning to (same as current if not transitioning) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather")
	EWeatherType GetTargetWeather() const { return TargetWeather; }

	/** Get current transition state */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather")
	EWeatherTransitionState GetTransitionState() const { return TransitionState; }

	/** Get transition progress (0-1) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather")
	float GetTransitionProgress() const { return TransitionProgress; }

	/** Is it currently raining? (light, heavy, or storm) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather")
	bool IsRaining() const;

	/** Get current gameplay modifiers for weather */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather")
	FWeatherGameplay GetCurrentWeatherGameplay() const;

	/** Get current visual settings for weather */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather")
	FWeatherVisuals GetCurrentWeatherVisuals() const;

	// ==================== Weather Control ====================

	/** Force a specific weather (optionally instant) */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeather(EWeatherType NewWeather, bool bInstant = false);

	/** Start transitioning to a new weather */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void TransitionToWeather(EWeatherType NewWeather, float Duration = -1.0f);

	/** Trigger a random weather change */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void TriggerRandomWeatherChange();

	/** Enable/disable weather changes */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeatherChangeEnabled(bool bEnabled) { bWeatherChangeEnabled = bEnabled; }

	// ==================== Storm Effects ====================

	/** Trigger a lightning flash and thunder */
	UFUNCTION(BlueprintCallable, Category = "Weather|Storm")
	void TriggerLightning();

protected:
	// ==================== Internal State ====================

	/** Current active weather */
	UPROPERTY(BlueprintReadOnly, Category = "Weather|State")
	EWeatherType CurrentWeather = EWeatherType::Clear;

	/** Weather we're transitioning to */
	EWeatherType TargetWeather = EWeatherType::Clear;

	/** Previous weather (for blending) */
	EWeatherType PreviousWeather = EWeatherType::Clear;

	/** Current transition state */
	EWeatherTransitionState TransitionState = EWeatherTransitionState::Stable;

	/** Transition progress (0-1) */
	float TransitionProgress = 0.0f;

	/** Duration of current transition */
	float CurrentTransitionDuration = 30.0f;

	/** Time until next weather change check */
	float TimeUntilWeatherChange = 0.0f;

	/** Current weather duration */
	float CurrentWeatherDuration = 0.0f;

	/** Timer for lightning in storms */
	float LightningTimer = 0.0f;

	// ==================== Active Components ====================

	/** Active weather particle component */
	UPROPERTY()
	UParticleSystemComponent* ActiveParticles;

	/** Active weather audio component */
	UPROPERTY()
	UAudioComponent* ActiveWeatherAudio;

	/** Active wind audio component */
	UPROPERTY()
	UAudioComponent* ActiveWindAudio;

	// ==================== Internal Functions ====================

	/** Initialize default weather configurations */
	void InitializeDefaults();

	/** Update weather transition */
	void UpdateTransition(float DeltaTime);

	/** Check if we should change weather */
	void CheckWeatherChange(float DeltaTime);

	/** Select next weather based on transition rules */
	EWeatherType SelectNextWeather() const;

	/** Apply current weather effects */
	void ApplyWeatherEffects();

	/** Update particle effects */
	void UpdateParticles();

	/** Update audio for current weather */
	void UpdateAudio();

	/** Lerp between weather visuals */
	FWeatherVisuals LerpWeatherVisuals(const FWeatherVisuals& A, const FWeatherVisuals& B, float Alpha) const;

	/** Get the appropriate particle system for a weather type */
	UParticleSystem* GetParticlesForWeather(EWeatherType Weather) const;

	/** Get the appropriate sound for a weather type */
	USoundBase* GetSoundForWeather(EWeatherType Weather) const;

	/** Play thunder sound */
	void PlayThunder();

	/** Handle lightning flash effect */
	void DoLightningFlash();
};
