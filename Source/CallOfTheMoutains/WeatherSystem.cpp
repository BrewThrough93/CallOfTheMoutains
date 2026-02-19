// CallOfTheMoutains - Weather System Implementation

#include "WeatherSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UWeatherSystem::UWeatherSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWeatherSystem::BeginPlay()
{
	Super::BeginPlay();

	// Initialize default configurations
	InitializeDefaults();

	// Set starting weather
	CurrentWeather = StartingWeather;
	TargetWeather = StartingWeather;
	TransitionState = EWeatherTransitionState::Stable;

	// Calculate initial duration
	if (WeatherTransitions.Contains(CurrentWeather))
	{
		const FWeatherTransition& Transition = WeatherTransitions[CurrentWeather];
		CurrentWeatherDuration = FMath::FRandRange(Transition.MinDuration, Transition.MaxDuration);
		TimeUntilWeatherChange = CurrentWeatherDuration;
	}
	else
	{
		TimeUntilWeatherChange = 300.0f; // Default 5 minutes
	}

	// Apply initial weather
	ApplyWeatherEffects();
}

void UWeatherSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update transition
	if (TransitionState != EWeatherTransitionState::Stable)
	{
		UpdateTransition(DeltaTime);
	}

	// Check for weather changes
	if (bWeatherChangeEnabled && TransitionState == EWeatherTransitionState::Stable)
	{
		CheckWeatherChange(DeltaTime);
	}

	// Update storm effects
	if (CurrentWeather == EWeatherType::Storm)
	{
		LightningTimer -= DeltaTime;
		if (LightningTimer <= 0.0f)
		{
			TriggerLightning();
			// Random interval between lightning strikes (5-30 seconds)
			LightningTimer = FMath::FRandRange(5.0f, 30.0f);
		}
	}

	// Update particle position to follow player
	if (ActiveParticles)
	{
		ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (Player)
		{
			FVector ParticleLocation = Player->GetActorLocation();
			ParticleLocation.Z += 500.0f; // Above player
			ActiveParticles->SetWorldLocation(ParticleLocation);
		}
	}
}

