// CallOfTheMoutains - Day/Night Gameplay Modifier Component
// Applies time/weather gameplay effects to actors

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DayNightTypes.h"
#include "DayNightGameplayModifier.generated.h"

class ADayNightManager;
class UWeatherSystem;
class UHealthComponent;

/**
 * Day/Night Gameplay Modifier Component
 *
 * Add to any actor that should be affected by time of day and weather.
 * Automatically subscribes to DayNightManager events and applies
 * appropriate gameplay modifiers.
 *
 * Usage:
 * 1. Add to player character, enemies, or any relevant actors
 * 2. Configure which modifiers should apply
 * 3. The component automatically handles integration with existing systems
 *
 * Integrates with:
 * - HealthComponent (stamina regen, damage modifiers)
 * - AI perception (detection range)
 * - Movement (speed modifiers)
 */
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UDayNightGameplayModifier : public UActorComponent
{
	GENERATED_BODY()

public:
	UDayNightGameplayModifier();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ==================== Configuration ====================

	/** Is this the player character? (receives different modifiers than enemies) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night Modifier")
	bool bIsPlayer = false;

	/** Apply stamina modifiers from time/weather? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night Modifier|Stamina")
	bool bApplyStaminaModifiers = true;

	/** Apply damage modifiers from time/weather? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night Modifier|Damage")
	bool bApplyDamageModifiers = true;

	/** Apply detection range modifiers? (for enemies) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night Modifier|Detection")
	bool bApplyDetectionModifiers = true;

	/** Apply movement speed modifiers from weather? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night Modifier|Movement")
	bool bApplyMovementModifiers = true;

	/** Base detection range (will be multiplied by time/weather modifiers) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night Modifier|Detection", meta = (ClampMin = "100.0"))
	float BaseDetectionRange = 1500.0f;

	/** Base hearing range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day Night Modifier|Detection", meta = (ClampMin = "100.0"))
	float BaseHearingRange = 2000.0f;

	// ==================== Current Modifiers ====================

	/** Get the current combined damage multiplier */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night Modifier")
	float GetCurrentDamageMultiplier() const { return CachedDamageMultiplier; }

	/** Get the current stamina regen multiplier */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night Modifier")
	float GetCurrentStaminaRegenMultiplier() const { return CachedStaminaRegenMultiplier; }

	/** Get the current detection range (after modifiers) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night Modifier")
	float GetCurrentDetectionRange() const { return CachedDetectionRange; }

	/** Get the current hearing range (after modifiers) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night Modifier")
	float GetCurrentHearingRange() const { return CachedHearingRange; }

	/** Get the current movement speed multiplier */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night Modifier")
	float GetCurrentMovementSpeedMultiplier() const { return CachedMovementSpeedMultiplier; }

	/** Get the current fire damage multiplier (from weather) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night Modifier")
	float GetCurrentFireDamageMultiplier() const { return CachedFireDamageMultiplier; }

	/** Get the current lightning damage multiplier (from weather) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Day Night Modifier")
	float GetCurrentLightningDamageMultiplier() const { return CachedLightningDamageMultiplier; }

	// ==================== Manual Update ====================

	/** Force update all modifiers */
	UFUNCTION(BlueprintCallable, Category = "Day Night Modifier")
	void RefreshModifiers();

protected:
	// ==================== Cached References ====================

	UPROPERTY()
	ADayNightManager* DayNightManager;

	UPROPERTY()
	UWeatherSystem* WeatherSystem;

	UPROPERTY()
	UHealthComponent* HealthComponent;

	// ==================== Cached Modifier Values ====================

	float CachedDamageMultiplier = 1.0f;
	float CachedStaminaRegenMultiplier = 1.0f;
	float CachedDetectionRange = 1500.0f;
	float CachedHearingRange = 2000.0f;
	float CachedMovementSpeedMultiplier = 1.0f;
	float CachedFireDamageMultiplier = 1.0f;
	float CachedLightningDamageMultiplier = 1.0f;
	float CachedStaminaDrainMultiplier = 1.0f;

	// Original values (for restoration)
	float OriginalStaminaRegenRate = 20.0f;
	float OriginalDamageMultiplier = 1.0f;

	// ==================== Internal Functions ====================

	/** Find and cache references */
	void CacheReferences();

	/** Calculate combined modifiers from time and weather */
	void CalculateModifiers();

	/** Apply modifiers to integrated components */
	void ApplyModifiersToComponents();

	// ==================== Event Handlers ====================

	UFUNCTION()
	void OnTimePeriodChanged(ETimePeriod NewPeriod, ETimePeriod OldPeriod);

	UFUNCTION()
	void OnWeatherChanged(EWeatherType NewWeather, EWeatherType OldWeather);
};
