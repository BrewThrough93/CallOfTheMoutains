// CallOfTheMoutains - Day/Night Cycle Manager Implementation

#include "DayNightManager.h"
#include "WeatherSystem.h"
#include "AmbientSFXComponent.h"
#include "DystopianPostProcess.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

ADayNightManager::ADayNightManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Default starting time: 8:00 AM
	StartingTime = FCOTMGameTime(8, 0, 1);

	// Create weather system component
	WeatherSystem = CreateDefaultSubobject<UWeatherSystem>(TEXT("WeatherSystem"));

	// Create ambient SFX component
	AmbientSFX = CreateDefaultSubobject<UAmbientSFXComponent>(TEXT("AmbientSFX"));
}

void ADayNightManager::BeginPlay()
{
	Super::BeginPlay();

	// Initialize default settings if not configured
	InitializeDefaultSettings();

	// Set starting time
	CurrentTime = StartingTime;
	LastHour = CurrentTime.Hour;
	CurrentTimePeriod = CalculateTimePeriod();
	PreviousTimePeriod = CurrentTimePeriod;

	// Cache light references
	CacheLightReferences();

	// Find player post-process
	FindPlayerPostProcess();

	// Initial update
	UpdateSunRotation();
	UpdateLighting(0.0f);
	UpdatePostProcess();

	// Fire initial events
	OnTimePeriodChanged.Broadcast(CurrentTimePeriod, CurrentTimePeriod);
}

void ADayNightManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCycleEnabled)
	{
		UpdateTime(DeltaTime);
	}

	UpdateSunRotation();
	UpdateLighting(DeltaTime);
	UpdatePostProcess();
}

void ADayNightManager::UpdateTime(float DeltaTime)
{
	// Calculate minutes per real second
	// 1440 minutes in a day / DayCycleDuration seconds = minutes per second
	float MinutesPerSecond = (1440.0f / DayCycleDuration) * TimeScale;
	float MinutesToAdd = MinutesPerSecond * DeltaTime;

	// Store previous state
	int32 PrevDay = CurrentTime.Day;

	// Accumulate fractional minutes and only add whole minutes to avoid precision issues
	// FractionalMinutes is now a member variable (not static) to support multiple instances
	FractionalMinutes += MinutesToAdd;
	if (FractionalMinutes >= 1.0f)
	{
		int32 WholeMinutes = FMath::FloorToInt(FractionalMinutes);
		CurrentTime.AddMinutes(WholeMinutes);
		FractionalMinutes -= WholeMinutes;
	}

	// Check for hour change
	if (CurrentTime.Hour != LastHour)
	{
		OnHourChanged.Broadcast(CurrentTime.Hour);
		LastHour = CurrentTime.Hour;
	}

	// Check for day change
	if (CurrentTime.Day != PrevDay)
	{
		OnDayChanged.Broadcast(CurrentTime.Day);
	}

	// Check time period change
	ETimePeriod NewPeriod = CalculateTimePeriod();
	if (NewPeriod != CurrentTimePeriod)
	{
		PreviousTimePeriod = CurrentTimePeriod;
		CurrentTimePeriod = NewPeriod;

		// Start blending
		bIsBlendingTimePeriod = true;
		TimePeriodBlendAlpha = 0.0f;

		if (TimePeriodVisuals.Contains(PreviousTimePeriod))
		{
			BlendStartVisuals = TimePeriodVisuals[PreviousTimePeriod];
		}
		if (TimePeriodVisuals.Contains(CurrentTimePeriod))
		{
			BlendTargetVisuals = TimePeriodVisuals[CurrentTimePeriod];
		}

		OnTimePeriodChanged.Broadcast(CurrentTimePeriod, PreviousTimePeriod);
	}

	// Check for special events
	CheckTimeEvents();
}

void ADayNightManager::UpdateSunRotation()
{
	if (!SunLight)
	{
		return;
	}

	// Calculate sun angle based on time
	// Normalized time: 0.0 = midnight, 0.5 = noon, 1.0 = midnight again
	float NormalizedTime = CurrentTime.GetNormalizedTime();

	// Convert to rotation: 0 at midnight, 180 at noon
	// Sun rises in east, sets in west
	float SunAngle = (NormalizedTime * 360.0f) - 90.0f + SunAngleOffset;

	// Apply rotation
	FRotator NewRotation = SunRotationAxis;
	NewRotation.Pitch = SunAngle;

	if (SunLightActor)
	{
		SunLightActor->SetActorRotation(NewRotation);
	}
}

