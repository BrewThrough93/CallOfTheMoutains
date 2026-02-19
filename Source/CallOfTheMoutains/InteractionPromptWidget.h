// CallOfTheMoutains - Interaction Prompt Widget
// Shows "[E] Prompt Text" when looking at interactables

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

class STextBlock;
class SBorder;

/**
 * Widget that displays interaction prompt
 * Positioned at bottom-center of screen
 */
UCLASS()
class CALLOFTHEMOUTAINS_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Show the prompt with given text */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void ShowPrompt(const FText& PromptText);

	/** Hide the prompt */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void HidePrompt();

	/** Check if prompt is visible */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	bool IsPromptVisible() const { return bIsVisible; }

protected:
	virtual void NativeConstruct() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// Slate widgets
	TSharedPtr<SBorder> PromptBackground;
	TSharedPtr<STextBlock> KeyText;
	TSharedPtr<STextBlock> PromptText;

	// State
	bool bIsVisible = false;
};
