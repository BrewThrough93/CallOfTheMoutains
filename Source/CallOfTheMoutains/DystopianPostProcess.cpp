// CallOfTheMoutains - Dystopian Post Process Component Implementation

#include "DystopianPostProcess.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/Actor.h"

UDystopianPostProcess::UDystopianPostProcess()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f; // Every frame for smooth blending
}

void UDystopianPostProcess::BeginPlay()
{
	Super::BeginPlay();

	CreatePostProcessComponent();

	// Apply initial preset
	Settings = GetPresetSettings(CurrentPreset);
	ApplySettings();
}

void UDystopianPostProcess::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bNeedsUpdate = false;

	// Handle preset blending
	if (bIsBlending)
	{
		BlendAlpha += DeltaTime / BlendDuration;

		if (BlendAlpha >= 1.0f)
		{
			BlendAlpha = 1.0f;
			bIsBlending = false;
			Settings = BlendTargetSettings;
		}
		else
		{
			Settings = LerpSettings(BlendStartSettings, BlendTargetSettings, BlendAlpha);
		}

		bNeedsUpdate = true;
	}

	// Handle pulse effect
	if (bIsPulsing)
	{
		PulseTimer -= DeltaTime;

		if (PulseTimer <= 0.0f)
		{
			bIsPulsing = false;
		}

		bNeedsUpdate = true;
	}

	if (bNeedsUpdate)
	{
		ApplySettings();
	}
}

void UDystopianPostProcess::CreatePostProcessComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Create post process component with unique name
	PostProcessComponent = NewObject<UPostProcessComponent>(Owner, TEXT("DystopianPP_Component"));
	if (PostProcessComponent)
	{
		PostProcessComponent->RegisterComponent();
		PostProcessComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

		// Configure as unbound (affects entire scene)
		PostProcessComponent->bUnbound = true;
		PostProcessComponent->Priority = PostProcessPriority;
	}
}

void UDystopianPostProcess::SetPreset(EDystopianPreset NewPreset, bool bInstant)
{
	if (NewPreset == CurrentPreset && !bInstant)
	{
		return;
	}

	CurrentPreset = NewPreset;
	FDystopianSettings NewSettings = GetPresetSettings(NewPreset);

	if (bInstant || PresetBlendTime <= 0.0f)
	{
		Settings = NewSettings;
		ApplySettings();
	}
	else
	{
		BlendToSettings(NewSettings, PresetBlendTime);
	}
}