void ADayNightManager::UpdateLighting(float DeltaTime)
{
	// Update blend state
	if (bIsBlendingTimePeriod && TimePeriodBlendTime > 0.0f)
	{
		TimePeriodBlendAlpha += DeltaTime / TimePeriodBlendTime;
		if (TimePeriodBlendAlpha >= 1.0f)
		{
			TimePeriodBlendAlpha = 1.0f;
			bIsBlendingTimePeriod = false;
		}

		// Lerp visuals
		FTimePeriodVisuals CurrentVisuals = LerpVisuals(BlendStartVisuals, BlendTargetVisuals, TimePeriodBlendAlpha);
		ApplyVisuals(CurrentVisuals);
	}
	else if (TimePeriodVisuals.Contains(CurrentTimePeriod))
	{
		ApplyVisuals(TimePeriodVisuals[CurrentTimePeriod]);
	}
}

void ADayNightManager::UpdatePostProcess()
{
	if (!bControlPostProcess || !PlayerPostProcess)
	{
		// Try to find it again
		if (bControlPostProcess && !PlayerPostProcess)
		{
			FindPlayerPostProcess();
		}
		return;
	}

	// The DystopianPostProcess handles its own blending
	// We just need to update settings based on time period
	// This is handled via the ApplyVisuals function affecting post-process indirectly
}

ETimePeriod ADayNightManager::CalculateTimePeriod() const
{
	int32 Hour = CurrentTime.Hour;

	if (Hour >= 5 && Hour < 7)
	{
		return ETimePeriod::Dawn;
	}
	else if (Hour >= 7 && Hour < 11)
	{
		return ETimePeriod::Morning;
	}
	else if (Hour >= 11 && Hour < 14)
	{
		return ETimePeriod::Midday;
	}
	else if (Hour >= 14 && Hour < 17)
	{
		return ETimePeriod::Afternoon;
	}
	else if (Hour >= 17 && Hour < 19)
	{
		return ETimePeriod::Dusk;
	}
	else if (Hour >= 19 && Hour < 21)
	{
		return ETimePeriod::Evening;
	}
	else if (Hour >= 21 || Hour < 3)
	{
		return ETimePeriod::Night;
	}
	else // 3:00 - 5:00
	{
		return ETimePeriod::LateNight;
	}
}

void ADayNightManager::CheckTimeEvents()
{
	int32 Hour = CurrentTime.Hour;
	int32 Minute = CurrentTime.Minute;

	// Combine hour and minute into a unique key to prevent duplicate event firing
	int32 CurrentEventKey = Hour * 60 + Minute;

	// Skip if we already fired an event for this minute
	if (CurrentEventKey == LastEventMinute)
	{
		return;
	}

	// Check for specific events (only fire once per minute)
	bool bEventFired = false;

	if (Hour == 5 && Minute == 0)
	{
		OnDayNightEvent.Broadcast(EDayNightEvent::DawnBreak);
		bEventFired = true;
	}
	else if (Hour == 6 && Minute == 0)
	{
		OnDayNightEvent.Broadcast(EDayNightEvent::SunRise);
		bEventFired = true;
	}
	else if (Hour == 12 && Minute == 0)
	{
		OnDayNightEvent.Broadcast(EDayNightEvent::NoonPeak);
		bEventFired = true;
	}
	else if (Hour == 18 && Minute == 0)
	{
		OnDayNightEvent.Broadcast(EDayNightEvent::SunSet);
		bEventFired = true;
	}
	else if (Hour == 20 && Minute == 0)
	{
		OnDayNightEvent.Broadcast(EDayNightEvent::NightFall);
		bEventFired = true;
	}
	else if (Hour == 0 && Minute == 0)
	{
		OnDayNightEvent.Broadcast(EDayNightEvent::Midnight);
		bEventFired = true;
	}

	// Update last event minute to prevent re-firing
	if (bEventFired)
	{
		LastEventMinute = CurrentEventKey;
	}
}

