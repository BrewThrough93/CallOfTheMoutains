// CallOfTheMoutains - Interaction Prompt Widget Implementation

#include "InteractionPromptWidget.h"
#include "UIStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void UInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UInteractionPromptWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	PromptBackground.Reset();
	KeyText.Reset();
	PromptText.Reset();
}

TSharedRef<SWidget> UInteractionPromptWidget::RebuildWidget()
{
	using namespace COTMStyle;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0, 0, 0, 100.0f)) // Above hotbar
		[
			// Outer glow
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(Colors::GlowOuter())
			.Padding(FMargin(4.0f))
			[
				// Inner glow
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(Colors::GlowInner())
				.Padding(FMargin(3.0f))
				[
					// Rust border
					SAssignNew(PromptBackground, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(Colors::BorderRust())
					.Padding(FMargin(2.0f))
					[
						// Dark background
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(Colors::BackgroundDark())
						.Padding(FMargin(12.0f, 8.0f))
						[
							SNew(SHorizontalBox)
							// Key indicator [E]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(0, 0, 8.0f, 0))
							[
								// Key box
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(Colors::BorderIronLight())
								.Padding(FMargin(1.0f))
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(Colors::BackgroundSlot())
									.Padding(FMargin(8.0f, 4.0f))
									[
										SAssignNew(KeyText, STextBlock)
										.Text(FText::FromString(TEXT("E")))
										.Font(Fonts::SubHeader())
										.ColorAndOpacity(FSlateColor(Colors::AccentAmber()))
									]
								]
							]
							// Prompt text
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SAssignNew(PromptText, STextBlock)
								.Text(FText::FromString(TEXT("Interact")))
								.Font(Fonts::Body())
								.ColorAndOpacity(FSlateColor(Colors::TextPrimary()))
							]
						]
					]
				]
			]
		];
}

void UInteractionPromptWidget::ShowPrompt(const FText& InPromptText)
{
	if (PromptText.IsValid())
	{
		PromptText->SetText(InPromptText);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsVisible = true;
}

void UInteractionPromptWidget::HidePrompt()
{
	SetVisibility(ESlateVisibility::Collapsed);
	bIsVisible = false;
}
