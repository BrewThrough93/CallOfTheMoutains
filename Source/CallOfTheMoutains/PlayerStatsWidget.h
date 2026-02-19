// CallOfTheMoutains - Player Stats HUD Widget (Health/Stamina)
// Top-right display using COTMStyle aesthetic

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatsWidget.generated.h"

class UHealthComponent;
class UMaterialInstanceDynamic;
class SImage;
class SProgressBar;
class SBorder;

/**
 * Player Stats Widget - Displays health (EKG material) and stamina (progress bar)
 * Uses Slate widgets with COTMStyle for consistent Dark Souls aesthetic
 * Positioned in top-right corner
 */
UCLASS()
class CALLOFTHEMOUTAINS_API UPlayerStatsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Initialize with player's health component */
	UFUNCTION(BlueprintCallable, Category = "PlayerStats")
	void InitializeStats(UHealthComponent* InHealthComponent);

	/** Manually update display (called automatically on events) */
	UFUNCTION(BlueprintCallable, Category = "PlayerStats")
	void UpdateDisplay();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	// ==================== Settings ====================

	/** Path to the EKG material instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Materials")
	FString EKGMaterialPath = TEXT("/Game/ProceduralOscillator/MaterialInstances/MI_EKG_Inst");

	/** Health threshold for medium color (percentage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Colors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MediumHealthThreshold = 0.6f;

	/** Health threshold for low color (percentage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Colors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.3f;

	/** Size of the health EKG display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	FVector2D HealthImageSize = FVector2D(220.0f, 40.0f);

	/** Size of the stamina bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	FVector2D StaminaBarSize = FVector2D(180.0f, 6.0f);

	/** Padding from top-right corner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	FVector2D CornerPadding = FVector2D(24.0f, 24.0f);

	// ==================== State ====================

	/** Reference to player's health component */
	UPROPERTY()
	UHealthComponent* HealthComponent;

	/** Dynamic material instance for health display */
	UPROPERTY()
	UMaterialInstanceDynamic* HealthMaterialInstance;

	// ==================== Event Handlers ====================

	UFUNCTION()
	void OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina, float Delta);

	UFUNCTION()
	void OnDeath(AActor* KilledBy, AController* InstigatorController);

private:
	/** Get health color based on current percentage (ember palette) */
	FLinearColor GetHealthColor(float HealthPercent) const;

	/** Update the EKG material parameters */
	void UpdateHealthMaterial();

	/** Update stamina bar display */
	void UpdateStaminaBar();

	/** Create the health material instance */
	void CreateHealthMaterial();

	// Slate widget references
	TSharedPtr<SImage> HealthImageWidget;
	TSharedPtr<SProgressBar> StaminaBarWidget;
	TSharedPtr<SBorder> HealthBorder;
	TSharedPtr<SBorder> StaminaBorder;

	// Material brush for health display
	FSlateBrush HealthBrush;

	// Animation state
	float AnimationTime = 0.0f;
	float LastHealthPercent = 1.0f;
	float DamageFlashTimer = 0.0f;
};
