// CallOfTheMoutains - Faith Currency Widget Implementation
// Bottom-right souls-like currency display

#include "FaithWidget.h"
#include "UIStyle.h"
#include "FaithComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

// ============================================================================
// Faith UI Colors - Matching souls aesthetic
// ============================================================================
namespace FaithColors
{
	// Icon - golden amber glow (like souls/embers)
	inline FLinearColor IconGlow()        { return FLinearColor(0.95f, 0.75f, 0.25f, 1.0f); }
	inline FLinearColor IconCore()        { return FLinearColor(1.0f, 0.85f, 0.4f, 1.0f); }
	inline FLinearColor IconDim()         { return FLinearColor(0.6f, 0.45f, 0.15f, 0.8f); }

	// Text colors
	inline FLinearColor TextNormal()      { return FLinearColor(0.9f, 0.85f, 0.75f, 0.95f); }
	inline FLinearColor TextGain()        { return FLinearColor(0.4f, 0.9f, 0.4f, 1.0f); }    // Green for gains
	inline FLinearColor TextLoss()        { return FLinearColor(0.9f, 0.3f, 0.2f, 1.0f); }    // Red for losses

	// Container
	inline FLinearColor Background()      { return FLinearColor(0.02f, 0.02f, 0.02f, 0.7f); }
	inline FLinearColor Border()          { return FLinearColor(0.3f, 0.2f, 0.1f, 0.6f); }
}

void UFaithWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UFaithWidget::NativeDestruct()
{
	if (FaithComponent)
	{
		FaithComponent->OnFaithChanged.RemoveDynamic(this, &UFaithWidget::OnFaithChanged);
	}
	Super::NativeDestruct();
}

void UFaithWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	FaithAmountText.Reset();
	FaithDeltaText.Reset();
	ContainerBorder.Reset();
	IconBorder.Reset();
}

TSharedRef<SWidget> UFaithWidget::RebuildWidget()
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Main container - anchored to bottom-right
	return SNew(SBox)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0, 0, CornerPadding.X, CornerPadding.Y))
		[
			SNew(SOverlay)
			// Shadow layer
			+ SOverlay::Slot()
			.Padding(FMargin(2.0f, 2.0f, 0, 0))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f))
				.Padding(FMargin(12.0f, 8.0f))
				[
					SNew(SBox)
					.WidthOverride(120.0f)
					.HeightOverride(IconSize + 8.0f)
				]
			]
			// Main container
			+ SOverlay::Slot()
			[
				SAssignNew(ContainerBorder, SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FaithColors::Background())
				.Padding(FMargin(12.0f, 8.0f))
				[
					SNew(SHorizontalBox)
					// Faith Icon (stylized diamond/ember shape using borders)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0, 0, IconTextSpacing, 0))
					[
						SNew(SOverlay)
						// Outer glow
						+ SOverlay::Slot()
						[
							SNew(SBox)
							.WidthOverride(IconSize + 4.0f)
							.HeightOverride(IconSize + 4.0f)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(0.95f, 0.7f, 0.2f, 0.3f))
							]
						]
						// Inner icon (diamond shape approximated)
						+ SOverlay::Slot()
						.Padding(FMargin(2.0f))
						[
							SAssignNew(IconBorder, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FaithColors::IconGlow())
							[
								SNew(SBox)
								.WidthOverride(IconSize)
								.HeightOverride(IconSize)
								[
									// Inner core
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(FaithColors::IconCore())
									.Padding(FMargin(3.0f))
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(FLinearColor(1.0f, 0.95f, 0.7f, 0.9f))
									]
								]
							]
						]
					]
					// Faith amount text
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						// Main amount
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(FaithAmountText, STextBlock)
							.Text(FText::FromString(TEXT("0")))
							.Font(Fonts::SubHeader())
							.ColorAndOpacity(FSlateColor(FaithColors::TextNormal()))
							.ShadowOffset(FVector2D(1.0f, 1.0f))
							.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f))
						]
						// Delta indicator (shows +/- when faith changes)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(FaithDeltaText, STextBlock)
							.Text(FText::GetEmpty())
							.Font(Fonts::Small())
							.ColorAndOpacity(FSlateColor(FaithColors::TextGain()))
							.Visibility(EVisibility::Collapsed)
						]
					]
				]
			]
			// Border overlay
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FaithColors::Border())
				.Padding(FMargin(1.0f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(FLinearColor::Transparent)
				]
			]
		];
}

void UFaithWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AnimationTime += InDeltaTime;

	// Smooth counting animation
	if (DisplayedFaith != TargetFaith)
	{
		// Calculate how fast to count based on difference
		int32 Difference = FMath::Abs(TargetFaith - DisplayedFaith);
		float Speed = FMath::Max(Difference * 5.0f, 50.0f);  // At least 50 per second

		if (DisplayedFaith < TargetFaith)
		{
			DisplayedFaith = FMath::Min(DisplayedFaith + FMath::CeilToInt(Speed * InDeltaTime), TargetFaith);
		}
		else
		{
			DisplayedFaith = FMath::Max(DisplayedFaith - FMath::CeilToInt(Speed * InDeltaTime), TargetFaith);
		}

		if (FaithAmountText.IsValid())
		{
			FaithAmountText->SetText(FText::FromString(FormatFaithNumber(DisplayedFaith)));
		}
	}

	// Delta display timer
	if (DeltaDisplayTimer > 0.0f)
	{
		DeltaDisplayTimer -= InDeltaTime;

		if (FaithDeltaText.IsValid())
		{
			// Fade out effect
			float Alpha = FMath::Clamp(DeltaDisplayTimer / 0.5f, 0.0f, 1.0f);
			FLinearColor DeltaColor = bLastWasGain ? FaithColors::TextGain() : FaithColors::TextLoss();
			DeltaColor.A = Alpha;
			FaithDeltaText->SetColorAndOpacity(FSlateColor(DeltaColor));

			if (DeltaDisplayTimer <= 0.0f)
			{
				FaithDeltaText->SetVisibility(EVisibility::Collapsed);
			}
		}
	}

	// Icon pulse effect
	if (IconBorder.IsValid())
	{
		// Subtle breathing effect
		float Pulse = (FMath::Sin(AnimationTime * 2.0f) + 1.0f) * 0.5f;
		FLinearColor IconColor = FMath::Lerp(FaithColors::IconDim(), FaithColors::IconGlow(), 0.7f + Pulse * 0.3f);
		IconBorder->SetBorderBackgroundColor(IconColor);
	}
}

void UFaithWidget::InitializeFaith(UFaithComponent* InFaithComponent)
{
	FaithComponent = InFaithComponent;

	if (!FaithComponent)
	{
		return;
	}

	// Bind to faith events
	FaithComponent->OnFaithChanged.AddDynamic(this, &UFaithWidget::OnFaithChanged);

	// Initialize display
	TargetFaith = FaithComponent->GetFaith();
	DisplayedFaith = TargetFaith;

	UpdateDisplay();
}

void UFaithWidget::UpdateDisplay()
{
	if (!FaithComponent)
	{
		return;
	}

	if (FaithAmountText.IsValid())
	{
		FaithAmountText->SetText(FText::FromString(FormatFaithNumber(DisplayedFaith)));
	}
}

void UFaithWidget::OnFaithChanged(int32 CurrentFaith, int32 Delta, bool bWasGained)
{
	TargetFaith = CurrentFaith;
	LastDelta = Delta;
	bLastWasGain = bWasGained;

	// Show delta indicator
	if (FaithDeltaText.IsValid() && Delta != 0)
	{
		FString DeltaStr = bWasGained ?
			FString::Printf(TEXT("+%s"), *FormatFaithNumber(FMath::Abs(Delta))) :
			FString::Printf(TEXT("-%s"), *FormatFaithNumber(FMath::Abs(Delta)));

		FaithDeltaText->SetText(FText::FromString(DeltaStr));
		FaithDeltaText->SetColorAndOpacity(FSlateColor(bWasGained ? FaithColors::TextGain() : FaithColors::TextLoss()));
		FaithDeltaText->SetVisibility(EVisibility::Visible);

		DeltaDisplayTimer = bWasGained ? GainAnimationDuration : LossAnimationDuration;
	}

	// Pulse the icon on gain
	if (bWasGained && IconBorder.IsValid())
	{
		IconBorder->SetBorderBackgroundColor(FaithColors::IconCore());
	}
}

FString UFaithWidget::FormatFaithNumber(int32 Amount) const
{
	// Add thousand separators for readability
	FString NumberStr = FString::FromInt(Amount);

	if (NumberStr.Len() <= 3)
	{
		return NumberStr;
	}

	FString Result;
	int32 Count = 0;

	for (int32 i = NumberStr.Len() - 1; i >= 0; --i)
	{
		if (Count > 0 && Count % 3 == 0)
		{
			Result = FString(TEXT(",")) + Result;
		}
		Result = FString(1, &NumberStr[i]) + Result;
		Count++;
	}

	return Result;
}
