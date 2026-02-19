// CallOfTheMoutains - Day/Night Cycle Manager
// Controls time progression, lighting, post-process, and weather

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightTypes.h"
#include "DayNightManager.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UDystopianPostProcess;
class UWeatherSystem;
class UAmbientSFXComponent;

/**
 * Day/Night Cycle Manager
 *
 * Place one in your level to control the day/night cycle.
 * Manages:
 * - Time progression with configurable day length
 * - Sun/moon position and lighting
 * - Post-process transitions per time period
 * - Weather system integration
 * - Ambient audio per time/weather
 * - Gameplay modifier events
 *
 * Usage:
 * 1. Place ADayNightManager in your level
 * 2. Assign your DirectionalLight (sun), SkyLight, and Fog references
 * 3. Configure time period visuals and gameplay settings
 * 4. The system handles everything else automatically
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API ADayNightManager : public AActor
{
	GENERATED_BODY()

public:
	ADayNightManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ==================== Core Configuration ====================

	/** Real-time seconds for a full 24-hour cycle (default: 1800 = 30 minutes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Time", meta = (ClampMin = "60.0", ClampMax = "7200.0"))
	float DayCycleDuration = 1800.0f;

	/** Starting time of day */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Time")
	FCOTMGameTime StartingTime;

	/** Is the cycle currently running? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Time")
	bool bCycleEnabled = true;

	/** Time scale multiplier (1.0 = normal, 2.0 = double speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Time", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float TimeScale = 1.0f;

	// ==================== Lighting References ====================

	/** Reference to the directional light (sun/moon) in the level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Lighting")
	AActor* SunLightActor;

	/** Reference to the sky light in the level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Lighting")
	AActor* SkyLightActor;

	/** Reference to the exponential height fog in the level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Lighting")
	AActor* FogActor;

	/** Sun rotation axis (world up by default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Lighting")
	FRotator SunRotationAxis = FRotator(0.0f, 0.0f, 0.0f);

	/** Angle offset for sunrise/sunset times */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Lighting", meta = (ClampMin = "-90.0", ClampMax = "90.0"))
	float SunAngleOffset = 0.0f;

	// ==================== Time Period Configurations ====================

	/** Visual settings for each time period */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Visuals")
	TMap<ETimePeriod, FTimePeriodVisuals> TimePeriodVisuals;

	/** Gameplay settings for each time period */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Gameplay")
	TMap<ETimePeriod, FTimePeriodGameplay> TimePeriodGameplay;

	/** Blend time when transitioning between time periods (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|Visuals", meta = (ClampMin = "0.0"))
	float TimePeriodBlendTime = 30.0f;

	// ==================== Components ====================

	/** Weather system component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Day Night|Components")
	UWeatherSystem* WeatherSystem;

	/** Ambient SFX component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Day Night|Components")
	UAmbientSFXComponent* AmbientSFX;

	// ==================== Post Process Integration ====================

	/** Reference to the player's post process component (auto-found if null) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|PostProcess")
	UDystopianPostProcess* PlayerPostProcess;

	/** Should we modify the player's post-process? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night|PostProcess")
	bool bControlPostProcess = true;

	// ==================== Events ====================

	/** Called when time period changes */
	UPROPERTY(BlueprintAssignable, Category = "Day Night|Events")
	FOnTimePeriodChanged OnTimePeriodChanged;

	/** Called on special day/night events */
	UPROPERTY(BlueprintAssignable, Category = "Day Night|Events")
	FOnDayNightEvent OnDayNightEvent;

	/** Called every in-game hour */
	UPROPERTY(BlueprintAssignable, Category = "Day Night|Events")
	FOnHourChanged OnHourChanged;

	/** Called when a new day begins */
	UPROPERTY(BlueprintAssignable, Category = "Day Night|Events")
	FOnDayChanged OnDayChanged;

	// ==================== State Getters ====================

	/** Get current time */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	FCOTMGameTime GetCurrentTime() const { return CurrentTime; }

	/** Get current time as normalized 0-1 (0=midnight, 0.5=noon) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	float GetNormalizedTime() const { return CurrentTime.GetNormalizedTime(); }

	/** Get current time period */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	ETimePeriod GetCurrentTimePeriod() const { return CurrentTimePeriod; }

	/** Get current day number */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	int32 GetCurrentDay() const { return CurrentTime.Day; }

	/** Is it currently day time? (between dawn and dusk) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	bool IsDaytime() const;

	/** Is it currently night time? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	bool IsNighttime() const;

	/** Get formatted time string (HH:MM) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	FString GetTimeString() const { return CurrentTime.ToString(); }

	/** Get current gameplay modifiers for this time period */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night")
	FTimePeriodGameplay GetCurrentGameplayModifiers() const;

	// ==================== Time Control ====================

	/** Set the current time (triggers appropriate events) */
	UFUNCTION(BlueprintCallable, Category = "Day Night")
	void SetTime(FCOTMGameTime NewTime, bool bTriggerEvents = true);

	/** Set time by hour and minute */
	UFUNCTION(BlueprintCallable, Category = "Day Night")
	void SetTimeByHourMinute(int32 Hour, int32 Minute, bool bTriggerEvents = true);

	/** Skip to a specific time period */
	UFUNCTION(BlueprintCallable, Category = "Day Night")
	void SkipToTimePeriod(ETimePeriod TargetPeriod, bool bTriggerEvents = true);

	/** Pause the day/night cycle */
	UFUNCTION(BlueprintCallable, Category = "Day Night")
	void PauseCycle() { bCycleEnabled = false; }

	/** Resume the day/night cycle */
	UFUNCTION(BlueprintCallable, Category = "Day Night")
	void ResumeCycle() { bCycleEnabled = true; }

	/** Toggle cycle pause */
	UFUNCTION(BlueprintCallable, Category = "Day Night")
	void ToggleCycle() { bCycleEnabled = !bCycleEnabled; }

	// ==================== Save/Load ====================

	/** Get save data for current state */
	UFUNCTION(BlueprintCallable, Category = "Day Night|Save")
	void GetSaveData(FCOTMGameTime& OutTime, EWeatherType& OutWeather) const;

	/** Load state from save data */
	UFUNCTION(BlueprintCallable, Category = "Day Night|Save")
	void LoadSaveData(const FCOTMGameTime& InTime, EWeatherType InWeather);

	// ==================== Static Access ====================

	/** Get the day/night manager instance in the current world */
	UFUNCTION(BlueprintCallable, Category = "Day Night", meta = (WorldContext = "WorldContextObject"))
	static ADayNightManager* GetDayNightManager(const UObject* WorldContextObject);