FDystopianSettings UDystopianPostProcess::GetPresetSettings(EDystopianPreset Preset) const
{
	FDystopianSettings Result;

	switch (Preset)
	{
	case EDystopianPreset::Grim:
		// Default grim atmosphere - the worn concrete world
		// Cold, desaturated, oppressive but not completely drained of color
		Result.Saturation = 0.65f;
		Result.Contrast = 1.15f;
		Result.Gamma = 1.02f;
		Result.Temperature = -0.12f;
		Result.ColorTint = FLinearColor(0.98f, 0.96f, 0.94f, 1.0f);
		Result.ShadowTint = FLinearColor(0.88f, 0.90f, 0.96f, 1.0f); // Cold blue shadows
		Result.HighlightTint = FLinearColor(1.0f, 0.97f, 0.92f, 1.0f); // Warm sickly highlights
		Result.VignetteIntensity = 0.4f;
		Result.FilmGrain = 0.05f;
		Result.FilmGrainHighlights = 0.4f;
		Result.BloomIntensity = 0.3f;
		Result.BloomThreshold = 1.0f;
		Result.ChromaticAberration = 0.08f;
		Result.AOIntensity = 0.55f;
		Result.AORadius = 90.0f;
		Result.ExposureCompensation = 0.2f;
		Result.AutoExposureMin = 0.6f;
		Result.AutoExposureMax = 2.4f;
		break;

	case EDystopianPreset::Industrial:
		// Rust, decay, smoke, harsh industrial lighting
		Result.Saturation = 0.55f;
		Result.Contrast = 1.25f;
		Result.Gamma = 0.88f;
		Result.Temperature = 0.15f; // Warmer - rust and fire
		Result.ColorTint = FLinearColor(1.02f, 0.95f, 0.88f, 1.0f); // Orange/rust tint
		Result.ShadowTint = FLinearColor(0.7f, 0.75f, 0.85f, 1.0f); // Deep cold shadows
		Result.HighlightTint = FLinearColor(1.1f, 0.95f, 0.8f, 1.0f); // Fiery highlights
		Result.VignetteIntensity = 0.55f;
		Result.FilmGrain = 0.1f;
		Result.FilmGrainHighlights = 0.6f;
		Result.BloomIntensity = 0.4f;
		Result.BloomThreshold = 0.8f;
		Result.ChromaticAberration = 0.15f;
		Result.AOIntensity = 0.75f;
		Result.AORadius = 120.0f;
		Result.ExposureCompensation = -0.4f;
		Result.AutoExposureMin = 0.3f;
		Result.AutoExposureMax = 1.5f;
		break;

	case EDystopianPreset::Ashen:
		// Nearly monochrome, ash and dust everywhere, gray skies
		Result.Saturation = 0.35f;
		Result.Contrast = 1.1f;
		Result.Gamma = 1.0f;
		Result.Temperature = -0.05f;
		Result.ColorTint = FLinearColor(0.95f, 0.95f, 0.96f, 1.0f); // Neutral gray
		Result.ShadowTint = FLinearColor(0.9f, 0.9f, 0.92f, 1.0f);
		Result.HighlightTint = FLinearColor(1.0f, 0.99f, 0.97f, 1.0f);
		Result.VignetteIntensity = 0.35f;
		Result.FilmGrain = 0.12f; // More grain - dusty
		Result.FilmGrainHighlights = 0.7f;
		Result.BloomIntensity = 0.5f; // Hazy bloom
		Result.BloomThreshold = 0.6f;
		Result.ChromaticAberration = 0.05f;
		Result.AOIntensity = 0.5f;
		Result.AORadius = 60.0f;
		Result.ExposureCompensation = 0.2f; // Slightly overexposed, washed out
		Result.AutoExposureMin = 0.6f;
		Result.AutoExposureMax = 2.0f;
		break;

	case EDystopianPreset::Underground:
		// Dark, claustrophobic, artificial warm light sources
		Result.Saturation = 0.75f;
		Result.Contrast = 1.3f;
		Result.Gamma = 0.85f;
		Result.Temperature = 0.25f; // Warm artificial light
		Result.ColorTint = FLinearColor(1.05f, 0.98f, 0.9f, 1.0f);
		Result.ShadowTint = FLinearColor(0.6f, 0.65f, 0.75f, 1.0f); // Deep blue-black shadows
		Result.HighlightTint = FLinearColor(1.15f, 1.0f, 0.85f, 1.0f); // Warm bulb highlights
		Result.VignetteIntensity = 0.65f; // Heavy vignette - claustrophobic
		Result.FilmGrain = 0.04f;
		Result.FilmGrainHighlights = 0.3f;
		Result.BloomIntensity = 0.35f;
		Result.BloomThreshold = 0.5f;
		Result.ChromaticAberration = 0.12f;
		Result.AOIntensity = 0.85f; // Heavy AO - cramped spaces
		Result.AORadius = 150.0f;
		Result.ExposureCompensation = -0.6f;
		Result.AutoExposureMin = 0.2f;
		Result.AutoExposureMax = 1.2f;
		break;

	case EDystopianPreset::Toxic:
		// Sickly green/yellow, hazardous atmosphere
		Result.Saturation = 0.6f;
		Result.Contrast = 1.15f;
		Result.Gamma = 0.95f;
		Result.Temperature = 0.1f;
		Result.ColorTint = FLinearColor(0.95f, 1.05f, 0.9f, 1.0f); // Green tint
		Result.ShadowTint = FLinearColor(0.7f, 0.85f, 0.7f, 1.0f); // Green shadows
		Result.HighlightTint = FLinearColor(1.0f, 1.1f, 0.85f, 1.0f); // Yellow-green highlights
		Result.VignetteIntensity = 0.5f;
		Result.FilmGrain = 0.08f;
		Result.FilmGrainHighlights = 0.5f;
		Result.BloomIntensity = 0.45f;
		Result.BloomThreshold = 0.7f;
		Result.ChromaticAberration = 0.2f; // Heavy aberration - distorted vision
		Result.AOIntensity = 0.6f;
		Result.AORadius = 80.0f;
		Result.ExposureCompensation = 0.1f;
		Result.AutoExposureMin = 0.5f;
		Result.AutoExposureMax = 1.8f;
		break;

	case EDystopianPreset::Memory:
		// Sepia, dreamlike, soft focus feel
		Result.Saturation = 0.4f;
		Result.Contrast = 0.95f;
		Result.Gamma = 1.05f;
		Result.Temperature = 0.3f; // Warm sepia
		Result.ColorTint = FLinearColor(1.1f, 1.0f, 0.85f, 1.0f); // Sepia tint
		Result.ShadowTint = FLinearColor(0.95f, 0.9f, 0.8f, 1.0f);
		Result.HighlightTint = FLinearColor(1.15f, 1.05f, 0.9f, 1.0f);
		Result.VignetteIntensity = 0.6f;
		Result.FilmGrain = 0.15f; // Heavy grain - old film
		Result.FilmGrainHighlights = 0.8f;
		Result.BloomIntensity = 0.7f; // Heavy bloom - dreamy
		Result.BloomThreshold = 0.4f;
		Result.ChromaticAberration = 0.03f;
		Result.AOIntensity = 0.3f;
		Result.AORadius = 40.0f;
		Result.ExposureCompensation = 0.3f;
		Result.AutoExposureMin = 0.7f;
		Result.AutoExposureMax = 2.5f;
		break;

	case EDystopianPreset::Combat:
		// Heightened intensity, slight red, punchy contrast
		Result.Saturation = 0.8f;
		Result.Contrast = 1.35f;
		Result.Gamma = 0.9f;
		Result.Temperature = 0.05f;
		Result.ColorTint = FLinearColor(1.05f, 0.98f, 0.95f, 1.0f); // Slight red
		Result.ShadowTint = FLinearColor(0.8f, 0.75f, 0.85f, 1.0f);
		Result.HighlightTint = FLinearColor(1.1f, 1.0f, 0.95f, 1.0f);
		Result.VignetteIntensity = 0.55f;
		Result.FilmGrain = 0.04f;
		Result.FilmGrainHighlights = 0.3f;
		Result.BloomIntensity = 0.3f;
		Result.BloomThreshold = 1.0f;
		Result.ChromaticAberration = 0.12f;
		Result.AOIntensity = 0.7f;
		Result.AORadius = 90.0f;
		Result.ExposureCompensation = -0.2f;
		Result.AutoExposureMin = 0.4f;
		Result.AutoExposureMax = 1.5f;
		break;

	case EDystopianPreset::Custom:
	default:
		// Return current settings as-is
		Result = Settings;
		break;
	}

	return Result;
}

