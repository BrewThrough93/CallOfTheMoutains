// CallOfTheMoutains - Combat Feedback Component Implementation

#include "CombatFeedbackComponent.h"
#include "DystopianPostProcess.h"
#include "EquipmentComponent.h"
#include "HealthComponent.h"
#include "MeleeTraceComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

UCombatFeedbackComponent::UCombatFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCombatFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheComponents();
	BindCombatEvents();

	// Store base FOV
	if (CachedCamera)
	{
		BaseFOV = CachedCamera->FieldOfView;
		TargetFOV = BaseFOV;
	}
}

void UCombatFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindCombatEvents();

	// Restore time dilation if we're ending mid-effect
	if (bInHitstop || bInSlowMotion)
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	}

	Super::EndPlay(EndPlayReason);
}

void UCombatFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Use unscaled delta time for effect timers (so they work during slow-mo)
	float UnscaledDeltaTime = DeltaTime;
	if (UWorld* World = GetWorld())
	{
		float TimeDilation = World->GetWorldSettings()->TimeDilation;
		if (TimeDilation > 0.0f)
		{
			UnscaledDeltaTime = DeltaTime / TimeDilation;
		}
	}

	UpdateHitstop(UnscaledDeltaTime);
	UpdateSlowMotion(UnscaledDeltaTime);
	UpdateDynamicFOV(DeltaTime);
	UpdateLowHealthEffects(DeltaTime);
	UpdateScreenFlash(UnscaledDeltaTime);
}

void UCombatFeedbackComponent::CacheComponents()
{
	CachedPlayerController = Cast<APlayerController>(GetOwner());
	if (!CachedPlayerController)
	{
		// Try to get from pawn owner
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			CachedPlayerController = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (CachedPlayerController)
	{
		// Get post process component
		CachedPostProcess = CachedPlayerController->FindComponentByClass<UDystopianPostProcess>();

		// Get equipment component
		CachedEquipmentComponent = CachedPlayerController->FindComponentByClass<UEquipmentComponent>();

		// Get pawn components
		if (APawn* Pawn = CachedPlayerController->GetPawn())
		{
			CachedHealthComponent = Pawn->FindComponentByClass<UHealthComponent>();
			CachedMeleeTraceComponent = Pawn->FindComponentByClass<UMeleeTraceComponent>();
			CachedCamera = Pawn->FindComponentByClass<UCameraComponent>();
			CachedCameraBoom = Pawn->FindComponentByClass<USpringArmComponent>();
		}
	}
	else if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		// We're on the pawn directly
		CachedHealthComponent = Pawn->FindComponentByClass<UHealthComponent>();
		CachedMeleeTraceComponent = Pawn->FindComponentByClass<UMeleeTraceComponent>();
		CachedCamera = Pawn->FindComponentByClass<UCameraComponent>();
		CachedCameraBoom = Pawn->FindComponentByClass<USpringArmComponent>();

		if (AController* Controller = Pawn->GetController())
		{
			CachedPlayerController = Cast<APlayerController>(Controller);
			CachedPostProcess = Controller->FindComponentByClass<UDystopianPostProcess>();
			CachedEquipmentComponent = Controller->FindComponentByClass<UEquipmentComponent>();
		}
	}
}

void UCombatFeedbackComponent::BindCombatEvents()
{
	// Bind to melee trace hits
	if (CachedMeleeTraceComponent)
	{
		CachedMeleeTraceComponent->OnMeleeHit.AddDynamic(this, &UCombatFeedbackComponent::OnMeleeHitCallback);
	}

	// Bind to health damage events
	if (CachedHealthComponent)
	{
		CachedHealthComponent->OnDamageReceived.AddDynamic(this, &UCombatFeedbackComponent::OnDamageReceivedCallback);
		CachedHealthComponent->OnHealthChanged.AddDynamic(this, &UCombatFeedbackComponent::OnHealthChangedCallback);
	}

	// Bind to equipment combat events
	if (CachedEquipmentComponent)
	{
		CachedEquipmentComponent->OnParrySuccess.AddDynamic(this, &UCombatFeedbackComponent::OnParrySuccessCallback);
		CachedEquipmentComponent->OnCombatStateChanged.AddDynamic(this, &UCombatFeedbackComponent::OnCombatStateChangedCallback);
	}
}

