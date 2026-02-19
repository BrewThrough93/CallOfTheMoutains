// CallOfTheMoutains - Player Stats HUD Widget Implementation
// Souls-like design with dark dystopian aesthetic

#include "PlayerStatsWidget.h"
#include "UIStyle.h"
#include "HealthComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Styling/CoreStyle.h"

// ============================================================================
// Souls-like Color Palette
// ============================================================================
namespace SoulsColors
{
	// Health bar - deep crimson blood red
	inline FLinearColor HealthFill()      { return FLinearColor(0.7f, 0.12f, 0.08f, 1.0f); }
	inline FLinearColor HealthDamage()    { return FLinearColor(0.95f, 0.3f, 0.15f, 0.8f); }  // Trail after damage
	inline FLinearColor HealthCritical()  { return FLinearColor(0.5f, 0.05f, 0.05f, 1.0f); }  // Low health
	inline FLinearColor HealthBackground(){ return FLinearColor(0.15f, 0.03f, 0.02f, 0.9f); }

	// Stamina bar - cold grey-teal (contrasts warm health)
	inline FLinearColor StaminaFill()     { return FLinearColor(0.25f, 0.55f, 0.45f, 1.0f); }
	inline FLinearColor StaminaDepleted() { return FLinearColor(0.12f, 0.25f, 0.2f, 1.0f); }
	inline FLinearColor StaminaBackground(){ return FLinearColor(0.04f, 0.08f, 0.07f, 0.85f); }

	// Frame - rusted iron
	inline FLinearColor FrameOuter()      { return FLinearColor(0.35f, 0.2f, 0.1f, 0.95f); }   // Rust
	inline FLinearColor FrameInner()      { return FLinearColor(0.15f, 0.12f, 0.1f, 0.9f); }   // Dark iron
	inline FLinearColor FrameHighlight()  { return FLinearColor(0.5f, 0.3f, 0.15f, 0.6f); }    // Edge highlight

	// Segment dividers
	inline FLinearColor SegmentLine()     { return FLinearColor(0.0f, 0.0f, 0.0f, 0.5f); }
}

void UPlayerStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPlayerStatsWidget::NativeDestruct()
{
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &UPlayerStatsWidget::OnHealthChanged);
		HealthComponent->OnStaminaChanged.RemoveDynamic(this, &UPlayerStatsWidget::OnStaminaChanged);
		HealthComponent->OnDeath.RemoveDynamic(this, &UPlayerStatsWidget::OnDeath);
	}
	Super::NativeDestruct();
}

void UPlayerStatsWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	HealthFillBox.Reset();
	HealthDamageBox.Reset();
	HealthFillBorder.Reset();
	StaminaFillBox.Reset();
	StaminaDamageBox.Reset();
	StaminaFillBorder.Reset();
	HealthFrameBorder.Reset();
	StaminaFrameBorder.Reset();
}

TSharedRef<SWidget> UPlayerStatsWidget::RebuildWidget()
{
	// Main container - anchored to top-right
	return SNew(SBox)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0, CornerPadding.Y, CornerPadding.X, 0))
		[
			SNew(SVerticalBox)
			// Health Bar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(FMargin(0, 0, 0, BarSpacing))
			[
				BuildStatBar(
					HealthFillBox,
					HealthDamageBox,
					HealthFillBorder,
					HealthBarWidth,
					HealthBarHeight,
					HealthBarSegments,
					SoulsColors::HealthFill(),
					SoulsColors::HealthBackground()
				)
			]
			// Stamina Bar - slightly offset for visual interest
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(FMargin(0, 0, 16.0f, 0))  // Indented from right edge
			[
				BuildStatBar(
					StaminaFillBox,
					StaminaDamageBox,
					StaminaFillBorder,
					StaminaBarWidth,
					StaminaBarHeight,
					StaminaBarSegments,
					SoulsColors::StaminaFill(),
					SoulsColors::StaminaBackground()
				)
			]
		];
}