void ADayNightManager::InitializeDefaultSettings()
{
	// Only initialize if not already configured
	if (TimePeriodVisuals.Num() > 0)
	{
		return;
	}

	// Dawn - warm golden light, low sun
	FTimePeriodVisuals Dawn;
	Dawn.SunColor = FLinearColor(1.0f, 0.7f, 0.4f, 1.0f);
	Dawn.SunIntensity = 5.0f;
	Dawn.SkyLightIntensity = 1.2f;
	Dawn.SkyLightColor = FLinearColor(0.8f, 0.6f, 0.5f, 1.0f);
	Dawn.FogDensity = 0.02f;
	Dawn.FogColor = FLinearColor(0.8f, 0.6f, 0.5f, 1.0f);
	Dawn.Saturation = 0.85f;
	Dawn.Temperature = 0.3f;
	Dawn.ExposureCompensation = 0.2f;
	Dawn.VignetteIntensity = 0.3f;
	TimePeriodVisuals.Add(ETimePeriod::Dawn, Dawn);

	// Morning - bright, clear
	FTimePeriodVisuals Morning;
	Morning.SunColor = FLinearColor(1.0f, 0.95f, 0.85f, 1.0f);
	Morning.SunIntensity = 10.0f;
	Morning.SkyLightIntensity = 2.0f;
	Morning.SkyLightColor = FLinearColor(0.6f, 0.7f, 0.9f, 1.0f);
	Morning.FogDensity = 0.01f;
	Morning.FogColor = FLinearColor(0.7f, 0.75f, 0.85f, 1.0f);
	Morning.Saturation = 0.9f;
	Morning.Temperature = 0.1f;
	Morning.ExposureCompensation = 0.5f;
	Morning.VignetteIntensity = 0.25f;
	TimePeriodVisuals.Add(ETimePeriod::Morning, Morning);

	// Midday - bright and vibrant
	FTimePeriodVisuals Midday;
	Midday.SunColor = FLinearColor(1.0f, 1.0f, 0.95f, 1.0f);
	Midday.SunIntensity = 15.0f;
	Midday.SkyLightIntensity = 2.5f;
	Midday.SkyLightColor = FLinearColor(0.7f, 0.75f, 0.85f, 1.0f);
	Midday.FogDensity = 0.008f;
	Midday.FogColor = FLinearColor(0.6f, 0.65f, 0.7f, 1.0f);
	Midday.Saturation = 0.85f;
	Midday.Temperature = 0.0f;
	Midday.ExposureCompensation = 0.8f;
	Midday.VignetteIntensity = 0.3f;
	TimePeriodVisuals.Add(ETimePeriod::Midday, Midday);

	// Afternoon - warm, slightly less intense
	FTimePeriodVisuals Afternoon;
	Afternoon.SunColor = FLinearColor(1.0f, 0.9f, 0.75f, 1.0f);
	Afternoon.SunIntensity = 12.0f;
	Afternoon.SkyLightIntensity = 1.8f;
	Afternoon.SkyLightColor = FLinearColor(0.65f, 0.7f, 0.8f, 1.0f);
	Afternoon.FogDensity = 0.012f;
	Afternoon.FogColor = FLinearColor(0.7f, 0.68f, 0.65f, 1.0f);
	Afternoon.Saturation = 0.85f;
	Afternoon.Temperature = 0.15f;
	Afternoon.ExposureCompensation = 0.6f;
	Afternoon.VignetteIntensity = 0.3f;
	TimePeriodVisuals.Add(ETimePeriod::Afternoon, Afternoon);

	// Dusk - orange/red, dramatic
	FTimePeriodVisuals Dusk;
	Dusk.SunColor = FLinearColor(1.0f, 0.5f, 0.2f, 1.0f);
	Dusk.SunIntensity = 3.0f;
	Dusk.SkyLightIntensity = 0.5f;
	Dusk.SkyLightColor = FLinearColor(0.7f, 0.5f, 0.4f, 1.0f);
	Dusk.FogDensity = 0.035f;
	Dusk.FogColor = FLinearColor(0.7f, 0.5f, 0.4f, 1.0f);
	Dusk.Saturation = 0.8f;
	Dusk.Temperature = 0.4f;
	Dusk.ExposureCompensation = -0.2f;
	Dusk.VignetteIntensity = 0.45f;
	TimePeriodVisuals.Add(ETimePeriod::Dusk, Dusk);

	// Evening - blue hour, transitioning to dark
	FTimePeriodVisuals Evening;
	Evening.SunColor = FLinearColor(0.4f, 0.4f, 0.6f, 1.0f);
	Evening.SunIntensity = 1.0f;
	Evening.SkyLightIntensity = 0.3f;
	Evening.SkyLightColor = FLinearColor(0.3f, 0.35f, 0.5f, 1.0f);
	Evening.FogDensity = 0.03f;
	Evening.FogColor = FLinearColor(0.25f, 0.3f, 0.4f, 1.0f);
	Evening.Saturation = 0.6f;
	Evening.Temperature = -0.2f;
	Evening.ExposureCompensation = -0.5f;
	Evening.VignetteIntensity = 0.5f;
	TimePeriodVisuals.Add(ETimePeriod::Evening, Evening);

	// Night - dark, cold, mysterious
	FTimePeriodVisuals Night;
	Night.SunColor = FLinearColor(0.2f, 0.25f, 0.4f, 1.0f); // Moonlight
	Night.SunIntensity = 0.3f;
	Night.SkyLightIntensity = 0.15f;
	Night.SkyLightColor = FLinearColor(0.15f, 0.18f, 0.3f, 1.0f);
	Night.FogDensity = 0.04f;
	Night.FogColor = FLinearColor(0.1f, 0.12f, 0.18f, 1.0f);
	Night.Saturation = 0.5f;
	Night.Temperature = -0.35f;
	Night.ExposureCompensation = -1.0f;
	Night.VignetteIntensity = 0.6f;
	TimePeriodVisuals.Add(ETimePeriod::Night, Night);

	// Late Night - darkest, eerie
	FTimePeriodVisuals LateNight;
	LateNight.SunColor = FLinearColor(0.15f, 0.18f, 0.3f, 1.0f);
	LateNight.SunIntensity = 0.15f;
	LateNight.SkyLightIntensity = 0.1f;
	LateNight.SkyLightColor = FLinearColor(0.1f, 0.12f, 0.2f, 1.0f);
	LateNight.FogDensity = 0.05f;
	LateNight.FogColor = FLinearColor(0.08f, 0.1f, 0.15f, 1.0f);
	LateNight.Saturation = 0.4f;
	LateNight.Temperature = -0.4f;
	LateNight.ExposureCompensation = -1.5f;
	LateNight.VignetteIntensity = 0.65f;
	TimePeriodVisuals.Add(ETimePeriod::LateNight, LateNight);

	// Initialize default gameplay settings
	if (TimePeriodGameplay.Num() == 0)
	{
		// Day periods - normal gameplay
		FTimePeriodGameplay DayGameplay;
		DayGameplay.EnemyDetectionRange = 1.0f;
		DayGameplay.EnemyDamageMultiplier = 1.0f;
		DayGameplay.StaminaRegenMultiplier = 1.0f;
		DayGameplay.PlayerDamageMultiplier = 1.0f;
		DayGameplay.bNightEventsEnabled = false;

		TimePeriodGameplay.Add(ETimePeriod::Dawn, DayGameplay);
		TimePeriodGameplay.Add(ETimePeriod::Morning, DayGameplay);
		TimePeriodGameplay.Add(ETimePeriod::Midday, DayGameplay);
		TimePeriodGameplay.Add(ETimePeriod::Afternoon, DayGameplay);

		// Dusk - transition
		FTimePeriodGameplay DuskGameplay;
		DuskGameplay.EnemyDetectionRange = 0.85f;
		DuskGameplay.EnemyDamageMultiplier = 1.1f;
		DuskGameplay.StaminaRegenMultiplier = 0.95f;
		DuskGameplay.PlayerDamageMultiplier = 1.0f;
		DuskGameplay.bNightEventsEnabled = false;
		TimePeriodGameplay.Add(ETimePeriod::Dusk, DuskGameplay);

		// Evening - harder
		FTimePeriodGameplay EveningGameplay;
		EveningGameplay.EnemyDetectionRange = 0.7f;
		EveningGameplay.EnemyDamageMultiplier = 1.2f;
		EveningGameplay.StaminaRegenMultiplier = 0.9f;
		EveningGameplay.PlayerDamageMultiplier = 1.1f;
		EveningGameplay.bNightEventsEnabled = true;
		TimePeriodGameplay.Add(ETimePeriod::Evening, EveningGameplay);

		// Night - most dangerous
		FTimePeriodGameplay NightGameplay;
		NightGameplay.EnemyDetectionRange = 0.5f;
		NightGameplay.EnemyDamageMultiplier = 1.35f;
		NightGameplay.StaminaRegenMultiplier = 0.8f;
		NightGameplay.PlayerDamageMultiplier = 1.15f;
		NightGameplay.bNightEventsEnabled = true;
		NightGameplay.EnabledSpawnTypes.Add(FName("NightCreature"));
		NightGameplay.EnabledSpawnTypes.Add(FName("Shadow"));
		TimePeriodGameplay.Add(ETimePeriod::Night, NightGameplay);

		// Late Night - deadliest
		FTimePeriodGameplay LateNightGameplay;
		LateNightGameplay.EnemyDetectionRange = 0.4f;
		LateNightGameplay.EnemyDamageMultiplier = 1.5f;
		LateNightGameplay.StaminaRegenMultiplier = 0.7f;
		LateNightGameplay.PlayerDamageMultiplier = 1.2f;
		LateNightGameplay.bNightEventsEnabled = true;
		LateNightGameplay.EnabledSpawnTypes.Add(FName("NightCreature"));
		LateNightGameplay.EnabledSpawnTypes.Add(FName("Shadow"));
		LateNightGameplay.EnabledSpawnTypes.Add(FName("Nightmare"));
		TimePeriodGameplay.Add(ETimePeriod::LateNight, LateNightGameplay);
	}
}