void UCombatFeedbackComponent::UnbindCombatEvents()
{
	if (CachedMeleeTraceComponent)
	{
		CachedMeleeTraceComponent->OnMeleeHit.RemoveDynamic(this, &UCombatFeedbackComponent::OnMeleeHitCallback);
	}

	if (CachedHealthComponent)
	{
		CachedHealthComponent->OnDamageReceived.RemoveDynamic(this, &UCombatFeedbackComponent::OnDamageReceivedCallback);
		CachedHealthComponent->OnHealthChanged.RemoveDynamic(this, &UCombatFeedbackComponent::OnHealthChangedCallback);
	}

	if (CachedEquipmentComponent)
	{
		CachedEquipmentComponent->OnParrySuccess.RemoveDynamic(this, &UCombatFeedbackComponent::OnParrySuccessCallback);
		CachedEquipmentComponent->OnCombatStateChanged.RemoveDynamic(this, &UCombatFeedbackComponent::OnCombatStateChangedCallback);
	}
}

// ==================== Public Functions ====================

void UCombatFeedbackComponent::OnDealHit(const FVector& HitLocation, ECombatFeedbackIntensity Intensity)
{
	// Camera shake
	if (CameraShakeConfig.bEnabled && CameraShakeConfig.bShakeOnDealDamage)
	{
		TSubclassOf<UCameraShakeBase> ShakeClass = GetShakeClassForIntensity(Intensity);
		if (ShakeClass)
		{
			PlayCameraShake(ShakeClass, CameraShakeConfig.IntensityMultiplier);
		}
	}

	// Hitstop
	if (HitstopConfig.bEnabled)
	{
		PlayHitstop(Intensity);
	}

	// Screen flash
	if (ScreenEffectsConfig.bFlashOnHit)
	{
		PlayScreenFlash(ScreenEffectsConfig.DealDamageFlashColor, ScreenEffectsConfig.FlashDuration);
	}

	// Chromatic aberration spike for heavy+ hits
	if (ScreenEffectsConfig.bChromaticAberrationSpike && Intensity >= ECombatFeedbackIntensity::Heavy)
	{
		float Amount = ScreenEffectsConfig.ChromaticAberrationMax;
		if (Intensity == ECombatFeedbackIntensity::Heavy)
		{
			Amount *= 0.7f;
		}
		SetChromaticAberration(Amount);

		// Fade out chromatic aberration
		FTimerHandle ChromaticFadeHandle;
		GetWorld()->GetTimerManager().SetTimer(ChromaticFadeHandle, [this]()
		{
			SetChromaticAberration(0.1f); // Return to default
		}, 0.15f, false);
	}

	// Spawn impact VFX
	if (ImpactVFXConfig.bEnabled)
	{
		SpawnImpactVFX(HitLocation, FVector::UpVector, true);
	}
}

void UCombatFeedbackComponent::OnReceiveDamage(float DamageAmount, AActor* DamageCauser)
{
	// Camera shake
	if (CameraShakeConfig.bEnabled && CameraShakeConfig.bShakeOnReceiveDamage && DamageTakenShake)
	{
		float Scale = FMath::Clamp(DamageAmount / 50.0f, 0.5f, 2.0f);
		PlayCameraShake(DamageTakenShake, Scale * CameraShakeConfig.IntensityMultiplier);
	}

	// Screen flash (red)
	if (ScreenEffectsConfig.bFlashOnHit)
	{
		PlayScreenFlash(ScreenEffectsConfig.ReceiveDamageFlashColor, ScreenEffectsConfig.FlashDuration * 1.5f);
	}

	// Post process pulse
	if (CachedPostProcess)
	{
		float Intensity = FMath::Clamp(DamageAmount / 100.0f, 0.3f, 1.0f);
		CachedPostProcess->PulseEffect(Intensity, 0.2f);
	}
}

