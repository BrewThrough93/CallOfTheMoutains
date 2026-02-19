// CallOfTheMoutains - Dystopian Post Process Component
// Atmospheric presets for a worn-down concrete world

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PostProcessComponent.h"
#include "DystopianPostProcess.generated.h"

/**
 * Visual atmosphere presets
 */
UENUM(BlueprintType)
enum class EDystopianPreset : uint8
{
	// Default grim atmosphere - desaturated, cold, oppressive
	Grim			UMETA(DisplayName = "Grim (Default)"),

	// Industrial decay - rust tones, high contrast, smoky
	Industrial		UMETA(DisplayName = "Industrial Decay"),

	// Ashen wasteland - very desaturated, gray skies, dust
	Ashen			UMETA(DisplayName = "Ashen Wasteland"),

	// Underground/interior - dark, warm artificial light, claustrophobic
	Underground		UMETA(DisplayName = "Underground"),

	// Toxic zones - sickly green tint, hazardous feel
	Toxic			UMETA(DisplayName = "Toxic Zone"),

	// Memory/flashback - sepia, dreamlike, soft
	Memory			UMETA(DisplayName = "Memory/Flashback"),

	// Combat intensity - heightened contrast, slight red
	Combat			UMETA(DisplayName = "Combat Intensity"),

	// Custom - use manual settings
	Custom			UMETA(DisplayName = "Custom")
};

/**
 * Post-process settings structure for easy tweaking
 */
USTRUCT(BlueprintType)
struct FDystopianSettings
{
	GENERATED_BODY()

	// ==================== Color Grading ====================

	/** Overall saturation (0 = grayscale, 1 = normal, >1 = oversaturated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Saturation = 0.7f;

	/** Contrast (1 = normal, <1 = flat, >1 = punchy) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float Contrast = 1.15f;

	/** Gamma adjustment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float Gamma = 0.95f;

	/** Color temperature shift (-1 = cold/blue, 0 = neutral, 1 = warm/orange) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Temperature = -0.15f;

	/** Tint shift for overall color cast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
	FLinearColor ColorTint = FLinearColor(1.0f, 0.98f, 0.95f, 1.0f);

	/** Shadow tint - color of dark areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
	FLinearColor ShadowTint = FLinearColor(0.9f, 0.92f, 1.0f, 1.0f);

	/** Highlight tint - color of bright areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
	FLinearColor HighlightTint = FLinearColor(1.0f, 0.98f, 0.92f, 1.0f);

	// ==================== Vignette ====================

	/** Vignette intensity (darkening at edges) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vignette", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VignetteIntensity = 0.5f;

	// ==================== Film Effects ====================

	/** Film grain intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FilmGrain = 0.08f;

	/** Film grain response - how grain varies with brightness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FilmGrainHighlights = 0.5f;

	// ==================== Bloom ====================

	/** Bloom intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloom", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BloomIntensity = 0.3f;

	/** Bloom threshold - brightness level where bloom starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloom", meta = (ClampMin = "-1.0", ClampMax = "8.0"))
	float BloomThreshold = 1.0f;

	// ==================== Chromatic Aberration ====================

	/** Chromatic aberration intensity (color fringing at edges) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aberration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChromaticAberration = 0.1f;

	// ==================== Ambient Occlusion ====================

	/** AO intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AO", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AOIntensity = 0.6f;

	/** AO radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AO", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float AORadius = 80.0f;

	// ==================== Exposure ====================

	/** Exposure compensation (EV adjustment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exposure", meta = (ClampMin = "-5.0", ClampMax = "5.0"))
	float ExposureCompensation = 0.0f;

	/** Min auto exposure brightness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exposure")
	float AutoExposureMin = 0.5f;

	/** Max auto exposure brightness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exposure")
	float AutoExposureMax = 2.0f;
};

/**
 * Dystopian Post Process Component
 *
 * Attach to your camera or player to apply atmospheric post-processing.
 * Choose from presets or customize individual settings.
 *
 * Usage:
 * 1. Add to your PlayerController or CameraActor
 * 2. Select a preset or set to Custom and tweak settings
 * 3. Use SetPreset() to change atmosphere at runtime (entering buildings, combat, etc.)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UDystopianPostProcess : public UActorComponent
{
	GENERATED_BODY()

public:
	UDystopianPostProcess();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ==================== Configuration ====================

	/** Current visual preset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dystopian|Preset")
	EDystopianPreset CurrentPreset = EDystopianPreset::Grim;

	/** Custom settings (used when preset is Custom, or as base for modifications) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dystopian|Settings")
	FDystopianSettings Settings;

	/** Blend time when transitioning between presets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dystopian|Transition", meta = (ClampMin = "0.0"))
	float PresetBlendTime = 1.5f;

	/** Priority for this post process volume (higher = takes precedence) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dystopian|Settings")
	float PostProcessPriority = 100.0f;

	// ==================== Functions ====================

	/** Set a new atmosphere preset with optional blend */
	UFUNCTION(BlueprintCallable, Category = "Dystopian")
	void SetPreset(EDystopianPreset NewPreset, bool bInstant = false);

	/** Get settings for a specific preset */
	UFUNCTION(BlueprintCallable, Category = "Dystopian")
	FDystopianSettings GetPresetSettings(EDystopianPreset Preset) const;

	/** Apply current settings to the post process */
	UFUNCTION(BlueprintCallable, Category = "Dystopian")
	void ApplySettings();

	/** Blend between two setting configurations */
	UFUNCTION(BlueprintCallable, Category = "Dystopian")
	void BlendToSettings(const FDystopianSettings& TargetSettings, float BlendTime);

	/** Temporarily intensify effects (for damage, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Dystopian")
	void PulseEffect(float Intensity = 0.5f, float Duration = 0.3f);

protected:
	/** The actual post process component */
	UPROPERTY()
	UPostProcessComponent* PostProcessComponent;

	// Blending state
	bool bIsBlending = false;
	float BlendAlpha = 0.0f;
	float BlendDuration = 1.0f;
	FDystopianSettings BlendStartSettings;
	FDystopianSettings BlendTargetSettings;

	// Pulse effect state
	bool bIsPulsing = false;
	float PulseTimer = 0.0f;
	float PulseDuration = 0.0f;
	float PulseIntensity = 0.0f;

	/** Create and configure the post process component */
	void CreatePostProcessComponent();

	/** Lerp between two settings */
	FDystopianSettings LerpSettings(const FDystopianSettings& A, const FDystopianSettings& B, float Alpha) const;

	/** Apply settings to the post process component */
	void ApplyToPostProcess(const FDystopianSettings& InSettings);
};
