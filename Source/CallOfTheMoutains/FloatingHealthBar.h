// CallOfTheMoutains - Floating Health Bar Widget
// Simple animated health bar for enemies/NPCs

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingHealthBar.generated.h"

class SProgressBar;
class SBorder;

/**
 * Floating Health Bar - displays above enemies/NPCs
 * Animates smoothly on damage with a trailing "damage" bar
 */
UCLASS()
class CALLOFTHEMOUTAINS_API UFloatingHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set health percentage (0-1) with optional animation */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealthPercent(float NewPercent, bool bAnimate = true);

	/** Flash the bar (on damage) */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void FlashDamage();

	/** Show/hide the bar */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetBarVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	// ==================== Settings ====================

	/** Width of the health bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float BarWidth = 120.0f;

	/** Height of the health bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float BarHeight = 8.0f;

	/** Main health bar color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor HealthColor = FLinearColor(0.6f, 0.08f, 0.08f, 1.0f);

	/** Damage trail color (shows recent damage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor DamageTrailColor = FLinearColor(0.9f, 0.2f, 0.1f, 1.0f);

	/** Background color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.02f, 0.8f);

	/** Border color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BorderColor = FLinearColor(0.15f, 0.12f, 0.1f, 0.9f);

	/** How fast the main bar animates */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "1.0"))
	float HealthAnimSpeed = 8.0f;

	/** How fast the damage trail catches up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0.5"))
	float TrailAnimSpeed = 2.0f;

	/** Delay before trail starts catching up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0.0"))
	float TrailDelay = 0.4f;

private:
	// Slate widgets
	TSharedPtr<SProgressBar> HealthBar;
	TSharedPtr<SProgressBar> DamageTrailBar;
	TSharedPtr<SBorder> ContainerBorder;

	// Animation state
	float TargetPercent = 1.0f;
	float CurrentPercent = 1.0f;
	float TrailPercent = 1.0f;
	float TrailDelayTimer = 0.0f;

	// Damage flash
	float FlashTimer = 0.0f;
	bool bIsFlashing = false;
};