void ADayNightManager::CacheLightReferences()
{
	// Get directional light component
	if (SunLightActor)
	{
		SunLight = SunLightActor->FindComponentByClass<UDirectionalLightComponent>();
	}

	// Get sky light component
	if (SkyLightActor)
	{
		SkyLight = SkyLightActor->FindComponentByClass<USkyLightComponent>();
	}

	// Get fog component
	if (FogActor)
	{
		HeightFog = FogActor->FindComponentByClass<UExponentialHeightFogComponent>();
	}

	// Auto-find if not specified
	if (!SunLight)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			UDirectionalLightComponent* DirLight = It->FindComponentByClass<UDirectionalLightComponent>();
			if (DirLight)
			{
				SunLight = DirLight;
				SunLightActor = *It;
				break;
			}
		}
	}

	if (!SkyLight)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			USkyLightComponent* Sky = It->FindComponentByClass<USkyLightComponent>();
			if (Sky)
			{
				SkyLight = Sky;
				SkyLightActor = *It;
				break;
			}
		}
	}

	if (!HeightFog)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			UExponentialHeightFogComponent* Fog = It->FindComponentByClass<UExponentialHeightFogComponent>();
			if (Fog)
			{
				HeightFog = Fog;
				FogActor = *It;
				break;
			}
		}
	}
}