void UDystopianPostProcess::ApplySettings()
{
	FDystopianSettings EffectiveSettings = Settings;

	// Apply pulse effect modification
	if (bIsPulsing && PulseDuration > 0.0f)
	{
		float PulseAlpha = PulseTimer / PulseDuration;
		// Pulse affects contrast and vignette
		EffectiveSettings.Contrast += PulseIntensity * 0.3f * PulseAlpha;
		EffectiveSettings.VignetteIntensity += PulseIntensity * 0.3f * PulseAlpha;
		EffectiveSettings.ChromaticAberration += PulseIntensity * 0.2f * PulseAlpha;
		EffectiveSettings.Saturation -= PulseIntensity * 0.2f * PulseAlpha;
	}

	ApplyToPostProcess(EffectiveSettings);
}

void UDystopianPostProcess::BlendToSettings(const FDystopianSettings& TargetSettings, float BlendTime)
{
	BlendStartSettings = Settings;
	BlendTargetSettings = TargetSettings;
	BlendDuration = FMath::Max(BlendTime, 0.01f);
	BlendAlpha = 0.0f;
	bIsBlending = true;
}

void UDystopianPostProcess::PulseEffect(float Intensity, float Duration)
{
	bIsPulsing = true;
	PulseIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	PulseDuration = FMath::Max(Duration, 0.01f);
	PulseTimer = PulseDuration;
}