void UCombatFeedbackComponent::OnParrySuccess(AActor* ParriedActor)
{
	// Camera shake
	if (CameraShakeConfig.bEnabled && CameraShakeConfig.bShakeOnParry && ParryShake)
	{
		PlayCameraShake(ParryShake, CameraShakeConfig.IntensityMultiplier);
	}

	// Hitstop for parry (heavy intensity)
	if (HitstopConfig.bEnabled)
	{
		PlayHitstop(ECombatFeedbackIntensity::Heavy);
	}

	// Slow motion for parry
	if (SlowMotionConfig.bSlowMoOnParry)
	{
		PlaySlowMotion(SlowMotionConfig.ParryDuration, SlowMotionConfig.ParryTimeDilation);
	}

	// Spawn parry sparks
	if (ImpactVFXConfig.bEnabled && ImpactVFXConfig.ParrySparkVFX && ParriedActor)
	{
		FVector SparkLocation = ParriedActor->GetActorLocation();
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			// Spawn between the two actors
			SparkLocation = (OwnerPawn->GetActorLocation() + ParriedActor->GetActorLocation()) * 0.5f;
			SparkLocation.Z += 50.0f; // Adjust height to chest level
		}

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactVFXConfig.ParrySparkVFX,
			SparkLocation,
			FRotator::ZeroRotator,
			FVector(ImpactVFXConfig.VFXScaleMultiplier),
			true
		);
	}
}

void UCombatFeedbackComponent::OnRiposte(AActor* Target)
{
	// Slow motion for riposte
	if (SlowMotionConfig.bSlowMoOnRiposte)
	{
		PlaySlowMotion(SlowMotionConfig.RiposteDuration, SlowMotionConfig.RiposteTimeDilation);
	}

	// Devastating hit effects
	OnDealHit(Target ? Target->GetActorLocation() : FVector::ZeroVector, ECombatFeedbackIntensity::Devastating);
}

void UCombatFeedbackComponent::OnKill(AActor* KilledActor)
{
	// Slow motion for kill
	if (SlowMotionConfig.bSlowMoOnKill)
	{
		PlaySlowMotion(SlowMotionConfig.KillDuration, SlowMotionConfig.KillTimeDilation);
	}

	// Heavy hit feedback
	if (KilledActor)
	{
		OnDealHit(KilledActor->GetActorLocation(), ECombatFeedbackIntensity::Heavy);
	}
}

void UCombatFeedbackComponent::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass || !CachedPlayerController)
	{
		return;
	}

	CachedPlayerController->ClientStartCameraShake(ShakeClass, Scale);
}

void UCombatFeedbackComponent::PlayHitstop(ECombatFeedbackIntensity Intensity)
{
	if (!HitstopConfig.bEnabled)
	{
		return;
	}

	// Don't stack hitstops
	if (bInHitstop)
	{
		return;
	}

	float Duration = GetHitstopDuration(Intensity);
	if (Duration <= 0.0f)
	{
		return;
	}

	bInHitstop = true;
	HitstopTimer = Duration;
	OriginalTimeDilation = GetWorld()->GetWorldSettings()->TimeDilation;

	// Apply hitstop
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), HitstopConfig.TimeDilation);
}

void UCombatFeedbackComponent::PlayScreenFlash(FLinearColor FlashColor, float Duration)
{
	CurrentFlashColor = FlashColor;
	ScreenFlashTimer = Duration;
	ScreenFlashDuration = Duration;

	// Use post process pulse for flash effect
	if (CachedPostProcess)
	{
		CachedPostProcess->PulseEffect(FlashColor.A, Duration);
	}
}