protected:
	// ==================== Internal State ====================

	/** Current time of day */
	UPROPERTY(BlueprintReadOnly, Category = "Day Night|State")
	FCOTMGameTime CurrentTime;

	/** Current time period */
	UPROPERTY(BlueprintReadOnly, Category = "Day Night|State")
	ETimePeriod CurrentTimePeriod = ETimePeriod::Midday;

	/** Previous time period (for blending) */
	ETimePeriod PreviousTimePeriod = ETimePeriod::Midday;

	/** Last hour for hourly events */
	int32 LastHour = -1;

	/** Fractional minute accumulator for smooth time progression (instance member, not static) */
	float FractionalMinutes = 0.0f;

	/** Last minute when a time event was fired (prevents duplicate event firing) */
	int32 LastEventMinute = -1;

	/** Cached light components */
	UPROPERTY()
	UDirectionalLightComponent* SunLight;

	UPROPERTY()
	USkyLightComponent* SkyLight;

	UPROPERTY()
	UExponentialHeightFogComponent* HeightFog;

	/** Time period blend state */
	bool bIsBlendingTimePeriod = false;
	float TimePeriodBlendAlpha = 0.0f;
	FTimePeriodVisuals BlendStartVisuals;
	FTimePeriodVisuals BlendTargetVisuals;

	// ==================== Internal Functions ====================

	/** Update time progression */
	void UpdateTime(float DeltaTime);

	/** Update sun/moon rotation based on time */
	void UpdateSunRotation();

	/** Update lighting based on current time period */
	void UpdateLighting(float DeltaTime);

	/** Update post-process based on time period */
	void UpdatePostProcess();

	/** Determine time period from current time */
	ETimePeriod CalculateTimePeriod() const;

	/** Check for and fire time-based events */
	void CheckTimeEvents();

	/** Initialize default time period configurations */
	void InitializeDefaultSettings();

	/** Find and cache light component references */
	void CacheLightReferences();

	/** Find player's post-process component */
	void FindPlayerPostProcess();

	/** Lerp between two visual configurations */
	FTimePeriodVisuals LerpVisuals(const FTimePeriodVisuals& A, const FTimePeriodVisuals& B, float Alpha) const;

	/** Apply visual settings to lights and fog */
	void ApplyVisuals(const FTimePeriodVisuals& Visuals);

	/** Get the starting hour for a time period */
	int32 GetTimePeriodStartHour(ETimePeriod Period) const;
};