TSharedRef<SWidget> UPlayerStatsWidget::BuildStatBar(
	TSharedPtr<SBox>& OutFillBox,
	TSharedPtr<SBox>& OutDamageBox,
	TSharedPtr<SBorder>& OutFillBorder,
	float Width,
	float Height,
	int32 Segments,
	const FLinearColor& FillColor,
	const FLinearColor& BackgroundColor)
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const float InnerHeight = Height - (FrameThickness * 2) - 2.0f;

	// Build segment overlay if segments are enabled
	TSharedPtr<SHorizontalBox> SegmentOverlay;
	if (Segments > 0)
	{
		SAssignNew(SegmentOverlay, SHorizontalBox);
		float SegmentWidth = Width / Segments;
		for (int32 i = 0; i < Segments - 1; ++i)
		{
			SegmentOverlay->AddSlot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(SegmentWidth)
				];
			// Add segment divider line
			SegmentOverlay->AddSlot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(1.0f)
					.HeightOverride(InnerHeight)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(SoulsColors::SegmentLine())
					]
				];
		}
	}

	// The stat bar structure:
	// [Outer Frame] -> [Inner Shadow] -> [Background + Damage Trail + Fill]
	return SNew(SOverlay)
		// Layer 0: Drop shadow
		+ SOverlay::Slot()
		.Padding(FMargin(3.0f, 3.0f, 0, 0))
		[
			SNew(SBox)
			.WidthOverride(Width + FrameThickness * 2)
			.HeightOverride(Height + FrameThickness * 2)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
			]
		]
		// Layer 1: Outer frame (rust color)
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.WidthOverride(Width + FrameThickness * 2)
			.HeightOverride(Height + FrameThickness * 2)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(SoulsColors::FrameOuter())
				.Padding(FMargin(FrameThickness))
				[
					// Inner frame (dark iron)
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(SoulsColors::FrameInner())
					.Padding(FMargin(1.0f))
					[
						// Background
						SNew(SOverlay)
						// Background fill
						+ SOverlay::Slot()
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(BackgroundColor)
						]
						// Damage trail bar (shows where health was) - wrapped in SBox for size control
						// Drains from right to left
						+ SOverlay::Slot()
						.HAlign(HAlign_Right)
						[
							SAssignNew(OutDamageBox, SBox)
							.WidthOverride(Width)  // Full width initially
							.HeightOverride(InnerHeight)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(SoulsColors::HealthDamage())
							]
						]
						// Main fill bar - wrapped in SBox for size control
						// Drains from right to left
						+ SOverlay::Slot()
						.HAlign(HAlign_Right)
						[
							SAssignNew(OutFillBox, SBox)
							.WidthOverride(Width)  // Full width initially
							.HeightOverride(InnerHeight)
							[
								SAssignNew(OutFillBorder, SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FillColor)
							]
						]
					]
				]
			]
		]
		// Layer 2: Top edge highlight (subtle bevel effect)
		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.WidthOverride(Width + FrameThickness * 2)
			.HeightOverride(1.0f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(SoulsColors::FrameHighlight())
			]
		]
		// Layer 3: Segment dividers (if enabled)
		+ SOverlay::Slot()
		.Padding(FMargin(FrameThickness + 1.0f, FrameThickness + 1.0f, 0, 0))
		[
			SegmentOverlay.IsValid() ? SegmentOverlay.ToSharedRef() : SNullWidget::NullWidget
		];
}

void UPlayerStatsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AnimationTime += InDeltaTime;

	if (!HealthComponent)
	{
		return;
	}

	// Get current target values
	TargetHealthPercent = HealthComponent->GetHealthPercent();
	TargetStaminaPercent = HealthComponent->GetStaminaPercent();

	// Smooth the displayed values for fluid animation
	const float SmoothSpeed = 8.0f;
	DisplayedHealthPercent = FMath::FInterpTo(DisplayedHealthPercent, TargetHealthPercent, InDeltaTime, SmoothSpeed);
	DisplayedStaminaPercent = FMath::FInterpTo(DisplayedStaminaPercent, TargetStaminaPercent, InDeltaTime, SmoothSpeed * 1.5f);

	// Damage trail effect - delay before it catches up
	if (DamageTrailDelay > 0.0f)
	{
		DamageTrailDelay -= InDeltaTime;
	}
	else
	{
		// Trail slowly catches up to actual health
		DamageTrailPercent = FMath::FInterpTo(DamageTrailPercent, TargetHealthPercent, InDeltaTime, 3.0f);
	}

	// Update damage flash
	if (DamageFlashTimer > 0.0f)
	{
		DamageFlashTimer -= InDeltaTime;
	}

	// Update visual displays
	UpdateHealthBar();
	UpdateStaminaBar();
}

