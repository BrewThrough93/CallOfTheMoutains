// CallOfTheMoutains - Player Stats HUD Widget Implementation
// Dark Souls aesthetic with ember/rust color palette

#include "PlayerStatsWidget.h"
#include "UIStyle.h"
#include "HealthComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPlayerStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CreateHealthMaterial();
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

	HealthImageWidget.Reset();
	StaminaBarWidget.Reset();
	HealthBorder.Reset();
	StaminaBorder.Reset();
}

TSharedRef<SWidget> UPlayerStatsWidget::RebuildWidget()
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Floating elements - no container, subtle shadows via dark underlays
	return SNew(SBox)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0, CornerPadding.Y, CornerPadding.X, 0))
		[
			SNew(SVerticalBox)
			// Health EKG - floating with subtle shadow
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 8.0f))
			[
				SNew(SOverlay)
				// Shadow layer (offset dark copy)
				+ SOverlay::Slot()
				.Padding(FMargin(3.0f, 3.0f, 0, 0))
				[
					SNew(SBox)
					.WidthOverride(HealthImageSize.X)
					.HeightOverride(HealthImageSize.Y)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f))
					]
				]
				// Main EKG element
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.WidthOverride(HealthImageSize.X)
					.HeightOverride(HealthImageSize.Y)
					[
						SAssignNew(HealthBorder, SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.85f))
						[
							SAssignNew(HealthImageWidget, SImage)
							.Image(&HealthBrush)
						]
					]
				]
			]
			// Stamina bar - floating, offset slightly for visual interest
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(FMargin(0, 0, 12.0f, 0))
			[
				SNew(SOverlay)
				// Shadow layer
				+ SOverlay::Slot()
				.Padding(FMargin(2.0f, 2.0f, 0, 0))
				[
					SNew(SBox)
					.WidthOverride(StaminaBarSize.X)
					.HeightOverride(StaminaBarSize.Y)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f))
					]
				]
				// Main stamina bar
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.WidthOverride(StaminaBarSize.X)
					.HeightOverride(StaminaBarSize.Y)
					[
						SAssignNew(StaminaBorder, SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.7f))
						.Padding(FMargin(2.0f, 3.0f))
						[
							SAssignNew(StaminaBarWidget, SProgressBar)
							.Percent(1.0f)
							.BarFillType(EProgressBarFillType::RightToLeft)
							.FillColorAndOpacity(FLinearColor(0.85f, 0.7f, 0.25f, 1.0f))
							.BackgroundImage(WhiteBrush)
							.FillImage(WhiteBrush)
						]
					]
				]
			]
		];
}

void UPlayerStatsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AnimationTime += InDeltaTime;

	// Update EKG animation speed based on health (faster heartbeat at low health)
	if (HealthMaterialInstance && HealthComponent)
	{
		float HealthPercent = HealthComponent->GetHealthPercent();
		// Speed up heartbeat as health decreases (1.0 at full health, up to 3.0 at low health)
		float HeartbeatSpeed = FMath::Lerp(3.0f, 1.0f, HealthPercent);
		HealthMaterialInstance->SetScalarParameterValue(TEXT("Speed"), HeartbeatSpeed);

		// Detect damage taken - flash effect
		if (HealthPercent < LastHealthPercent - 0.01f)
		{
			DamageFlashTimer = 0.3f; // Flash for 0.3 seconds
		}
		LastHealthPercent = HealthPercent;

		// Update damage flash
		if (DamageFlashTimer > 0.0f)
		{
			DamageFlashTimer -= InDeltaTime;
		}

		// Subtle pulse at low health - just vary the alpha slightly
		if (HealthBorder.IsValid() && HealthPercent <= LowHealthThreshold && !HealthComponent->IsDead())
		{
			float Pulse = (FMath::Sin(AnimationTime * 4.0f) + 1.0f) * 0.5f;
			float Alpha = FMath::Lerp(0.75f, 0.95f, Pulse);
			HealthBorder->SetBorderBackgroundColor(FLinearColor(0.08f, 0.02f, 0.02f, Alpha));
		}
		else if (HealthBorder.IsValid() && DamageFlashTimer > 0.0f)
		{
			// Brief flash on damage - subtle red tint
			float Flash = DamageFlashTimer / 0.3f;
			float RedTint = FMath::Lerp(0.02f, 0.12f, Flash);
			HealthBorder->SetBorderBackgroundColor(FLinearColor(RedTint, 0.02f, 0.02f, 0.85f));
		}
		else if (HealthBorder.IsValid())
		{
			// Normal state - near black
			HealthBorder->SetBorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.85f));
		}
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

	// Initial update
	UpdateDisplay();
}

