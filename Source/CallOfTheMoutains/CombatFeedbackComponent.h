// CallOfTheMoutains - Combat Feedback Component
// Handles camera shake, hitstop, VFX, slow-mo, and screen effects for immersive combat

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "CombatFeedbackComponent.generated.h"

class UCameraShakeBase;
class UDystopianPostProcess;
class UEquipmentComponent;
class UHealthComponent;
class UMeleeTraceComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UCameraComponent;
class USpringArmComponent;
class UMaterialParameterCollection;
class UMaterialInstanceDynamic;

// Forward declare the melee hit result
struct FMeleeHitResult;

/**
 * Combat feedback intensity preset
 */
UENUM(BlueprintType)
enum class ECombatFeedbackIntensity : uint8
{
	Light		UMETA(DisplayName = "Light"),		// Light attack, minor hit
	Medium		UMETA(DisplayName = "Medium"),		// Heavy attack, solid hit
	Heavy		UMETA(DisplayName = "Heavy"),		// Critical, parry, stagger
	Devastating	UMETA(DisplayName = "Devastating")	// Riposte, kill, boss hit
};

/**
 * Configuration for camera shake effects
 */
USTRUCT(BlueprintType)
struct FCameraShakeConfig
{
	GENERATED_BODY()

	/** Enable camera shake on hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
	bool bEnabled = true;

	/** Base shake intensity multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float IntensityMultiplier = 1.0f;

	/** Shake on dealing damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
	bool bShakeOnDealDamage = true;

	/** Shake on receiving damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
	bool bShakeOnReceiveDamage = true;

	/** Shake on successful parry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
	bool bShakeOnParry = true;
};

/**
 * Configuration for hitstop (time dilation on impact)
 */
USTRUCT(BlueprintType)
struct FHitstopConfig
{
	GENERATED_BODY()

	/** Enable hitstop effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop")
	bool bEnabled = true;

	/** Duration of hitstop for light hits (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float LightHitDuration = 0.03f;

	/** Duration of hitstop for medium hits (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float MediumHitDuration = 0.06f;

	/** Duration of hitstop for heavy hits (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float HeavyHitDuration = 0.1f;

	/** Duration of hitstop for devastating hits (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DevastatingHitDuration = 0.15f;

	/** Time dilation amount during hitstop (lower = slower) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TimeDilation = 0.05f;
};

/**
 * Configuration for screen effects (flash, vignette, chromatic aberration)
 */
USTRUCT(BlueprintType)
struct FScreenEffectsConfig
{
	GENERATED_BODY()

	/** Enable screen flash on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects")
	bool bFlashOnHit = true;

	/** Flash color for dealing damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects")
	FLinearColor DealDamageFlashColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);

	/** Flash color for receiving damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects")
	FLinearColor ReceiveDamageFlashColor = FLinearColor(1.0f, 0.2f, 0.1f, 0.4f);

	/** Flash duration (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlashDuration = 0.1f;

	/** Enable vignette pulse on low health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects")
	bool bLowHealthVignette = true;

	/** Health threshold for low health effects (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.25f;

	/** Enable chromatic aberration spike on heavy hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects")
	bool bChromaticAberrationSpike = true;

	/** Max chromatic aberration on spike */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Effects", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float ChromaticAberrationMax = 2.0f;
};

/**
 * Configuration for motion blur and FOV effects
 */
USTRUCT(BlueprintType)
struct FDynamicCameraConfig
{
	GENERATED_BODY()

	/** Enable motion blur spike on attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Camera")
	bool bMotionBlurOnAttack = true;

	/** Motion blur amount during attack (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AttackMotionBlurAmount = 0.5f;

	/** Enable FOV change during combat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Camera")
	bool bDynamicFOV = true;

	/** FOV increase when attacking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Camera", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float AttackFOVIncrease = 5.0f;

	/** FOV change speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Camera", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float FOVChangeSpeed = 10.0f;

	/** Enable radial blur on heavy attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Camera")
	bool bRadialBlurOnHeavyAttack = true;
};

/**
 * Configuration for slow-motion effects
 */
USTRUCT(BlueprintType)
struct FSlowMotionConfig
{
	GENERATED_BODY()

	/** Enable slow-mo on successful riposte */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion")
	bool bSlowMoOnRiposte = true;