void UCombatFeedbackComponent::PlaySlowMotion(float Duration, float TimeDilation)
{
	// Don't interrupt hitstop
	if (bInHitstop)
	{
		// Queue the slow-mo to start after hitstop
		FTimerHandle DelayedSlowMoHandle;
		GetWorld()->GetTimerManager().SetTimer(DelayedSlowMoHandle, [this, Duration, TimeDilation]()
		{
			PlaySlowMotion(Duration, TimeDilation);
		}, HitstopTimer + 0.01f, false);
		return;
	}

	// Don't extend slow-mo if already in it (unless this is longer)
	if (bInSlowMotion && SlowMotionTimer > Duration)
	{
		return;
	}

	bInSlowMotion = true;
	SlowMotionTimer = Duration;
	SlowMotionDuration = Duration;
	OriginalTimeDilation = 1.0f;

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TimeDilation);
}

void UCombatFeedbackComponent::SpawnImpactVFX(const FVector& Location, const FVector& Normal, bool bIsFlesh)
{
	if (!ImpactVFXConfig.bEnabled)
	{
		return;
	}

	UNiagaraSystem* VFXToSpawn = nullptr;

	if (bIsFlesh && ImpactVFXConfig.FleshImpactVFX)
	{
		VFXToSpawn = ImpactVFXConfig.FleshImpactVFX;
	}
	else if (!bIsFlesh && ImpactVFXConfig.MetalImpactVFX)
	{
		VFXToSpawn = ImpactVFXConfig.MetalImpactVFX;
	}
	else if (ImpactVFXConfig.DefaultImpactVFX)
	{
		VFXToSpawn = ImpactVFXConfig.DefaultImpactVFX;
	}

	if (VFXToSpawn)
	{
		FRotator VFXRotation = Normal.Rotation();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			VFXToSpawn,
			Location,
			VFXRotation,
			FVector(ImpactVFXConfig.VFXScaleMultiplier),
			true
		);
	}
}

void UCombatFeedbackComponent::OnAttackStart(bool bHeavyAttack)
{
	bIsAttacking = true;
	bIsHeavyAttack = bHeavyAttack;

	// Motion blur
	if (DynamicCameraConfig.bMotionBlurOnAttack)
	{
		SetMotionBlur(DynamicCameraConfig.AttackMotionBlurAmount);
	}

	// FOV increase
	if (DynamicCameraConfig.bDynamicFOV && CachedCamera)
	{
		TargetFOV = BaseFOV + DynamicCameraConfig.AttackFOVIncrease;
	}
}

void UCombatFeedbackComponent::OnAttackEnd()
{
	bIsAttacking = false;
	bIsHeavyAttack = false;

	// Restore motion blur
	if (DynamicCameraConfig.bMotionBlurOnAttack)
	{
		SetMotionBlur(0.0f);
	}

	// Restore FOV
	if (DynamicCameraConfig.bDynamicFOV)
	{
		TargetFOV = BaseFOV;
	}
}

void UCombatFeedbackComponent::SetLowHealthState(bool bLowHealth)
{
	bIsLowHealth = bLowHealth;

	if (!bIsLowHealth)
	{
		// Reset vignette when health recovers
		SetVignette(0.5f); // Return to default
		LowHealthPulseTimer = 0.0f;
	}
}

// ==================== Update Functions ====================

void UCombatFeedbackComponent::UpdateHitstop(float DeltaTime)
{
	if (!bInHitstop)
	{
		return;
	}

	HitstopTimer -= DeltaTime;
	if (HitstopTimer <= 0.0f)
	{
		EndHitstop();
	}
}

void UCombatFeedbackComponent::UpdateSlowMotion(float DeltaTime)
{
	if (!bInSlowMotion)
	{
		return;
	}

	SlowMotionTimer -= DeltaTime;
	if (SlowMotionTimer <= 0.0f)
	{
		EndSlowMotion();
	}
}