FDystopianSettings UDystopianPostProcess::LerpSettings(const FDystopianSettings& A, const FDystopianSettings& B, float Alpha) const
{
	FDystopianSettings Result;

	Result.Saturation = FMath::Lerp(A.Saturation, B.Saturation, Alpha);
	Result.Contrast = FMath::Lerp(A.Contrast, B.Contrast, Alpha);
	Result.Gamma = FMath::Lerp(A.Gamma, B.Gamma, Alpha);
	Result.Temperature = FMath::Lerp(A.Temperature, B.Temperature, Alpha);
	Result.ColorTint = FMath::Lerp(A.ColorTint, B.ColorTint, Alpha);
	Result.ShadowTint = FMath::Lerp(A.ShadowTint, B.ShadowTint, Alpha);
	Result.HighlightTint = FMath::Lerp(A.HighlightTint, B.HighlightTint, Alpha);
	Result.VignetteIntensity = FMath::Lerp(A.VignetteIntensity, B.VignetteIntensity, Alpha);
	Result.FilmGrain = FMath::Lerp(A.FilmGrain, B.FilmGrain, Alpha);
	Result.FilmGrainHighlights = FMath::Lerp(A.FilmGrainHighlights, B.FilmGrainHighlights, Alpha);
	Result.BloomIntensity = FMath::Lerp(A.BloomIntensity, B.BloomIntensity, Alpha);
	Result.BloomThreshold = FMath::Lerp(A.BloomThreshold, B.BloomThreshold, Alpha);
	Result.ChromaticAberration = FMath::Lerp(A.ChromaticAberration, B.ChromaticAberration, Alpha);
	Result.AOIntensity = FMath::Lerp(A.AOIntensity, B.AOIntensity, Alpha);
	Result.AORadius = FMath::Lerp(A.AORadius, B.AORadius, Alpha);
	Result.ExposureCompensation = FMath::Lerp(A.ExposureCompensation, B.ExposureCompensation, Alpha);
	Result.AutoExposureMin = FMath::Lerp(A.AutoExposureMin, B.AutoExposureMin, Alpha);
	Result.AutoExposureMax = FMath::Lerp(A.AutoExposureMax, B.AutoExposureMax, Alpha);

	return Result;
}

void UDystopianPostProcess::ApplyToPostProcess(const FDystopianSettings& InSettings)
{
	if (!PostProcessComponent)
	{
		return;
	}

	FPostProcessSettings& PP = PostProcessComponent->Settings;

	// ==================== Color Grading ====================

	// Global saturation
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(InSettings.Saturation, InSettings.Saturation, InSettings.Saturation, 1.0f);

	// Contrast
	PP.bOverride_ColorContrast = true;
	PP.ColorContrast = FVector4(InSettings.Contrast, InSettings.Contrast, InSettings.Contrast, 1.0f);

	// Gamma
	PP.bOverride_ColorGamma = true;
	PP.ColorGamma = FVector4(InSettings.Gamma, InSettings.Gamma, InSettings.Gamma, 1.0f);

	// Global color tint (gain)
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(InSettings.ColorTint.R, InSettings.ColorTint.G, InSettings.ColorTint.B, 1.0f);

	// Shadow tint
	PP.bOverride_ColorGainShadows = true;
	PP.ColorGainShadows = FVector4(InSettings.ShadowTint.R, InSettings.ShadowTint.G, InSettings.ShadowTint.B, 1.0f);

	// Highlight tint
	PP.bOverride_ColorGainHighlights = true;
	PP.ColorGainHighlights = FVector4(InSettings.HighlightTint.R, InSettings.HighlightTint.G, InSettings.HighlightTint.B, 1.0f);

	// Temperature (white balance)
	PP.bOverride_WhiteTemp = true;
	// Temperature is typically 1500-15000K, we map -1 to 1 range to cool-warm
	PP.WhiteTemp = 6500.0f + (InSettings.Temperature * -2000.0f); // Negative because cold = higher value in our system

	// ==================== Vignette ====================

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = InSettings.VignetteIntensity;

	// ==================== Film Effects ====================

	PP.bOverride_FilmGrainIntensity = true;
	PP.FilmGrainIntensity = InSettings.FilmGrain;

	PP.bOverride_FilmGrainIntensityShadows = true;
	PP.FilmGrainIntensityShadows = InSettings.FilmGrain * 1.2f;

	PP.bOverride_FilmGrainIntensityMidtones = true;
	PP.FilmGrainIntensityMidtones = InSettings.FilmGrain;

	PP.bOverride_FilmGrainIntensityHighlights = true;
	PP.FilmGrainIntensityHighlights = InSettings.FilmGrain * InSettings.FilmGrainHighlights;

	// ==================== Bloom ====================

	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = InSettings.BloomIntensity;

	PP.bOverride_BloomThreshold = true;
	PP.BloomThreshold = InSettings.BloomThreshold;

	// ==================== Chromatic Aberration ====================

	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = InSettings.ChromaticAberration;

	// ==================== Ambient Occlusion ====================

	PP.bOverride_AmbientOcclusionIntensity = true;
	PP.AmbientOcclusionIntensity = InSettings.AOIntensity;

	PP.bOverride_AmbientOcclusionRadius = true;
	PP.AmbientOcclusionRadius = InSettings.AORadius;

	// ==================== Exposure ====================

	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = InSettings.ExposureCompensation;

	PP.bOverride_AutoExposureMinBrightness = true;
	PP.AutoExposureMinBrightness = InSettings.AutoExposureMin;

	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.AutoExposureMaxBrightness = InSettings.AutoExposureMax;
}