	/** Duration of riposte slow-mo (seconds, real time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float RiposteDuration = 0.5f;

	/** Time dilation during riposte (lower = slower) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float RiposteTimeDilation = 0.3f;

	/** Enable slow-mo on kill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion")
	bool bSlowMoOnKill = true;

	/** Duration of kill slow-mo (seconds, real time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float KillDuration = 0.3f;

	/** Time dilation during kill (lower = slower) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float KillTimeDilation = 0.4f;

	/** Enable slow-mo on successful parry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion")
	bool bSlowMoOnParry = true;

	/** Duration of parry slow-mo (seconds, real time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ParryDuration = 0.2f;

	/** Time dilation during parry (lower = slower) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slow Motion", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ParryTimeDilation = 0.5f;
};

/**
 * Configuration for impact VFX
 */
USTRUCT(BlueprintType)
struct FImpactVFXConfig
{
	GENERATED_BODY()

	/** Enable impact particles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact VFX")
	bool bEnabled = true;

	/** Default impact particle system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact VFX")
	UNiagaraSystem* DefaultImpactVFX = nullptr;

	/** Blood/flesh impact particle system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact VFX")
	UNiagaraSystem* FleshImpactVFX = nullptr;

	/** Metal/armor impact particle system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact VFX")
	UNiagaraSystem* MetalImpactVFX = nullptr;

	/** Parry spark particle system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact VFX")
	UNiagaraSystem* ParrySparkVFX = nullptr;

	/** Scale multiplier for impact effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact VFX", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float VFXScaleMultiplier = 1.0f;
};

/**
 * Combat Feedback Component
 *
 * Attach to the PlayerController to enhance combat feel with:
 * - Camera shake on hits (dealing and receiving)
 * - Hitstop (brief time dilation on impact)
 * - Screen effects (flash, vignette, chromatic aberration)
 * - Motion blur and dynamic FOV
 * - Slow-motion for dramatic moments (riposte, kill)
 * - Impact VFX at hit locations
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UCombatFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatFeedbackComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== Configuration ====================

	/** Camera shake settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Camera Shake")
	FCameraShakeConfig CameraShakeConfig;

	/** Hitstop settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Hitstop")
	FHitstopConfig HitstopConfig;

	/** Screen effects settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Screen Effects")
	FScreenEffectsConfig ScreenEffectsConfig;

	/** Dynamic camera settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Dynamic Camera")
	FDynamicCameraConfig DynamicCameraConfig;

	/** Slow motion settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Slow Motion")
	FSlowMotionConfig SlowMotionConfig;

	/** Impact VFX settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|VFX")
	FImpactVFXConfig ImpactVFXConfig;

	// ==================== Camera Shake Classes ====================

	/** Camera shake for light hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Camera Shake|Classes")
	TSubclassOf<UCameraShakeBase> LightHitShake;

	/** Camera shake for medium hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Camera Shake|Classes")
	TSubclassOf<UCameraShakeBase> MediumHitShake;

	/** Camera shake for heavy hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Camera Shake|Classes")
	TSubclassOf<UCameraShakeBase> HeavyHitShake;

	/** Camera shake for devastating hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Camera Shake|Classes")
	TSubclassOf<UCameraShakeBase> DevastatingHitShake;

	/** Camera shake for receiving damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Camera Shake|Classes")
	TSubclassOf<UCameraShakeBase> DamageTakenShake;

	/** Camera shake for successful parry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Feedback|Camera Shake|Classes")
	TSubclassOf<UCameraShakeBase> ParryShake;

	// ==================== Public Functions ====================

	/** Trigger feedback for dealing a hit */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void OnDealHit(const FVector& HitLocation, ECombatFeedbackIntensity Intensity = ECombatFeedbackIntensity::Light);

	/** Trigger feedback for receiving damage */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void OnReceiveDamage(float DamageAmount, AActor* DamageCauser);

	/** Trigger feedback for successful parry */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void OnParrySuccess(AActor* ParriedActor);

	/** Trigger feedback for riposte */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void OnRiposte(AActor* Target);

	/** Trigger feedback for killing an enemy */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void OnKill(AActor* KilledActor);

	/** Trigger camera shake */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.0f);

	/** Trigger hitstop effect */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void PlayHitstop(ECombatFeedbackIntensity Intensity);

	/** Trigger screen flash */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void PlayScreenFlash(FLinearColor FlashColor, float Duration);

	/** Trigger slow motion */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void PlaySlowMotion(float Duration, float TimeDilation);

	/** Spawn impact VFX at location */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void SpawnImpactVFX(const FVector& Location, const FVector& Normal, bool bIsFlesh = true);