void UWeatherSystem::InitializeDefaults()
{
	if (WeatherTransitions.Num() > 0)
	{
		return;
	}

	// Clear weather transitions
	FWeatherTransition ClearTransition;
	ClearTransition.PossibleNextWeathers = { EWeatherType::Cloudy, EWeatherType::Fog };
	ClearTransition.TransitionWeights = { 0.7f, 0.3f };
	ClearTransition.MinDuration = 180.0f;
	ClearTransition.MaxDuration = 600.0f;
	ClearTransition.TransitionDuration = 45.0f;
	WeatherTransitions.Add(EWeatherType::Clear, ClearTransition);

	// Cloudy transitions
	FWeatherTransition CloudyTransition;
	CloudyTransition.PossibleNextWeathers = { EWeatherType::Clear, EWeatherType::LightRain, EWeatherType::Fog };
	CloudyTransition.TransitionWeights = { 0.4f, 0.4f, 0.2f };
	CloudyTransition.MinDuration = 120.0f;
	CloudyTransition.MaxDuration = 480.0f;
	CloudyTransition.TransitionDuration = 30.0f;
	WeatherTransitions.Add(EWeatherType::Cloudy, CloudyTransition);

	// Light rain transitions
	FWeatherTransition LightRainTransition;
	LightRainTransition.PossibleNextWeathers = { EWeatherType::Cloudy, EWeatherType::HeavyRain, EWeatherType::Clear };
	LightRainTransition.TransitionWeights = { 0.35f, 0.35f, 0.3f };
	LightRainTransition.MinDuration = 90.0f;
	LightRainTransition.MaxDuration = 360.0f;
	LightRainTransition.TransitionDuration = 25.0f;
	WeatherTransitions.Add(EWeatherType::LightRain, LightRainTransition);

	// Heavy rain transitions
	FWeatherTransition HeavyRainTransition;
	HeavyRainTransition.PossibleNextWeathers = { EWeatherType::LightRain, EWeatherType::Storm };
	HeavyRainTransition.TransitionWeights = { 0.6f, 0.4f };
	HeavyRainTransition.MinDuration = 60.0f;
	HeavyRainTransition.MaxDuration = 240.0f;
	HeavyRainTransition.TransitionDuration = 20.0f;
	WeatherTransitions.Add(EWeatherType::HeavyRain, HeavyRainTransition);

	// Storm transitions
	FWeatherTransition StormTransition;
	StormTransition.PossibleNextWeathers = { EWeatherType::HeavyRain, EWeatherType::LightRain };
	StormTransition.TransitionWeights = { 0.7f, 0.3f };
	StormTransition.MinDuration = 45.0f;
	StormTransition.MaxDuration = 180.0f;
	StormTransition.TransitionDuration = 30.0f;
	WeatherTransitions.Add(EWeatherType::Storm, StormTransition);

	// Fog transitions
	FWeatherTransition FogTransition;
	FogTransition.PossibleNextWeathers = { EWeatherType::Clear, EWeatherType::Cloudy, EWeatherType::LightRain };
	FogTransition.TransitionWeights = { 0.5f, 0.3f, 0.2f };
	FogTransition.MinDuration = 120.0f;
	FogTransition.MaxDuration = 420.0f;
	FogTransition.TransitionDuration = 60.0f;
	WeatherTransitions.Add(EWeatherType::Fog, FogTransition);

	// Snow transitions
	FWeatherTransition SnowTransition;
	SnowTransition.PossibleNextWeathers = { EWeatherType::Cloudy, EWeatherType::Clear };
	SnowTransition.TransitionWeights = { 0.6f, 0.4f };
	SnowTransition.MinDuration = 180.0f;
	SnowTransition.MaxDuration = 600.0f;
	SnowTransition.TransitionDuration = 45.0f;
	WeatherTransitions.Add(EWeatherType::Snow, SnowTransition);

	// Initialize weather visuals
	if (WeatherVisuals.Num() == 0)
	{
		// Clear
		FWeatherVisuals ClearVisuals;
		ClearVisuals.FogDensityMultiplier = 1.0f;
		ClearVisuals.SunIntensityMultiplier = 1.0f;
		ClearVisuals.SaturationMultiplier = 1.0f;
		ClearVisuals.ContrastMultiplier = 1.0f;
		ClearVisuals.AtmosphereTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
		ClearVisuals.ParticleIntensity = 0.0f;
		WeatherVisuals.Add(EWeatherType::Clear, ClearVisuals);

		// Cloudy
		FWeatherVisuals CloudyVisuals;
		CloudyVisuals.FogDensityMultiplier = 1.3f;
		CloudyVisuals.SunIntensityMultiplier = 0.6f;
		CloudyVisuals.SaturationMultiplier = 0.85f;
		CloudyVisuals.ContrastMultiplier = 0.9f;
		CloudyVisuals.AtmosphereTint = FLinearColor(0.85f, 0.87f, 0.9f, 1.0f);
		CloudyVisuals.ParticleIntensity = 0.0f;
		WeatherVisuals.Add(EWeatherType::Cloudy, CloudyVisuals);

		// Light Rain
		FWeatherVisuals LightRainVisuals;
		LightRainVisuals.FogDensityMultiplier = 1.5f;
		LightRainVisuals.SunIntensityMultiplier = 0.4f;
		LightRainVisuals.SaturationMultiplier = 0.75f;
		LightRainVisuals.ContrastMultiplier = 0.85f;
		LightRainVisuals.AtmosphereTint = FLinearColor(0.7f, 0.75f, 0.8f, 1.0f);
		LightRainVisuals.ParticleIntensity = 0.5f;
		WeatherVisuals.Add(EWeatherType::LightRain, LightRainVisuals);

		// Heavy Rain
		FWeatherVisuals HeavyRainVisuals;
		HeavyRainVisuals.FogDensityMultiplier = 2.0f;
		HeavyRainVisuals.SunIntensityMultiplier = 0.2f;
		HeavyRainVisuals.SaturationMultiplier = 0.6f;
		HeavyRainVisuals.ContrastMultiplier = 0.8f;
		HeavyRainVisuals.AtmosphereTint = FLinearColor(0.5f, 0.55f, 0.65f, 1.0f);
		HeavyRainVisuals.ParticleIntensity = 1.0f;
		WeatherVisuals.Add(EWeatherType::HeavyRain, HeavyRainVisuals);

		// Storm
		FWeatherVisuals StormVisuals;
		StormVisuals.FogDensityMultiplier = 2.5f;
		StormVisuals.SunIntensityMultiplier = 0.1f;
		StormVisuals.SaturationMultiplier = 0.5f;
		StormVisuals.ContrastMultiplier = 1.2f;
		StormVisuals.AtmosphereTint = FLinearColor(0.35f, 0.38f, 0.5f, 1.0f);
		StormVisuals.ParticleIntensity = 1.5f;
		WeatherVisuals.Add(EWeatherType::Storm, StormVisuals);

		// Fog
		FWeatherVisuals FogVisuals;
		FogVisuals.FogDensityMultiplier = 4.0f;
		FogVisuals.SunIntensityMultiplier = 0.3f;
		FogVisuals.SaturationMultiplier = 0.7f;
		FogVisuals.ContrastMultiplier = 0.7f;
		FogVisuals.AtmosphereTint = FLinearColor(0.8f, 0.82f, 0.85f, 1.0f);
		FogVisuals.ParticleIntensity = 0.3f;
		WeatherVisuals.Add(EWeatherType::Fog, FogVisuals);

		// Snow
		FWeatherVisuals SnowVisuals;
		SnowVisuals.FogDensityMultiplier = 1.8f;
		SnowVisuals.SunIntensityMultiplier = 0.5f;
		SnowVisuals.SaturationMultiplier = 0.65f;
		SnowVisuals.ContrastMultiplier = 0.85f;
		SnowVisuals.AtmosphereTint = FLinearColor(0.9f, 0.92f, 0.95f, 1.0f);
		SnowVisuals.ParticleIntensity = 0.8f;
		WeatherVisuals.Add(EWeatherType::Snow, SnowVisuals);
	}

	// Initialize gameplay modifiers
	if (WeatherGameplay.Num() == 0)
	{
		// Clear - baseline
		FWeatherGameplay ClearGameplay;
		WeatherGameplay.Add(EWeatherType::Clear, ClearGameplay);

		// Cloudy - slightly reduced visibility
		FWeatherGameplay CloudyGameplay;
		CloudyGameplay.VisionRangeMultiplier = 0.9f;
		WeatherGameplay.Add(EWeatherType::Cloudy, CloudyGameplay);

		// Light Rain - reduced hearing, some fire reduction
		FWeatherGameplay LightRainGameplay;
		LightRainGameplay.HearingRangeMultiplier = 0.8f;
		LightRainGameplay.FireDamageMultiplier = 0.85f;
		LightRainGameplay.VisionRangeMultiplier = 0.85f;
		WeatherGameplay.Add(EWeatherType::LightRain, LightRainGameplay);

		// Heavy Rain - significant audio masking
		FWeatherGameplay HeavyRainGameplay;
		HeavyRainGameplay.HearingRangeMultiplier = 0.5f;
		HeavyRainGameplay.FireDamageMultiplier = 0.5f;
		HeavyRainGameplay.VisionRangeMultiplier = 0.7f;
		HeavyRainGameplay.MovementSpeedMultiplier = 0.95f;
		WeatherGameplay.Add(EWeatherType::HeavyRain, HeavyRainGameplay);

		// Storm - dangerous conditions
		FWeatherGameplay StormGameplay;
		StormGameplay.HearingRangeMultiplier = 0.3f;
		StormGameplay.FireDamageMultiplier = 0.25f;
		StormGameplay.LightningDamageMultiplier = 2.0f;
		StormGameplay.VisionRangeMultiplier = 0.5f;
		StormGameplay.MovementSpeedMultiplier = 0.9f;
		StormGameplay.StaminaDrainMultiplier = 1.2f;
		WeatherGameplay.Add(EWeatherType::Storm, StormGameplay);

		// Fog - severely reduced vision
		FWeatherGameplay FogGameplay;
		FogGameplay.VisionRangeMultiplier = 0.3f;
		FogGameplay.HearingRangeMultiplier = 1.1f; // Sound travels better in fog
		WeatherGameplay.Add(EWeatherType::Fog, FogGameplay);

		// Snow - cold conditions
		FWeatherGameplay SnowGameplay;
		SnowGameplay.MovementSpeedMultiplier = 0.85f;
		SnowGameplay.StaminaDrainMultiplier = 1.15f;
		SnowGameplay.FireDamageMultiplier = 0.75f;
		SnowGameplay.VisionRangeMultiplier = 0.75f;
		WeatherGameplay.Add(EWeatherType::Snow, SnowGameplay);
	}
}

