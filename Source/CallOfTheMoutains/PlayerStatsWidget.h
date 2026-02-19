// CallOfTheMoutains - Player Stats HUD Widget (Health/Stamina)
// Souls-like design with dark dystopian aesthetic - positioned top-left

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatsWidget.generated.h"

class UHealthComponent;
class SImage;
class SBorder;
class SBox;

/**
 * Player Stats Widget - Souls-like horizontal health and stamina bars
 * Dark dystopian aesthetic with rusted metal frames
 * Positioned in top-left corner
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

	// ==================== Layout Settings ====================

	/** Width of the health bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	float HealthBarWidth = 280.0f;

	/** Height of the health bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	float HealthBarHeight = 18.0f;

	/** Width of the stamina bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	float StaminaBarWidth = 220.0f;

	/** Height of the stamina bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	float StaminaBarHeight = 10.0f;

	/** Padding from top-left corner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	FVector2D CornerPadding = FVector2D(32.0f, 32.0f);

	/** Spacing between health and stamina bars */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	float BarSpacing = 6.0f;

	/** Frame thickness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Layout")
	float FrameThickness = 2.0f;

	// ==================== Visual Settings ====================

	/** Number of segments in the health bar (0 = no segments) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Visual")
	int32 HealthBarSegments = 10;

	/** Number of segments in the stamina bar (0 = no segments) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Visual")
	int32 StaminaBarSegments = 8;

	/** Health threshold for critical state (flashing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalHealthThreshold = 0.25f;

	// ==================== State ====================

	/** Reference to player's health component */
	UPROPERTY()
	UHealthComponent* HealthComponent;

	// ==================== Event Handlers ====================

	UFUNCTION()
	void OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina, float Delta);

	UFUNCTION()
	void OnDeath(AActor* KilledBy, AController* InstigatorController);

private:
	/** Build a single stat bar with frame and fill */
	TSharedRef<SWidget> BuildStatBar(
		TSharedPtr<SBox>& OutFillBox,
		TSharedPtr<SBox>& OutDamageBox,
		TSharedPtr<SBorder>& OutFillBorder,
		float Width,
		float Height,
		int32 Segments,
		const FLinearColor& FillColor,
		const FLinearColor& BackgroundColor
	);

	/** Update health bar visual */
	void UpdateHealthBar();

	/** Update stamina bar visual */
	void UpdateStaminaBar();

	// Slate widget references - using SBox for size control
	TSharedPtr<SBox> HealthFillBox;
	TSharedPtr<SBox> HealthDamageBox;
	TSharedPtr<SBorder> HealthFillBorder;
	TSharedPtr<SBox> StaminaFillBox;
	TSharedPtr<SBox> StaminaDamageBox;
	TSharedPtr<SBorder> StaminaFillBorder;
	TSharedPtr<SBorder> HealthFrameBorder;
	TSharedPtr<SBorder> StaminaFrameBorder;

	// Animation state
	float AnimationTime = 0.0f;
	float TargetHealthPercent = 1.0f;
	float DisplayedHealthPercent = 1.0f;
	float TargetStaminaPercent = 1.0f;
	float DisplayedStaminaPercent = 1.0f;
	float DamageFlashTimer = 0.0f;

	// Damage trail effect (the "ghost" bar that trails behind health loss)
	float DamageTrailPercent = 1.0f;
	float DamageTrailDelay = 0.0f;
};