void ADayNightManager::FindPlayerPostProcess()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PlayerPostProcess = PC->FindComponentByClass<UDystopianPostProcess>();

		// Also check the pawn
		if (!PlayerPostProcess && PC->GetPawn())
		{
			PlayerPostProcess = PC->GetPawn()->FindComponentByClass<UDystopianPostProcess>();
		}
	}
}

FTimePeriodVisuals ADayNightManager::LerpVisuals(const FTimePeriodVisuals& A, const FTimePeriodVisuals& B, float Alpha) const
{
	FTimePeriodVisuals Result;

	Result.SunColor = FMath::Lerp(A.SunColor, B.SunColor, Alpha);
	Result.SunIntensity = FMath::Lerp(A.SunIntensity, B.SunIntensity, Alpha);
	Result.SkyLightIntensity = FMath::Lerp(A.SkyLightIntensity, B.SkyLightIntensity, Alpha);
	Result.SkyLightColor = FMath::Lerp(A.SkyLightColor, B.SkyLightColor, Alpha);
	Result.FogDensity = FMath::Lerp(A.FogDensity, B.FogDensity, Alpha);
	Result.FogColor = FMath::Lerp(A.FogColor, B.FogColor, Alpha);
	Result.Saturation = FMath::Lerp(A.Saturation, B.Saturation, Alpha);
	Result.Temperature = FMath::Lerp(A.Temperature, B.Temperature, Alpha);
	Result.ExposureCompensation = FMath::Lerp(A.ExposureCompensation, B.ExposureCompensation, Alpha);
	Result.VignetteIntensity = FMath::Lerp(A.VignetteIntensity, B.VignetteIntensity, Alpha);

	return Result;
}