void UWeatherSystem::UpdateTransition(float DeltaTime)
{
	TransitionProgress += DeltaTime / CurrentTransitionDuration;

	if (TransitionProgress >= 1.0f)
	{
		TransitionProgress = 1.0f;
		TransitionState = EWeatherTransitionState::Stable;

		EWeatherType OldWeather = CurrentWeather;
		CurrentWeather = TargetWeather;

		// Set up next duration
		if (WeatherTransitions.Contains(CurrentWeather))
		{
			const FWeatherTransition& Transition = WeatherTransitions[CurrentWeather];
			CurrentWeatherDuration = FMath::FRandRange(Transition.MinDuration, Transition.MaxDuration);
			TimeUntilWeatherChange = CurrentWeatherDuration * WeatherChangeProbability;
		}

		OnWeatherChanged.Broadcast(CurrentWeather, OldWeather);

		// Reset lightning timer for storms
		if (CurrentWeather == EWeatherType::Storm)
		{
			LightningTimer = FMath::FRandRange(3.0f, 10.0f);
		}
	}

	ApplyWeatherEffects();
}

void UWeatherSystem::CheckWeatherChange(float DeltaTime)
{
	TimeUntilWeatherChange -= DeltaTime;

	if (TimeUntilWeatherChange <= 0.0f)
	{
		TriggerRandomWeatherChange();
	}
}