void UPlayerStatsWidget::UpdateHealthBar()
{
	if (!HealthFillBox.IsValid() || !HealthDamageBox.IsValid() || !HealthFillBorder.IsValid())
	{
		return;
	}

	// Calculate bar widths based on current percentages
	float InnerWidth = HealthBarWidth - (FrameThickness * 2) - 2.0f;  // Account for frame and inner padding
	float FillWidth = InnerWidth * FMath::Clamp(DisplayedHealthPercent, 0.0f, 1.0f);
	float TrailWidth = InnerWidth * FMath::Clamp(DamageTrailPercent, 0.0f, 1.0f);

	// Update SBox width overrides
	HealthFillBox->SetWidthOverride(FillWidth);
	HealthDamageBox->SetWidthOverride(TrailWidth);

	// Color based on health state
	FLinearColor FillColor;
	if (HealthComponent && HealthComponent->IsDead())
	{
		FillColor = FLinearColor(0.1f, 0.05f, 0.05f, 0.5f);
	}
	else if (TargetHealthPercent <= CriticalHealthThreshold)
	{
		// Critical health - pulse effect
		float Pulse = (FMath::Sin(AnimationTime * 6.0f) + 1.0f) * 0.5f;
		FillColor = FMath::Lerp(SoulsColors::HealthCritical(), SoulsColors::HealthFill(), Pulse * 0.5f);
	}
	else if (DamageFlashTimer > 0.0f)
	{
		// Flash white briefly on damage
		float Flash = DamageFlashTimer / 0.15f;
		FillColor = FMath::Lerp(SoulsColors::HealthFill(), FLinearColor(1.0f, 0.9f, 0.8f, 1.0f), Flash * 0.3f);
	}
	else
	{
		FillColor = SoulsColors::HealthFill();
	}

	HealthFillBorder->SetBorderBackgroundColor(FillColor);
}

void UPlayerStatsWidget::UpdateStaminaBar()
{
	if (!StaminaFillBox.IsValid() || !StaminaFillBorder.IsValid())
	{
		return;
	}

	// Calculate bar width
	float InnerWidth = StaminaBarWidth - (FrameThickness * 2) - 2.0f;
	float FillWidth = InnerWidth * FMath::Clamp(DisplayedStaminaPercent, 0.0f, 1.0f);

	// Update SBox width override
	StaminaFillBox->SetWidthOverride(FillWidth);

	// Color interpolation based on stamina level
	FLinearColor FillColor;
	if (TargetStaminaPercent < 0.15f)
	{
		// Very low stamina - darker, depleted look
		FillColor = SoulsColors::StaminaDepleted();
	}
	else
	{
		// Normal stamina - interpolate from depleted to full
		FillColor = FMath::Lerp(SoulsColors::StaminaDepleted(), SoulsColors::StaminaFill(), TargetStaminaPercent);
	}

	StaminaFillBorder->SetBorderBackgroundColor(FillColor);

	// Hide damage bar for stamina (not needed)
	if (StaminaDamageBox.IsValid())
	{
		StaminaDamageBox->SetWidthOverride(0.0f);
	}
}

void UPlayerStatsWidget::InitializeStats(UHealthComponent* InHealthComponent)
{
	HealthComponent = InHealthComponent;

	if (!HealthComponent)
	{
		return;
	}

	// Bind to health component events
	HealthComponent->OnHealthChanged.AddDynamic(this, &UPlayerStatsWidget::OnHealthChanged);
	HealthComponent->OnStaminaChanged.AddDynamic(this, &UPlayerStatsWidget::OnStaminaChanged);
	HealthComponent->OnDeath.AddDynamic(this, &UPlayerStatsWidget::OnDeath);

	// Initialize display values
	TargetHealthPercent = HealthComponent->GetHealthPercent();
	DisplayedHealthPercent = TargetHealthPercent;
	DamageTrailPercent = TargetHealthPercent;
	TargetStaminaPercent = HealthComponent->GetStaminaPercent();
	DisplayedStaminaPercent = TargetStaminaPercent;

	// Initial update
	UpdateDisplay();
}

void UPlayerStatsWidget::UpdateDisplay()
{
	UpdateHealthBar();
	UpdateStaminaBar();
}

void UPlayerStatsWidget::OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	if (Delta < 0.0f)
	{
		// Took damage - start damage flash and trail delay
		DamageFlashTimer = 0.15f;
		DamageTrailDelay = 0.5f;  // Trail waits before catching up
	}
}

void UPlayerStatsWidget::OnStaminaChanged(float CurrentStamina, float MaxStamina, float Delta)
{
	// Stamina changes are handled in tick for smooth animation
}

void UPlayerStatsWidget::OnDeath(AActor* KilledBy, AController* InstigatorController)
{
	// Death state is handled in UpdateHealthBar
}
