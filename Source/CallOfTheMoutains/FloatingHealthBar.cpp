// CallOfTheMoutains - Floating Health Bar Implementation

#include "FloatingHealthBar.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"

void UFloatingHealthBar::NativeConstruct()
{
	Super::NativeConstruct();
}

void UFloatingHealthBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	bool bNeedsUpdate = false;

	// Animate main health bar
	if (!FMath::IsNearlyEqual(CurrentPercent, TargetPercent, 0.001f))
	{
		CurrentPercent = FMath::FInterpTo(CurrentPercent, TargetPercent, InDeltaTime, HealthAnimSpeed);
		if (HealthBar.IsValid())
		{
			HealthBar->SetPercent(CurrentPercent);
		}
		bNeedsUpdate = true;
	}

	// Handle trail delay
	if (TrailDelayTimer > 0.0f)
	{
		TrailDelayTimer -= InDeltaTime;
	}

	// Animate damage trail (catches up after delay)
	if (TrailDelayTimer <= 0.0f && !FMath::IsNearlyEqual(TrailPercent, TargetPercent, 0.001f))
	{
		TrailPercent = FMath::FInterpTo(TrailPercent, TargetPercent, InDeltaTime, TrailAnimSpeed);
		if (DamageTrailBar.IsValid())
		{
			DamageTrailBar->SetPercent(TrailPercent);
		}
		bNeedsUpdate = true;
	}

	// Handle damage flash
	if (bIsFlashing)
	{
		FlashTimer -= InDeltaTime;
		if (FlashTimer <= 0.0f)
		{
			bIsFlashing = false;
			// Reset border color
			if (ContainerBorder.IsValid())
			{
				ContainerBorder->SetBorderBackgroundColor(BorderColor);
			}
		}
		else
		{
			// Pulse the border
			float FlashAlpha = FlashTimer / 0.2f;
			FLinearColor FlashColor = FMath::Lerp(BorderColor, FLinearColor(1.0f, 0.3f, 0.2f, 1.0f), FlashAlpha);
			if (ContainerBorder.IsValid())
			{
				ContainerBorder->SetBorderBackgroundColor(FlashColor);
			}
		}
	}
}

void UFloatingHealthBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	HealthBar.Reset();
	DamageTrailBar.Reset();
	ContainerBorder.Reset();
}

TSharedRef<SWidget> UFloatingHealthBar::RebuildWidget()
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	return SNew(SBox)
		.WidthOverride(BarWidth + 4.0f)
		.HeightOverride(BarHeight + 4.0f)
		[
			// Outer border
			SAssignNew(ContainerBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(BorderColor)
			.Padding(FMargin(1.5f))
			[
				// Background
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(BackgroundColor)
				[
					SNew(SOverlay)
					// Damage trail bar (behind main bar)
					+ SOverlay::Slot()
					[
						SAssignNew(DamageTrailBar, SProgressBar)
						.Percent(1.0f)
						.FillColorAndOpacity(DamageTrailColor)
						.BackgroundImage(nullptr)
						.FillImage(WhiteBrush)
					]
					// Main health bar
					+ SOverlay::Slot()
					[
						SAssignNew(HealthBar, SProgressBar)
						.Percent(1.0f)
						.FillColorAndOpacity(HealthColor)
						.BackgroundImage(nullptr)
						.FillImage(WhiteBrush)
					]
				]
			]
		];
}

void UFloatingHealthBar::SetHealthPercent(float NewPercent, bool bAnimate)
{
	NewPercent = FMath::Clamp(NewPercent, 0.0f, 1.0f);

	// If taking damage, trigger trail delay
	if (NewPercent < TargetPercent)
	{
		TrailDelayTimer = TrailDelay;
		FlashDamage();
	}
	// If healing, trail should catch up instantly
	else if (NewPercent > TargetPercent)
	{
		TrailPercent = NewPercent;
		if (DamageTrailBar.IsValid())
		{
			DamageTrailBar->SetPercent(TrailPercent);
		}
	}

	TargetPercent = NewPercent;

	if (!bAnimate)
	{
		CurrentPercent = NewPercent;
		TrailPercent = NewPercent;
		TrailDelayTimer = 0.0f;

		if (HealthBar.IsValid())
		{
			HealthBar->SetPercent(CurrentPercent);
		}
		if (DamageTrailBar.IsValid())
		{
			DamageTrailBar->SetPercent(TrailPercent);
		}
	}
}

void UFloatingHealthBar::FlashDamage()
{
	bIsFlashing = true;
	FlashTimer = 0.2f;
}

void UFloatingHealthBar::SetBarVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