EWeatherType UWeatherSystem::SelectNextWeather() const
{
	if (!WeatherTransitions.Contains(CurrentWeather))
	{
		return EWeatherType::Clear;
	}

	const FWeatherTransition& Transition = WeatherTransitions[CurrentWeather];

	if (Transition.PossibleNextWeathers.Num() == 0)
	{
		return EWeatherType::Clear;
	}

	// Calculate total weight
	float TotalWeight = 0.0f;
	for (int32 i = 0; i < Transition.TransitionWeights.Num() && i < Transition.PossibleNextWeathers.Num(); i++)
	{
		TotalWeight += Transition.TransitionWeights[i];
	}

	// Random selection
	float Random = FMath::FRand() * TotalWeight;
	float Accumulated = 0.0f;

	for (int32 i = 0; i < Transition.PossibleNextWeathers.Num(); i++)
	{
		float Weight = (i < Transition.TransitionWeights.Num()) ? Transition.TransitionWeights[i] : 1.0f;
		Accumulated += Weight;

		if (Random <= Accumulated)
		{
			return Transition.PossibleNextWeathers[i];
		}
	}

	return Transition.PossibleNextWeathers[0];
}

void UWeatherSystem::ApplyWeatherEffects()
{
	UpdateParticles();
	UpdateAudio();
}

void UWeatherSystem::UpdateParticles()
{
	UParticleSystem* TargetParticles = GetParticlesForWeather(TargetWeather);

	// Get player for attachment
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Player)
	{
		return;
	}

	// Create or update particle component
	if (TargetParticles)
	{
		if (!ActiveParticles || ActiveParticles->Template != TargetParticles)
		{
			// Need new particle component
			if (ActiveParticles)
			{
				ActiveParticles->DestroyComponent();
			}

			ActiveParticles = UGameplayStatics::SpawnEmitterAtLocation(
				this,
				TargetParticles,
				Player->GetActorLocation() + FVector(0, 0, 500),
				FRotator::ZeroRotator,
				FVector(1.0f),
				true,
				EPSCPoolMethod::None,
				false
			);
		}

		// Adjust intensity based on transition
		if (ActiveParticles && WeatherVisuals.Contains(TargetWeather))
		{
			float Intensity = WeatherVisuals[TargetWeather].ParticleIntensity * TransitionProgress;
			// Note: You'd need to set spawn rate or other parameters here
			// This depends on your specific particle system setup
		}
	}
	else if (ActiveParticles)
	{
		// Fade out and destroy
		ActiveParticles->DestroyComponent();
		ActiveParticles = nullptr;
	}
}