void UPlayerStatsWidget::UpdateDisplay()
{
	UpdateHealthMaterial();
	UpdateStaminaBar();
}

void UPlayerStatsWidget::CreateHealthMaterial()
{
	// Load the EKG material
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, *EKGMaterialPath);
	if (BaseMaterial)
	{
		HealthMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (HealthMaterialInstance)
		{
			// Set up brush to use the material
			HealthBrush.SetResourceObject(HealthMaterialInstance);
			HealthBrush.ImageSize = FVector2D(HealthImageSize.X, HealthImageSize.Y);
			HealthBrush.DrawAs = ESlateBrushDrawType::Image;

			// Update the image widget if it exists
			if (HealthImageWidget.IsValid())
			{
				HealthImageWidget->SetImage(&HealthBrush);
			}
		}
	}
}

void UPlayerStatsWidget::UpdateHealthMaterial()
{
	if (!HealthMaterialInstance || !HealthComponent)
	{
		return;
	}

	float HealthPercent = HealthComponent->GetHealthPercent();
	FLinearColor HealthColor = GetHealthColor(HealthPercent);

	// Update material parameters
	HealthMaterialInstance->SetVectorParameterValue(TEXT("Color"), HealthColor);
	HealthMaterialInstance->SetVectorParameterValue(TEXT("LineColor"), HealthColor);

	// If dead, flatline the EKG
	if (HealthComponent->IsDead())
	{
		HealthMaterialInstance->SetScalarParameterValue(TEXT("Amplitude"), 0.0f);
		if (HealthBorder.IsValid())
		{
			HealthBorder->SetBorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.6f));
		}
	}
	else
	{
		// Scale amplitude with health (lower health = weaker signal)
		float Amplitude = FMath::Lerp(0.4f, 1.0f, HealthPercent);
		HealthMaterialInstance->SetScalarParameterValue(TEXT("Amplitude"), Amplitude);
	}
}

void UPlayerStatsWidget::UpdateStaminaBar()
{
	if (!StaminaBarWidget.IsValid() || !HealthComponent)
	{
		return;
	}

	float StaminaPercent = HealthComponent->GetStaminaPercent();
	StaminaBarWidget->SetPercent(StaminaPercent);

	// Stamina color: warm gold when full, dims as depleted
	const FLinearColor StaminaFull = FLinearColor(0.9f, 0.75f, 0.3f, 1.0f);
	const FLinearColor StaminaLow = FLinearColor(0.4f, 0.25f, 0.1f, 1.0f);

	FLinearColor StaminaColor = FMath::Lerp(StaminaLow, StaminaFull, StaminaPercent);
	StaminaBarWidget->SetFillColorAndOpacity(StaminaColor);
}

FLinearColor UPlayerStatsWidget::GetHealthColor(float HealthPercent) const
{
	// Classic green → yellow → red health colors
	const FLinearColor HealthGreen = FLinearColor(0.2f, 0.9f, 0.3f, 1.0f);  // Vibrant green
	const FLinearColor HealthYellow = FLinearColor(1.0f, 0.85f, 0.1f, 1.0f); // Bright yellow
	const FLinearColor HealthRed = FLinearColor(0.95f, 0.15f, 0.1f, 1.0f);   // Vivid red

	if (HealthPercent <= LowHealthThreshold)
	{
		// Low health - red
		return HealthRed;
	}
	else if (HealthPercent <= MediumHealthThreshold)
	{
		// Medium health - interpolate between red and yellow
		float T = (HealthPercent - LowHealthThreshold) / (MediumHealthThreshold - LowHealthThreshold);
		return FMath::Lerp(HealthRed, HealthYellow, T);
	}
	else
	{
		// High health - interpolate between yellow and green
		float T = (HealthPercent - MediumHealthThreshold) / (1.0f - MediumHealthThreshold);
		return FMath::Lerp(HealthYellow, HealthGreen, T);
	}
}

void UPlayerStatsWidget::OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	UpdateHealthMaterial();
}

void UPlayerStatsWidget::OnStaminaChanged(float CurrentStamina, float MaxStamina, float Delta)
{
	UpdateStaminaBar();
}

void UPlayerStatsWidget::OnDeath(AActor* KilledBy, AController* InstigatorController)
{
	UpdateHealthMaterial();
}