void UCombatFeedbackComponent::UpdateDynamicFOV(float DeltaTime)
{
	if (!DynamicCameraConfig.bDynamicFOV || !CachedCamera)
	{
		return;
	}

	float CurrentFOV = CachedCamera->FieldOfView;
	if (!FMath::IsNearlyEqual(CurrentFOV, TargetFOV, 0.1f))
	{
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, DynamicCameraConfig.FOVChangeSpeed);
		CachedCamera->SetFieldOfView(NewFOV);
	}
}

void UCombatFeedbackComponent::UpdateLowHealthEffects(float DeltaTime)
{
	if (!ScreenEffectsConfig.bLowHealthVignette || !bIsLowHealth)
	{
		return;
	}

	// Pulsing vignette effect
	LowHealthPulseTimer += DeltaTime * 3.0f; // Pulse speed
	float PulseValue = (FMath::Sin(LowHealthPulseTimer) + 1.0f) * 0.5f; // 0-1 range
	float VignetteIntensity = FMath::Lerp(0.5f, 0.8f, PulseValue);

	SetVignette(VignetteIntensity);
}

void UCombatFeedbackComponent::UpdateScreenFlash(float DeltaTime)
{
	if (ScreenFlashTimer <= 0.0f)
	{
		return;
	}

	ScreenFlashTimer -= DeltaTime;
	if (ScreenFlashTimer <= 0.0f)
	{
		ScreenFlashTimer = 0.0f;
	}
}

void UCombatFeedbackComponent::EndHitstop()
{
	bInHitstop = false;
	HitstopTimer = 0.0f;

	// Restore time dilation (might be going into slow-mo)
	if (!bInSlowMotion)
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), OriginalTimeDilation);
	}
}