void UWeatherSystem::UpdateAudio()
{
	USoundBase* TargetSound = GetSoundForWeather(TargetWeather);

	if (TargetSound)
	{
		if (!ActiveWeatherAudio || ActiveWeatherAudio->Sound != TargetSound)
		{
			// Stop old audio
			if (ActiveWeatherAudio)
			{
				ActiveWeatherAudio->FadeOut(2.0f, 0.0f);
			}

			// Start new audio
			ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
			if (Player)
			{
				ActiveWeatherAudio = UGameplayStatics::SpawnSoundAttached(
					TargetSound,
					Player->GetRootComponent(),
					NAME_None,
					FVector::ZeroVector,
					EAttachLocation::KeepRelativeOffset,
					true,
					TransitionProgress,
					1.0f,
					0.0f,
					nullptr,
					nullptr,
					false
				);
			}
		}
		else if (ActiveWeatherAudio)
		{
			// Adjust volume based on transition
			ActiveWeatherAudio->SetVolumeMultiplier(TransitionProgress);
		}
	}
	else if (ActiveWeatherAudio)
	{
		ActiveWeatherAudio->FadeOut(2.0f, 0.0f);
		ActiveWeatherAudio = nullptr;
	}
}

UParticleSystem* UWeatherSystem::GetParticlesForWeather(EWeatherType Weather) const
{
	switch (Weather)
	{
	case EWeatherType::LightRain:
		return RainParticles;
	case EWeatherType::HeavyRain:
		return HeavyRainParticles ? HeavyRainParticles : RainParticles;
	case EWeatherType::Storm:
		return StormParticles ? StormParticles : HeavyRainParticles;
	case EWeatherType::Snow:
		return SnowParticles;
	case EWeatherType::Fog:
		return FogParticles;
	default:
		return nullptr;
	}
}

USoundBase* UWeatherSystem::GetSoundForWeather(EWeatherType Weather) const
{
	switch (Weather)
	{
	case EWeatherType::LightRain:
		return RainSound;
	case EWeatherType::HeavyRain:
		return HeavyRainSound ? HeavyRainSound : RainSound;
	case EWeatherType::Storm:
		return StormWindSound ? StormWindSound : HeavyRainSound;
	case EWeatherType::Snow:
		return SnowWindSound;
	default:
		return nullptr;
	}
}

FWeatherVisuals UWeatherSystem::LerpWeatherVisuals(const FWeatherVisuals& A, const FWeatherVisuals& B, float Alpha) const
{
	FWeatherVisuals Result;

	Result.FogDensityMultiplier = FMath::Lerp(A.FogDensityMultiplier, B.FogDensityMultiplier, Alpha);
	Result.SunIntensityMultiplier = FMath::Lerp(A.SunIntensityMultiplier, B.SunIntensityMultiplier, Alpha);
	Result.SaturationMultiplier = FMath::Lerp(A.SaturationMultiplier, B.SaturationMultiplier, Alpha);
	Result.ContrastMultiplier = FMath::Lerp(A.ContrastMultiplier, B.ContrastMultiplier, Alpha);
	Result.AtmosphereTint = FMath::Lerp(A.AtmosphereTint, B.AtmosphereTint, Alpha);
	Result.ParticleIntensity = FMath::Lerp(A.ParticleIntensity, B.ParticleIntensity, Alpha);

	return Result;
}

bool UWeatherSystem::IsRaining() const
{
	return CurrentWeather == EWeatherType::LightRain ||
		   CurrentWeather == EWeatherType::HeavyRain ||
		   CurrentWeather == EWeatherType::Storm;
}

FWeatherGameplay UWeatherSystem::GetCurrentWeatherGameplay() const
{
	if (TransitionState == EWeatherTransitionState::Stable)
	{
		if (WeatherGameplay.Contains(CurrentWeather))
		{
			return WeatherGameplay[CurrentWeather];
		}
	}
	else
	{
		// Blend between previous and target
		FWeatherGameplay PrevGameplay, TargetGameplayData;

		if (WeatherGameplay.Contains(PreviousWeather))
		{
			PrevGameplay = WeatherGameplay[PreviousWeather];
		}
		if (WeatherGameplay.Contains(TargetWeather))
		{
			TargetGameplayData = WeatherGameplay[TargetWeather];
		}

		// Lerp gameplay values
		FWeatherGameplay Result;
		Result.MovementSpeedMultiplier = FMath::Lerp(PrevGameplay.MovementSpeedMultiplier, TargetGameplayData.MovementSpeedMultiplier, TransitionProgress);
		Result.HearingRangeMultiplier = FMath::Lerp(PrevGameplay.HearingRangeMultiplier, TargetGameplayData.HearingRangeMultiplier, TransitionProgress);
		Result.VisionRangeMultiplier = FMath::Lerp(PrevGameplay.VisionRangeMultiplier, TargetGameplayData.VisionRangeMultiplier, TransitionProgress);
		Result.FireDamageMultiplier = FMath::Lerp(PrevGameplay.FireDamageMultiplier, TargetGameplayData.FireDamageMultiplier, TransitionProgress);
		Result.LightningDamageMultiplier = FMath::Lerp(PrevGameplay.LightningDamageMultiplier, TargetGameplayData.LightningDamageMultiplier, TransitionProgress);
		Result.StaminaDrainMultiplier = FMath::Lerp(PrevGameplay.StaminaDrainMultiplier, TargetGameplayData.StaminaDrainMultiplier, TransitionProgress);

		return Result;
	}

	return FWeatherGameplay();
}