	/** Called when attack starts (for motion blur and FOV) */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void OnAttackStart(bool bIsHeavyAttack);

	/** Called when attack ends */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void OnAttackEnd();

	/** Set low health state (for pulsing vignette) */
	UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
	void SetLowHealthState(bool bIsLowHealth);

protected:
	// ==================== Cached References ====================

	UPROPERTY()
	APlayerController* CachedPlayerController;

	UPROPERTY()
	UDystopianPostProcess* CachedPostProcess;

	UPROPERTY()
	UEquipmentComponent* CachedEquipmentComponent;

	UPROPERTY()
	UHealthComponent* CachedHealthComponent;

	UPROPERTY()
	UMeleeTraceComponent* CachedMeleeTraceComponent;

	UPROPERTY()
	UCameraComponent* CachedCamera;

	UPROPERTY()
	USpringArmComponent* CachedCameraBoom;

	// ==================== Internal State ====================

	/** Is currently in hitstop */
	bool bInHitstop = false;

	/** Hitstop timer */
	float HitstopTimer = 0.0f;

	/** Target time dilation to restore */
	float OriginalTimeDilation = 1.0f;

	/** Is currently in slow motion */
	bool bInSlowMotion = false;

	/** Slow motion timer */
	float SlowMotionTimer = 0.0f;

	/** Slow motion duration */
	float SlowMotionDuration = 0.0f;

	/** Is currently attacking */
	bool bIsAttacking = false;

	/** Is heavy attack active */
	bool bIsHeavyAttack = false;

	/** Base FOV to return to */
	float BaseFOV = 90.0f;

	/** Current target FOV */
	float TargetFOV = 90.0f;

	/** Is in low health state */
	bool bIsLowHealth = false;

	/** Low health vignette pulse timer */
	float LowHealthPulseTimer = 0.0f;

	/** Screen flash timer */
	float ScreenFlashTimer = 0.0f;

	/** Screen flash duration */
	float ScreenFlashDuration = 0.0f;

	/** Screen flash color */
	FLinearColor CurrentFlashColor;

	// ==================== Internal Functions ====================

	/** Cache component references */
	void CacheComponents();

	/** Bind to combat events */
	void BindCombatEvents();

	/** Unbind from combat events */
	void UnbindCombatEvents();

	/** Update hitstop state */
	void UpdateHitstop(float DeltaTime);

	/** Update slow motion state */
	void UpdateSlowMotion(float DeltaTime);

	/** Update dynamic FOV */
	void UpdateDynamicFOV(float DeltaTime);

	/** Update low health effects */
	void UpdateLowHealthEffects(float DeltaTime);

	/** Update screen flash */
	void UpdateScreenFlash(float DeltaTime);

	/** End hitstop and restore time */
	void EndHitstop();

	/** End slow motion and restore time */
	void EndSlowMotion();

	/** Apply motion blur to post process */
	void SetMotionBlur(float Amount);

	/** Apply chromatic aberration to post process */
	void SetChromaticAberration(float Amount);

	/** Apply vignette to post process */
	void SetVignette(float Intensity);

	/** Get hitstop duration for intensity */
	float GetHitstopDuration(ECombatFeedbackIntensity Intensity) const;

	/** Get camera shake class for intensity */
	TSubclassOf<UCameraShakeBase> GetShakeClassForIntensity(ECombatFeedbackIntensity Intensity) const;

	// ==================== Event Handlers ====================

	/** Called when melee trace hits something */
	UFUNCTION()
	void OnMeleeHitCallback(const FMeleeHitResult& HitResult);

	/** Called when player takes damage */
	UFUNCTION()
	void OnDamageReceivedCallback(float Damage, AActor* DamageCauser, AController* InstigatorController);

	/** Called when parry succeeds */
	UFUNCTION()
	void OnParrySuccessCallback(AActor* ParriedActor);

	/** Called when combat state changes */
	UFUNCTION()
	void OnCombatStateChangedCallback(ECombatState NewState, ECombatState OldState);

	/** Called when health changes */
	UFUNCTION()
	void OnHealthChangedCallback(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	/** Called when an actor dies (to detect player kills) */
	UFUNCTION()
	void OnTargetDeathCallback(AActor* KilledBy, AController* InstigatorController);

	/** Timer handles */
	FTimerHandle HitstopTimerHandle;
	FTimerHandle SlowMotionTimerHandle;
	FTimerHandle FlashTimerHandle;
};
