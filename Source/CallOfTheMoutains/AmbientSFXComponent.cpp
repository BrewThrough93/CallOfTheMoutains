// CallOfTheMoutains - Ambient SFX Component Implementation

#include "AmbientSFXComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

UAmbientSFXComponent::UAmbientSFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAmbientSFXComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize defaults
	InitializeDefaults();

	// Create audio components
	PrimaryAmbientAudio = EnsureAudioComponent(PrimaryAmbientAudio, TEXT("PrimaryAmbient"));
	SecondaryAmbientAudio = EnsureAudioComponent(SecondaryAmbientAudio, TEXT("SecondaryAmbient"));
	WeatherAmbientAudio = EnsureAudioComponent(WeatherAmbientAudio, TEXT("WeatherAmbient"));

	// Set initial random sound timer
	RandomSoundTimer = FMath::FRandRange(RandomSoundInterval.X, RandomSoundInterval.Y);

	// Start with initial time period
	SetTimePeriod(CurrentTimePeriod);
}

void UAmbientSFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update crossfade
	if (bIsCrossfading)
	{
		UpdateCrossfade(DeltaTime);
	}

	// Update random sounds
	if (bPlayRandomSounds)
	{
		UpdateRandomSounds(DeltaTime);
	}
}

void UAmbientSFXComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllAmbient();
	Super::EndPlay(EndPlayReason);
}

void UAmbientSFXComponent::InitializeDefaults()
{
	// Quick setup: If the maps are empty but quick setup sounds are set,
	// create basic mappings
	if (TimePeriodAudio.Num() == 0)
	{
		// Day periods
		if (DayAmbientSound)
		{
			FAmbientAudioSet DayAudio;
			DayAudio.AmbientLoop = DayAmbientSound;
			DayAudio.Volume = 1.0f;

			TimePeriodAudio.Add(ETimePeriod::Morning, DayAudio);
			TimePeriodAudio.Add(ETimePeriod::Midday, DayAudio);
			TimePeriodAudio.Add(ETimePeriod::Afternoon, DayAudio);
		}

		// Transition periods (dawn/dusk)
		if (TransitionAmbientSound)
		{
			FAmbientAudioSet TransitionAudio;
			TransitionAudio.AmbientLoop = TransitionAmbientSound;
			TransitionAudio.Volume = 1.0f;

			TimePeriodAudio.Add(ETimePeriod::Dawn, TransitionAudio);
			TimePeriodAudio.Add(ETimePeriod::Dusk, TransitionAudio);
		}
		else if (DayAmbientSound)
		{
			// Fallback to day sounds for transitions
			FAmbientAudioSet TransitionAudio;
			TransitionAudio.AmbientLoop = DayAmbientSound;
			TransitionAudio.Volume = 0.7f;

			TimePeriodAudio.Add(ETimePeriod::Dawn, TransitionAudio);
			TimePeriodAudio.Add(ETimePeriod::Dusk, TransitionAudio);
		}

		// Night periods
		if (NightAmbientSound)
		{
			FAmbientAudioSet NightAudio;
			NightAudio.AmbientLoop = NightAmbientSound;
			NightAudio.Volume = 1.0f;

			TimePeriodAudio.Add(ETimePeriod::Evening, NightAudio);
			TimePeriodAudio.Add(ETimePeriod::Night, NightAudio);
			TimePeriodAudio.Add(ETimePeriod::LateNight, NightAudio);
		}
	}

	// Weather audio quick setup
	if (WeatherAudio.Num() == 0)
	{
		if (RainAmbientSound)
		{
			FAmbientAudioSet RainAudio;
			RainAudio.AmbientLoop = RainAmbientSound;
			RainAudio.Volume = 1.0f;

			WeatherAudio.Add(EWeatherType::LightRain, RainAudio);

			FAmbientAudioSet HeavyRainAudio;
			HeavyRainAudio.AmbientLoop = RainAmbientSound;
			HeavyRainAudio.Volume = 1.3f;
			WeatherAudio.Add(EWeatherType::HeavyRain, HeavyRainAudio);

			FAmbientAudioSet StormAudioSet;
			StormAudioSet.AmbientLoop = RainAmbientSound;
			StormAudioSet.Volume = 1.5f;
			WeatherAudio.Add(EWeatherType::Storm, StormAudioSet);
		}

		if (WindAmbientSound)
		{
			FAmbientAudioSet WindAudio;
			WindAudio.AmbientLoop = WindAmbientSound;
			WindAudio.Volume = 0.8f;

			WeatherAudio.Add(EWeatherType::Fog, WindAudio);
			WeatherAudio.Add(EWeatherType::Snow, WindAudio);
		}
	}
}

