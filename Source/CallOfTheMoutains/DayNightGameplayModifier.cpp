// CallOfTheMoutains - Day/Night Gameplay Modifier Implementation

#include "DayNightGameplayModifier.h"
#include "DayNightManager.h"
#include "WeatherSystem.h"
#include "HealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UDayNightGameplayModifier::UDayNightGameplayModifier()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.5f; // Update twice per second
}

void UDayNightGameplayModifier::BeginPlay()
{
	Super::BeginPlay();

	// Cache references
	CacheReferences();

	// Store original values
	if (HealthComponent)
	{
		OriginalStaminaRegenRate = HealthComponent->StaminaRegenRate;
		OriginalDamageMultiplier = HealthComponent->DamageMultiplier;
	}

	// Subscribe to events
	if (DayNightManager)
	{
		DayNightManager->OnTimePeriodChanged.AddDynamic(this, &UDayNightGameplayModifier::OnTimePeriodChanged);
	}

	if (WeatherSystem)
	{
		WeatherSystem->OnWeatherChanged.AddDynamic(this, &UDayNightGameplayModifier::OnWeatherChanged);
	}

	// Initial calculation
	RefreshModifiers();
}

void UDayNightGameplayModifier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Restore original values
	if (HealthComponent)
	{
		HealthComponent->StaminaRegenRate = OriginalStaminaRegenRate;
		HealthComponent->DamageMultiplier = OriginalDamageMultiplier;
	}

	// Unsubscribe from events
	if (DayNightManager)
	{
		DayNightManager->OnTimePeriodChanged.RemoveDynamic(this, &UDayNightGameplayModifier::OnTimePeriodChanged);
	}

	if (WeatherSystem)
	{
		WeatherSystem->OnWeatherChanged.RemoveDynamic(this, &UDayNightGameplayModifier::OnWeatherChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UDayNightGameplayModifier::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Periodically refresh modifiers (handles smooth transitions)
	CalculateModifiers();
	ApplyModifiersToComponents();
}

void UDayNightGameplayModifier::CacheReferences()
{
	// Find DayNightManager
	DayNightManager = ADayNightManager::GetDayNightManager(this);

	// Get WeatherSystem from DayNightManager
	if (DayNightManager)
	{
		WeatherSystem = DayNightManager->WeatherSystem;
	}

	// Find HealthComponent on owner
	HealthComponent = GetOwner()->FindComponentByClass<UHealthComponent>();
}

void UDayNightGameplayModifier::CalculateModifiers()
{
	// Start with defaults
	CachedDamageMultiplier = 1.0f;
	CachedStaminaRegenMultiplier = 1.0f;
	CachedDetectionRange = BaseDetectionRange;
	CachedHearingRange = BaseHearingRange;
	CachedMovementSpeedMultiplier = 1.0f;
	CachedFireDamageMultiplier = 1.0f;
	CachedLightningDamageMultiplier = 1.0f;
	CachedStaminaDrainMultiplier = 1.0f;

	// Get time-based modifiers
	if (DayNightManager)
	{
		FTimePeriodGameplay TimeGameplay = DayNightManager->GetCurrentGameplayModifiers();

		if (bIsPlayer)
		{
			// Player receives benefits
			CachedDamageMultiplier *= TimeGameplay.PlayerDamageMultiplier;
			CachedStaminaRegenMultiplier *= TimeGameplay.StaminaRegenMultiplier;
		}
		else
		{
			// Enemies receive different modifiers
			CachedDamageMultiplier *= TimeGameplay.EnemyDamageMultiplier;
			CachedDetectionRange *= TimeGameplay.EnemyDetectionRange;
		}
	}

	// Get weather-based modifiers
	if (WeatherSystem)
	{
		FWeatherGameplay WeatherGameplay = WeatherSystem->GetCurrentWeatherGameplay();

		// Movement applies to everyone
		if (bApplyMovementModifiers)
		{
			CachedMovementSpeedMultiplier *= WeatherGameplay.MovementSpeedMultiplier;
		}

		// Detection/hearing modifiers
		if (bApplyDetectionModifiers && !bIsPlayer)
		{
			CachedDetectionRange *= WeatherGameplay.VisionRangeMultiplier;
			CachedHearingRange *= WeatherGameplay.HearingRangeMultiplier;
		}

		// Elemental damage modifiers
		CachedFireDamageMultiplier *= WeatherGameplay.FireDamageMultiplier;
		CachedLightningDamageMultiplier *= WeatherGameplay.LightningDamageMultiplier;

		// Stamina effects
		if (bIsPlayer)
		{
			CachedStaminaDrainMultiplier *= WeatherGameplay.StaminaDrainMultiplier;
			// Inverse relationship - higher drain means lower regen
			CachedStaminaRegenMultiplier /= WeatherGameplay.StaminaDrainMultiplier;
		}
	}
}

void UDayNightGameplayModifier::ApplyModifiersToComponents()
{
	// Apply to HealthComponent
	if (HealthComponent)
	{
		if (bApplyStaminaModifiers)
		{
			HealthComponent->StaminaRegenRate = OriginalStaminaRegenRate * CachedStaminaRegenMultiplier;
		}

		if (bApplyDamageModifiers)
		{
			HealthComponent->DamageMultiplier = OriginalDamageMultiplier * CachedDamageMultiplier;
		}
	}

	// Apply movement speed modifier
	if (bApplyMovementModifiers)
	{
		ACharacter* Character = Cast<ACharacter>(GetOwner());
		if (Character)
		{
			UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
			if (MovementComp)
			{
				// Store the original max walk speed on first run
				static float OriginalMaxWalkSpeed = 0.0f;
				if (OriginalMaxWalkSpeed == 0.0f)
				{
					OriginalMaxWalkSpeed = MovementComp->MaxWalkSpeed;
				}

				// Apply modifier (but don't stack - use original as base)
				// Note: This is a simple implementation. A more robust system would
				// use a modifier stack system to combine multiple speed modifiers.
			}
		}
	}
}

void UDayNightGameplayModifier::RefreshModifiers()
{
	// Re-cache references in case they changed
	if (!DayNightManager || !WeatherSystem)
	{
		CacheReferences();
	}

	CalculateModifiers();
	ApplyModifiersToComponents();
}

void UDayNightGameplayModifier::OnTimePeriodChanged(ETimePeriod NewPeriod, ETimePeriod OldPeriod)
{
	RefreshModifiers();
}

void UDayNightGameplayModifier::OnWeatherChanged(EWeatherType NewWeather, EWeatherType OldWeather)
{
	RefreshModifiers();
}