void UCombatFeedbackComponent::EndSlowMotion()
{
	bInSlowMotion = false;
	SlowMotionTimer = 0.0f;

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

void UCombatFeedbackComponent::SetMotionBlur(float Amount)
{
	// Motion blur is typically set via post process settings
	// The DystopianPostProcess doesn't have motion blur, so this would need
	// to be added or use a separate post process volume
	// For now, this is a placeholder for future implementation
}

void UCombatFeedbackComponent::SetChromaticAberration(float Amount)
{
	if (CachedPostProcess)
	{
		// Modify the settings directly (this will need the post process to expose this)
		FDystopianSettings ModifiedSettings = CachedPostProcess->Settings;
		ModifiedSettings.ChromaticAberration = Amount;
		CachedPostProcess->BlendToSettings(ModifiedSettings, 0.05f);
	}
}

void UCombatFeedbackComponent::SetVignette(float Intensity)
{
	if (CachedPostProcess)
	{
		FDystopianSettings ModifiedSettings = CachedPostProcess->Settings;
		ModifiedSettings.VignetteIntensity = Intensity;
		CachedPostProcess->BlendToSettings(ModifiedSettings, 0.1f);
	}
}

float UCombatFeedbackComponent::GetHitstopDuration(ECombatFeedbackIntensity Intensity) const
{
	switch (Intensity)
	{
	case ECombatFeedbackIntensity::Light:
		return HitstopConfig.LightHitDuration;
	case ECombatFeedbackIntensity::Medium:
		return HitstopConfig.MediumHitDuration;
	case ECombatFeedbackIntensity::Heavy:
		return HitstopConfig.HeavyHitDuration;
	case ECombatFeedbackIntensity::Devastating:
		return HitstopConfig.DevastatingHitDuration;
	default:
		return HitstopConfig.LightHitDuration;
	}
}

TSubclassOf<UCameraShakeBase> UCombatFeedbackComponent::GetShakeClassForIntensity(ECombatFeedbackIntensity Intensity) const
{
	switch (Intensity)
	{
	case ECombatFeedbackIntensity::Light:
		return LightHitShake;
	case ECombatFeedbackIntensity::Medium:
		return MediumHitShake;
	case ECombatFeedbackIntensity::Heavy:
		return HeavyHitShake;
	case ECombatFeedbackIntensity::Devastating:
		return DevastatingHitShake;
	default:
		return LightHitShake;
	}
}

// ==================== Event Callbacks ====================

void UCombatFeedbackComponent::OnMeleeHitCallback(const FMeleeHitResult& HitResult)
{
	if (!HitResult.bHit)
	{
		return;
	}

	// Determine intensity based on damage
	ECombatFeedbackIntensity Intensity = ECombatFeedbackIntensity::Light;
	if (HitResult.AppliedDamage >= 50.0f)
	{
		Intensity = ECombatFeedbackIntensity::Heavy;
	}
	else if (HitResult.AppliedDamage >= 25.0f)
	{
		Intensity = ECombatFeedbackIntensity::Medium;
	}

	// Check if this was a heavy attack
	if (bIsHeavyAttack)
	{
		// Upgrade intensity for heavy attacks
		if (Intensity == ECombatFeedbackIntensity::Light)
		{
			Intensity = ECombatFeedbackIntensity::Medium;
		}
		else if (Intensity == ECombatFeedbackIntensity::Medium)
		{
			Intensity = ECombatFeedbackIntensity::Heavy;
		}
	}

	OnDealHit(HitResult.HitLocation, Intensity);

	// Check if we killed the target
	if (HitResult.HitActor)
	{
		if (UHealthComponent* TargetHealth = HitResult.HitActor->FindComponentByClass<UHealthComponent>())
		{
			if (TargetHealth->IsDead())
			{
				OnKill(HitResult.HitActor);
			}
		}
	}
}

void UCombatFeedbackComponent::OnDamageReceivedCallback(float Damage, AActor* DamageCauser, AController* InstigatorController)
{
	OnReceiveDamage(Damage, DamageCauser);
}

void UCombatFeedbackComponent::OnParrySuccessCallback(AActor* ParriedActor)
{
	OnParrySuccess(ParriedActor);
}

void UCombatFeedbackComponent::OnCombatStateChangedCallback(ECombatState NewState, ECombatState OldState)
{
	// Track attacking state for motion blur and FOV
	bool bNowAttacking = (NewState == ECombatState::Attacking);
	bool bWasAttacking = (OldState == ECombatState::Attacking);

	if (bNowAttacking && !bWasAttacking)
	{
		// Determine if it's a heavy attack based on equipment component state
		bool bHeavy = false;
		if (CachedEquipmentComponent)
		{
			bHeavy = CachedEquipmentComponent->GetHeavyComboIndex() > 0;
		}
		OnAttackStart(bHeavy);
	}
	else if (!bNowAttacking && bWasAttacking)
	{
		OnAttackEnd();
	}

	// Handle riposte state
	if (NewState == ECombatState::Riposting)
	{
		if (CachedEquipmentComponent)
		{
			OnRiposte(CachedEquipmentComponent->ParriedTarget);
		}
	}
}

void UCombatFeedbackComponent::OnHealthChangedCallback(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	if (MaxHealth <= 0.0f)
	{
		return;
	}

	float HealthPercent = CurrentHealth / MaxHealth;
	bool bNewLowHealthState = HealthPercent <= ScreenEffectsConfig.LowHealthThreshold;

	if (bNewLowHealthState != bIsLowHealth)
	{
		SetLowHealthState(bNewLowHealthState);
	}
}

void UCombatFeedbackComponent::OnTargetDeathCallback(AActor* KilledBy, AController* InstigatorController)
{
	// Check if we were the killer
	if (CachedPlayerController && InstigatorController == CachedPlayerController)
	{
		// Get the actor that died from the killed by parameter
		// This callback would need to be bound to enemy health components
		// For now, kill detection is handled in OnMeleeHitCallback
	}
}