UAudioComponent* UAmbientSFXComponent::EnsureAudioComponent(UAudioComponent*& AudioComp, const FString& Name)
{
	if (!AudioComp)
	{
		AudioComp = NewObject<UAudioComponent>(GetOwner(), *Name);
		if (AudioComp)
		{
			AudioComp->bAutoActivate = false;
			AudioComp->bAutoDestroy = false;
			AudioComp->RegisterComponent();
			AudioComp->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	return AudioComp;
}

USoundBase* UAmbientSFXComponent::GetSoundForTimePeriod(ETimePeriod Period) const
{
	if (TimePeriodAudio.Contains(Period))
	{
		const FAmbientAudioSet& AudioSet = TimePeriodAudio[Period];
		if (AudioSet.AmbientLoop.IsValid())
		{
			return AudioSet.AmbientLoop.Get();
		}
		else if (!AudioSet.AmbientLoop.IsNull())
		{
			// Try to load it
			return AudioSet.AmbientLoop.LoadSynchronous();
		}
	}

	// Fallback to quick setup
	switch (Period)
	{
	case ETimePeriod::Dawn:
	case ETimePeriod::Dusk:
		return TransitionAmbientSound ? TransitionAmbientSound : DayAmbientSound;

	case ETimePeriod::Morning:
	case ETimePeriod::Midday:
	case ETimePeriod::Afternoon:
		return DayAmbientSound;

	case ETimePeriod::Evening:
	case ETimePeriod::Night:
	case ETimePeriod::LateNight:
		return NightAmbientSound;

	default:
		return DayAmbientSound;
	}
}

USoundBase* UAmbientSFXComponent::GetSoundForWeather(EWeatherType Weather) const
{
	if (WeatherAudio.Contains(Weather))
	{
		const FAmbientAudioSet& AudioSet = WeatherAudio[Weather];
		if (AudioSet.AmbientLoop.IsValid())
		{
			return AudioSet.AmbientLoop.Get();
		}
		else if (!AudioSet.AmbientLoop.IsNull())
		{
			return AudioSet.AmbientLoop.LoadSynchronous();
		}
	}

	// Fallback
	switch (Weather)
	{
	case EWeatherType::LightRain:
	case EWeatherType::HeavyRain:
	case EWeatherType::Storm:
		return RainAmbientSound;

	case EWeatherType::Fog:
	case EWeatherType::Snow:
		return WindAmbientSound;

	default:
		return nullptr;
	}
}

void UAmbientSFXComponent::StartAmbientSound(UAudioComponent* AudioComp, USoundBase* Sound, float Volume)
{
	if (!AudioComp || !Sound)
	{
		return;
	}

	AudioComp->SetSound(Sound);
	AudioComp->SetVolumeMultiplier(Volume * MasterVolume);
	AudioComp->Play();
}

void UAmbientSFXComponent::SetTimePeriod(ETimePeriod NewPeriod)
{
	if (NewPeriod == CurrentTimePeriod && PrimaryAmbientAudio && PrimaryAmbientAudio->IsPlaying())
	{
		return;
	}

	CurrentTimePeriod = NewPeriod;

	USoundBase* NewSound = GetSoundForTimePeriod(NewPeriod);
	if (!NewSound)
	{
		return;
	}

	// Calculate volume
	float Volume = 1.0f;
	if (TimePeriodAudio.Contains(NewPeriod))
	{
		Volume = TimePeriodAudio[NewPeriod].Volume;
	}

	// If weather is active, reduce time-based audio
	if (CurrentWeather != EWeatherType::Clear && CurrentWeather != EWeatherType::Cloudy)
	{
		Volume *= (1.0f - WeatherAudioPriority);
	}

	// Check if we need to crossfade
	if (PrimaryAmbientAudio && PrimaryAmbientAudio->IsPlaying())
	{
		// Start crossfade
		bIsCrossfading = true;
		CrossfadeProgress = 0.0f;

		// Swap components - secondary becomes new primary
		SwapAudioComponents();

		// Start new sound on new primary
		StartAmbientSound(PrimaryAmbientAudio, NewSound, 0.0f);
		PrimaryTargetVolume = Volume;
		SecondaryTargetVolume = 0.0f;
	}
	else
	{
		// No crossfade needed, just start
		StartAmbientSound(PrimaryAmbientAudio, NewSound, Volume);
	}
}

void UAmbientSFXComponent::SetWeather(EWeatherType Weather)
{
	CurrentWeather = Weather;

	USoundBase* WeatherSound = GetSoundForWeather(Weather);

	if (WeatherSound)
	{
		// Calculate volume
		float Volume = WeatherAudioPriority;
		if (WeatherAudio.Contains(Weather))
		{
			Volume *= WeatherAudio[Weather].Volume;
		}

		if (!WeatherAmbientAudio)
		{
			WeatherAmbientAudio = EnsureAudioComponent(WeatherAmbientAudio, TEXT("WeatherAmbient"));
		}

		if (WeatherAmbientAudio)
		{
			if (WeatherAmbientAudio->Sound != WeatherSound)
			{
				// Fade out old, start new
				if (WeatherAmbientAudio->IsPlaying())
				{
					WeatherAmbientAudio->FadeOut(CrossfadeDuration * 0.5f, 0.0f);
				}

				// Start new weather sound with fade in
				WeatherAmbientAudio->SetSound(WeatherSound);
				WeatherAmbientAudio->SetVolumeMultiplier(0.0f);
				WeatherAmbientAudio->Play();
				WeatherAmbientAudio->FadeIn(CrossfadeDuration, Volume * MasterVolume);
			}
			else
			{
				// Same sound, just adjust volume
				WeatherAmbientAudio->SetVolumeMultiplier(Volume * MasterVolume);
			}
		}

		// Reduce time-based audio
		if (PrimaryAmbientAudio && PrimaryAmbientAudio->IsPlaying())
		{
			float ReducedVolume = 1.0f - WeatherAudioPriority;
			if (TimePeriodAudio.Contains(CurrentTimePeriod))
			{
				ReducedVolume *= TimePeriodAudio[CurrentTimePeriod].Volume;
			}
			PrimaryAmbientAudio->SetVolumeMultiplier(ReducedVolume * MasterVolume);
		}
	}
	else
	{
		// No weather sound, fade out weather audio
		if (WeatherAmbientAudio && WeatherAmbientAudio->IsPlaying())
		{
			WeatherAmbientAudio->FadeOut(CrossfadeDuration, 0.0f);
		}

		// Restore time-based audio volume
		if (PrimaryAmbientAudio)
		{
			float Volume = 1.0f;
			if (TimePeriodAudio.Contains(CurrentTimePeriod))
			{
				Volume = TimePeriodAudio[CurrentTimePeriod].Volume;
			}
			PrimaryAmbientAudio->SetVolumeMultiplier(Volume * MasterVolume);
		}
	}
}

void UAmbientSFXComponent::UpdateCrossfade(float DeltaTime)
{
	CrossfadeProgress += DeltaTime / CrossfadeDuration;

	if (CrossfadeProgress >= 1.0f)
	{
		CrossfadeProgress = 1.0f;
		bIsCrossfading = false;

		// Stop secondary completely
		if (SecondaryAmbientAudio)
		{
			SecondaryAmbientAudio->Stop();
		}

		// Set primary to target volume
		if (PrimaryAmbientAudio)
		{
			PrimaryAmbientAudio->SetVolumeMultiplier(PrimaryTargetVolume * MasterVolume);
		}
	}
	else
	{
		// Interpolate volumes
		float PrimaryVolume = FMath::Lerp(0.0f, PrimaryTargetVolume, CrossfadeProgress);
		float SecondaryVolume = FMath::Lerp(SecondaryTargetVolume, 0.0f, CrossfadeProgress);

		if (PrimaryAmbientAudio)
		{
			PrimaryAmbientAudio->SetVolumeMultiplier(PrimaryVolume * MasterVolume);
		}
		if (SecondaryAmbientAudio)
		{
			SecondaryAmbientAudio->SetVolumeMultiplier(SecondaryVolume * MasterVolume);
		}
	}
}

void UAmbientSFXComponent::UpdateRandomSounds(float DeltaTime)
{
	RandomSoundTimer -= DeltaTime;

	if (RandomSoundTimer <= 0.0f)
	{
		TriggerRandomSound();
		RandomSoundTimer = FMath::FRandRange(RandomSoundInterval.X, RandomSoundInterval.Y);
	}
}

void UAmbientSFXComponent::SwapAudioComponents()
{
	UAudioComponent* Temp = SecondaryAmbientAudio;
	SecondaryAmbientAudio = PrimaryAmbientAudio;
	PrimaryAmbientAudio = Temp;

	// Record current secondary volume for fadeout
	if (SecondaryAmbientAudio)
	{
		SecondaryTargetVolume = SecondaryAmbientAudio->VolumeMultiplier / MasterVolume;
	}
}

void UAmbientSFXComponent::StopAllAmbient()
{
	if (PrimaryAmbientAudio)
	{
		PrimaryAmbientAudio->Stop();
	}
	if (SecondaryAmbientAudio)
	{
		SecondaryAmbientAudio->Stop();
	}
	if (WeatherAmbientAudio)
	{
		WeatherAmbientAudio->Stop();
	}

	bIsCrossfading = false;
}

void UAmbientSFXComponent::SetMasterVolume(float NewVolume)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.0f, 2.0f);

	// Update all active audio components
	if (PrimaryAmbientAudio && PrimaryAmbientAudio->IsPlaying())
	{
		float CurrentRatio = PrimaryAmbientAudio->VolumeMultiplier;
		PrimaryAmbientAudio->SetVolumeMultiplier(CurrentRatio); // Will use new MasterVolume
	}
	if (WeatherAmbientAudio && WeatherAmbientAudio->IsPlaying())
	{
		float CurrentRatio = WeatherAmbientAudio->VolumeMultiplier;
		WeatherAmbientAudio->SetVolumeMultiplier(CurrentRatio);
	}
}

