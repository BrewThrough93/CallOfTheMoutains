// CallOfTheMoutains - Ambient SFX Component
// Manages time-based and weather-based ambient audio with crossfading

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DayNightTypes.h"
#include "AmbientSFXComponent.generated.h"

class UAudioComponent;

/**
 * Ambient SFX Component
 *
 * Manages ambient audio based on time of day and weather.
 * Supports crossfading between ambient states and random one-shot sounds.
 *
 * Attach to the DayNightManager actor.
 *
 * Features:
 * - Time-based ambient loops (day birds, night crickets, etc.)
 * - Weather audio integration
 * - Random one-shot sounds (bird calls, distant thunder, etc.)
 * - Smooth crossfading between states
 */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UAmbientSFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAmbientSFXComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ==================== Configuration ====================

	/** Master volume for all ambient audio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Settings", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MasterVolume = 1.0f;

	/** Crossfade duration when switching ambient loops (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Settings", meta = (ClampMin = "0.5"))
	float CrossfadeDuration = 3.0f;

	/** Should random sounds play? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Random")
	bool bPlayRandomSounds = true;

	// ==================== Time-Based Audio ====================

	/** Ambient audio sets for each time period */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Time")
	TMap<ETimePeriod, FAmbientAudioSet> TimePeriodAudio;

	// ==================== Weather Audio Overrides ====================

	/** Weather-specific ambient sounds (override time-based when active) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Weather")
	TMap<EWeatherType, FAmbientAudioSet> WeatherAudio;

	/** Volume multiplier for weather audio over time audio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeatherAudioPriority = 0.7f;

	// ==================== Quick Sound References ====================
	// (For common sounds - alternative to using the maps)

	/** Day ambient loop (birds, wind, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Quick Setup")
	USoundBase* DayAmbientSound;

	/** Night ambient loop (crickets, owls, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Quick Setup")
	USoundBase* NightAmbientSound;

	/** Dawn/Dusk transition ambient */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Quick Setup")
	USoundBase* TransitionAmbientSound;

	/** Rain ambient loop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Quick Setup")
	USoundBase* RainAmbientSound;

	/** Wind ambient loop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Quick Setup")
	USoundBase* WindAmbientSound;

	// ==================== Random One-Shot Sounds ====================

	/** Random day sounds (bird calls, distant animals, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Random Sounds")
	TArray<USoundBase*> RandomDaySounds;

	/** Random night sounds (owl hoots, wolf howls, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Random Sounds")
	TArray<USoundBase*> RandomNightSounds;

	/** Random storm sounds (thunder, etc.) - supplements WeatherSystem thunder */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Random Sounds")
	TArray<USoundBase*> RandomStormSounds;

	/** Min/max time between random sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Random Sounds")
	FVector2D RandomSoundInterval = FVector2D(15.0f, 60.0f);

	// ==================== State ====================

	/** Get current playing ambient type */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ambient")
	ETimePeriod GetCurrentAmbientPeriod() const { return CurrentTimePeriod; }

	/** Is ambient audio currently playing? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ambient")
	bool IsPlaying() const;

	// ==================== Control Functions ====================

	/** Update ambient audio for a new time period */
	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void SetTimePeriod(ETimePeriod NewPeriod);

	/** Update ambient audio for weather changes */
	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void SetWeather(EWeatherType Weather);

	/** Force stop all ambient audio */
	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void StopAllAmbient();

	/** Set master volume */
	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void SetMasterVolume(float NewVolume);

	/** Play a one-shot ambient sound */
	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void PlayOneShotSound(USoundBase* Sound, float VolumeMultiplier = 1.0f);

	/** Trigger a random sound from the current period's pool */
	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void TriggerRandomSound();

protected:
	// ==================== Internal State ====================

	/** Current time period */
	ETimePeriod CurrentTimePeriod = ETimePeriod::Morning;

	/** Current weather type */
	EWeatherType CurrentWeather = EWeatherType::Clear;

	/** Primary ambient audio component */
	UPROPERTY()
	UAudioComponent* PrimaryAmbientAudio;

	/** Secondary audio component (for crossfading) */
	UPROPERTY()
	UAudioComponent* SecondaryAmbientAudio;

	/** Weather ambient audio component */
	UPROPERTY()
	UAudioComponent* WeatherAmbientAudio;

	/** Timer for random sounds */
	float RandomSoundTimer = 0.0f;

	/** Is currently crossfading? */
	bool bIsCrossfading = false;

	/** Crossfade progress */
	float CrossfadeProgress = 0.0f;

	/** Target volumes for crossfade */
	float PrimaryTargetVolume = 1.0f;
	float SecondaryTargetVolume = 0.0f;

	// ==================== Internal Functions ====================

	/** Initialize default audio mappings */
	void InitializeDefaults();

	/** Get the appropriate sound for a time period */
	USoundBase* GetSoundForTimePeriod(ETimePeriod Period) const;

	/** Get the appropriate sound for weather */
	USoundBase* GetSoundForWeather(EWeatherType Weather) const;

	/** Start playing ambient sound on a component */
	void StartAmbientSound(UAudioComponent* AudioComp, USoundBase* Sound, float Volume);

	/** Update crossfade progress */
	void UpdateCrossfade(float DeltaTime);

	/** Update random sound timer */
	void UpdateRandomSounds(float DeltaTime);

	/** Swap primary and secondary audio components */
	void SwapAudioComponents();

	/** Create audio component if needed */
	UAudioComponent* EnsureAudioComponent(UAudioComponent*& AudioComp, const FString& Name);

	/** Get random sounds array for current time */
	const TArray<USoundBase*>& GetRandomSoundsForCurrentTime() const;
};
