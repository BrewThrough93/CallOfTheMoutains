// CallOfTheMoutains - Faith Currency Widget
// Bottom-right display showing current faith amount

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FaithWidget.generated.h"

class UFaithComponent;
class STextBlock;
class SBorder;
class SImage;

/**
 * Faith Widget - Displays the player's faith currency
 * Positioned in bottom-right corner with souls-like aesthetic
 * Shows icon + current faith amount with gain/loss animations
 */
UCLASS()
class CALLOFTHEMOUTAINS_API UFaithWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Initialize with player's faith component */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	void InitializeFaith(UFaithComponent* InFaithComponent);

	/** Manually update display */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	void UpdateDisplay();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	// ==================== Layout Settings ====================

	/** Padding from bottom-right corner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faith|Layout")
	FVector2D CornerPadding = FVector2D(32.0f, 32.0f);

	/** Size of the faith icon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faith|Layout")
	float IconSize = 28.0f;

	/** Spacing between icon and text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faith|Layout")
	float IconTextSpacing = 8.0f;

	// ==================== Visual Settings ====================

	/** Duration of the gain animation (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faith|Visual")
	float GainAnimationDuration = 0.5f;

	/** Duration of the loss animation (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faith|Visual")
	float LossAnimationDuration = 0.8f;

	// ==================== State ====================

	/** Reference to player's faith component */
	UPROPERTY()
	UFaithComponent* FaithComponent;

	// ==================== Event Handlers ====================

	UFUNCTION()
	void OnFaithChanged(int32 CurrentFaith, int32 Delta, bool bWasGained);

private:
	/** Format faith number with commas */
	FString FormatFaithNumber(int32 Amount) const;

	// Slate widget references
	TSharedPtr<STextBlock> FaithAmountText;
	TSharedPtr<STextBlock> FaithDeltaText;
	TSharedPtr<SBorder> ContainerBorder;
	TSharedPtr<SBorder> IconBorder;

	// Animation state
	float AnimationTime = 0.0f;
	float DeltaDisplayTimer = 0.0f;
	int32 DisplayedFaith = 0;
	int32 TargetFaith = 0;
	int32 LastDelta = 0;
	bool bLastWasGain = false;

	// Smooth counting animation
	float CountSpeed = 0.0f;
};