void ADayNightManager::ApplyVisuals(const FTimePeriodVisuals& Visuals)
{
	// Apply to directional light (sun)
	if (SunLight)
	{
		SunLight->SetLightColor(Visuals.SunColor);
		SunLight->SetIntensity(Visuals.SunIntensity);
	}

	// Apply to sky light
	if (SkyLight)
	{
		SkyLight->SetLightColor(Visuals.SkyLightColor);
		SkyLight->SetIntensity(Visuals.SkyLightIntensity);
		SkyLight->MarkRenderStateDirty();
	}

	// Apply to fog
	if (HeightFog)
	{
		HeightFog->SetFogDensity(Visuals.FogDensity);
		HeightFog->SetFogInscatteringColor(Visuals.FogColor);
	}

	// Apply post-process settings through DystopianPostProcess
	if (PlayerPostProcess)
	{
		// Update the post-process settings
		FDystopianSettings NewSettings = PlayerPostProcess->Settings;
		NewSettings.Saturation = Visuals.Saturation;
		NewSettings.Temperature = Visuals.Temperature;
		NewSettings.ExposureCompensation = Visuals.ExposureCompensation;
		NewSettings.VignetteIntensity = Visuals.VignetteIntensity;

		PlayerPostProcess->BlendToSettings(NewSettings, TimePeriodBlendTime * 0.5f);
	}
}

bool ADayNightManager::IsDaytime() const
{
	int32 Hour = CurrentTime.Hour;
	return Hour >= 6 && Hour < 19;
}

bool ADayNightManager::IsNighttime() const
{
	return !IsDaytime();
}

FTimePeriodGameplay ADayNightManager::GetCurrentGameplayModifiers() const
{
	if (TimePeriodGameplay.Contains(CurrentTimePeriod))
	{
		return TimePeriodGameplay[CurrentTimePeriod];
	}

	// Return defaults
	return FTimePeriodGameplay();
}

void ADayNightManager::SetTime(FCOTMGameTime NewTime, bool bTriggerEvents)
{
	FCOTMGameTime OldTime = CurrentTime;
	CurrentTime = NewTime;

	// Update period
	ETimePeriod OldPeriod = CurrentTimePeriod;
	CurrentTimePeriod = CalculateTimePeriod();

	if (bTriggerEvents && CurrentTimePeriod != OldPeriod)
	{
		OnTimePeriodChanged.Broadcast(CurrentTimePeriod, OldPeriod);
	}

	// Immediate visual update
	bIsBlendingTimePeriod = false;
	if (TimePeriodVisuals.Contains(CurrentTimePeriod))
	{
		ApplyVisuals(TimePeriodVisuals[CurrentTimePeriod]);
	}

	UpdateSunRotation();
}

void ADayNightManager::SetTimeByHourMinute(int32 Hour, int32 Minute, bool bTriggerEvents)
{
	SetTime(FCOTMGameTime(Hour, Minute, CurrentTime.Day), bTriggerEvents);
}

void ADayNightManager::SkipToTimePeriod(ETimePeriod TargetPeriod, bool bTriggerEvents)
{
	int32 TargetHour = GetTimePeriodStartHour(TargetPeriod);
	SetTimeByHourMinute(TargetHour, 0, bTriggerEvents);
}

int32 ADayNightManager::GetTimePeriodStartHour(ETimePeriod Period) const
{
	switch (Period)
	{
	case ETimePeriod::Dawn:       return 5;
	case ETimePeriod::Morning:    return 7;
	case ETimePeriod::Midday:     return 11;
	case ETimePeriod::Afternoon:  return 14;
	case ETimePeriod::Dusk:       return 17;
	case ETimePeriod::Evening:    return 19;
	case ETimePeriod::Night:      return 21;
	case ETimePeriod::LateNight:  return 3;
	default:                      return 12;
	}
}

void ADayNightManager::GetSaveData(FCOTMGameTime& OutTime, EWeatherType& OutWeather) const
{
	OutTime = CurrentTime;
	if (WeatherSystem)
	{
		OutWeather = WeatherSystem->GetCurrentWeather();
	}
	else
	{
		OutWeather = EWeatherType::Clear;
	}
}

void ADayNightManager::LoadSaveData(const FCOTMGameTime& InTime, EWeatherType InWeather)
{
	SetTime(InTime, false);

	if (WeatherSystem)
	{
		WeatherSystem->SetWeather(InWeather, true);
	}
}

ADayNightManager* ADayNightManager::GetDayNightManager(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ADayNightManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}