FWeatherVisuals UWeatherSystem::GetCurrentWeatherVisuals() const
{
	if (TransitionState == EWeatherTransitionState::Stable)
	{
		if (WeatherVisuals.Contains(CurrentWeather))
		{
			return WeatherVisuals[CurrentWeather];
		}
	}
	else
	{
		FWeatherVisuals PrevVisuals, TargetVisualsData;

		if (WeatherVisuals.Contains(PreviousWeather))
		{
			PrevVisuals = WeatherVisuals[PreviousWeather];
		}
		if (WeatherVisuals.Contains(TargetWeather))
		{
			TargetVisualsData = WeatherVisuals[TargetWeather];
		}

		return LerpWeatherVisuals(PrevVisuals, TargetVisualsData, TransitionProgress);
	}

	return FWeatherVisuals();
}

void UWeatherSystem::SetWeather(EWeatherType NewWeather, bool bInstant)
{
	if (bInstant)
	{
		PreviousWeather = CurrentWeather;
		CurrentWeather = NewWeather;
		TargetWeather = NewWeather;
		TransitionState = EWeatherTransitionState::Stable;
		TransitionProgress = 1.0f;

		ApplyWeatherEffects();
		OnWeatherChanged.Broadcast(CurrentWeather, PreviousWeather);
	}
	else
	{
		TransitionToWeather(NewWeather);
	}
}

void UWeatherSystem::TransitionToWeather(EWeatherType NewWeather, float Duration)
{
	if (NewWeather == CurrentWeather && TransitionState == EWeatherTransitionState::Stable)
	{
		return;
	}

	PreviousWeather = CurrentWeather;
	TargetWeather = NewWeather;
	TransitionState = EWeatherTransitionState::TransitioningIn;
	TransitionProgress = 0.0f;

	if (Duration > 0.0f)
	{
		CurrentTransitionDuration = Duration;
	}
	else if (WeatherTransitions.Contains(CurrentWeather))
	{
		CurrentTransitionDuration = WeatherTransitions[CurrentWeather].TransitionDuration;
	}
	else
	{
		CurrentTransitionDuration = 30.0f;
	}
}

void UWeatherSystem::TriggerRandomWeatherChange()
{
	EWeatherType NextWeather = SelectNextWeather();
	TransitionToWeather(NextWeather);
}

void UWeatherSystem::TriggerLightning()
{
	DoLightningFlash();
	PlayThunder();
}

void UWeatherSystem::DoLightningFlash()
{
	// This would typically trigger a bright directional light flash
	// You can also use post-process to briefly boost exposure
	// For now, we'll leave this as a hook for Blueprint extension
}

void UWeatherSystem::PlayThunder()
{
	if (ThunderSounds.Num() == 0)
	{
		return;
	}

	int32 SoundIndex = FMath::RandRange(0, ThunderSounds.Num() - 1);
	USoundBase* ThunderSound = ThunderSounds[SoundIndex];

	if (ThunderSound)
	{
		// Play with slight delay to simulate distance
		float Delay = FMath::FRandRange(0.5f, 3.0f);

		FTimerHandle ThunderTimer;
		GetWorld()->GetTimerManager().SetTimer(
			ThunderTimer,
			[this, ThunderSound]()
			{
				UGameplayStatics::PlaySound2D(this, ThunderSound, FMath::FRandRange(0.7f, 1.0f));
			},
			Delay,
			false
		);
	}
}