void UAmbientSFXComponent::PlayOneShotSound(USoundBase* Sound, float VolumeMultiplier)
{
	if (!Sound)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound, VolumeMultiplier * MasterVolume);
}

void UAmbientSFXComponent::TriggerRandomSound()
{
	const TArray<USoundBase*>& SoundPool = GetRandomSoundsForCurrentTime();

	if (SoundPool.Num() == 0)
	{
		return;
	}

	int32 SoundIndex = FMath::RandRange(0, SoundPool.Num() - 1);
	USoundBase* Sound = SoundPool[SoundIndex];

	if (Sound)
	{
		// Random volume variation
		float Volume = FMath::FRandRange(0.6f, 1.0f);
		PlayOneShotSound(Sound, Volume);
	}
}

const TArray<USoundBase*>& UAmbientSFXComponent::GetRandomSoundsForCurrentTime() const
{
	// Check weather first (storm sounds)
	if (CurrentWeather == EWeatherType::Storm && RandomStormSounds.Num() > 0)
	{
		return RandomStormSounds;
	}

	// Check time period
	switch (CurrentTimePeriod)
	{
	case ETimePeriod::Evening:
	case ETimePeriod::Night:
	case ETimePeriod::LateNight:
		return RandomNightSounds;

	default:
		return RandomDaySounds;
	}
}

bool UAmbientSFXComponent::IsPlaying() const
{
	return (PrimaryAmbientAudio && PrimaryAmbientAudio->IsPlaying()) ||
		   (WeatherAmbientAudio && WeatherAmbientAudio->IsPlaying());
}
