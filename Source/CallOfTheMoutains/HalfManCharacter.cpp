// CallOfTheMoutains - Half Man Enemy Character Implementation

#include "HalfManCharacter.h"
#include "HealthComponent.h"
#include "FootstepComponent.h"
#include "MeleeTraceComponent.h"
#include "TargetableComponent.h"
#include "GoreTrailComponent.h"
#include "BileProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

AHalfManCharacter::AHalfManCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create health component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// Create footstep component
	FootstepComponent = CreateDefaultSubobject<UFootstepComponent>(TEXT("FootstepComponent"));

	// Create melee trace component
	MeleeTraceComponent = CreateDefaultSubobject<UMeleeTraceComponent>(TEXT("MeleeTraceComponent"));

	// Create targetable component
	TargetableComponent = CreateDefaultSubobject<UTargetableComponent>(TEXT("TargetableComponent"));

	// Create gore trail component
	GoreTrailComponent = CreateDefaultSubobject<UGoreTrailComponent>(TEXT("GoreTrailComponent"));

	// Create wake trigger sphere
	WakeTriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("WakeTriggerSphere"));
	WakeTriggerSphere->SetupAttachment(RootComponent);
	WakeTriggerSphere->SetSphereRadius(WakeRange);
	WakeTriggerSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	WakeTriggerSphere->SetGenerateOverlapEvents(true);
}

void AHalfManCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Update wake trigger radius
	if (WakeTriggerSphere)
	{
		WakeTriggerSphere->SetSphereRadius(WakeRange);
		WakeTriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AHalfManCharacter::OnWakeTriggerOverlap);
	}

	// Bind health component events
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &AHalfManCharacter::OnHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &AHalfManCharacter::OnDeath);
	}

	// Configure targetable component - start not targetable (fake dead)
	if (TargetableComponent)
	{
		TargetableComponent->SetTargetable(false);
	}

	// Configure melee trace component
	if (MeleeTraceComponent)
	{
		MeleeTraceComponent->BaseDamage = MeleeDamage;
	}

	// Start in fake dead state
	SetState(EHalfManState::FakeDead);
}

void AHalfManCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update cooldown timers
	if (MeleeCooldownTimer > 0.0f)
	{
		MeleeCooldownTimer -= DeltaTime;
	}
	if (RangedCooldownTimer > 0.0f)
	{
		RangedCooldownTimer -= DeltaTime;
	}

	// Update state
	switch (CurrentState)
	{
		case EHalfManState::FakeDead:
			UpdateFakeDead(DeltaTime);
			break;
		case EHalfManState::Awakening:
			UpdateAwakening(DeltaTime);
			break;
		case EHalfManState::Idle:
			UpdateIdle(DeltaTime);
			break;
		case EHalfManState::Chasing:
			UpdateChasing(DeltaTime);
			break;
		case EHalfManState::MeleeAttack:
			UpdateMeleeAttack(DeltaTime);
			break;
		case EHalfManState::RangedAttack:
			UpdateRangedAttack(DeltaTime);
			break;
		case EHalfManState::Staggered:
			UpdateStaggered(DeltaTime);
			break;
		case EHalfManState::Dead:
			// Do nothing when dead
			break;
	}
}

// ==================== State Updates ====================

void AHalfManCharacter::UpdateFakeDead(float DeltaTime)
{
	// Just wait - the wake trigger overlap handles awakening
	// Could also check for damage here to wake up
}

void AHalfManCharacter::UpdateAwakening(float DeltaTime)
{
	AwakeningTimer -= DeltaTime;

	// Start facing target during awakening for smoother transition
	FaceTarget();

	if (AwakeningTimer <= 0.0f)
	{
		// Set grace period so we don't immediately lose target due to sight check
		PostAwakeningGraceTimer = 2.0f;

		// Awakening complete, start chasing
		SetState(EHalfManState::Chasing);
	}
}

void AHalfManCharacter::UpdateIdle(float DeltaTime)
{
	// Look for player
	LookForTarget();

	if (CurrentTarget)
	{
		SetState(EHalfManState::Chasing);
	}
}

void AHalfManCharacter::UpdateChasing(float DeltaTime)
{
	// Check if we still have a target
	if (!CurrentTarget)
	{
		LookForTarget();
		if (!CurrentTarget)
		{
			SetState(EHalfManState::Idle);
			return;
		}
	}

	// Grace period after awakening - skip sight check
	if (PostAwakeningGraceTimer > 0.0f)
	{
		PostAwakeningGraceTimer -= DeltaTime;
	}
	else
	{
		// Check if target is still visible
		if (!CanSeeTarget(CurrentTarget))
		{
			CurrentTarget = nullptr;
			SetState(EHalfManState::Idle);
			return;
		}
	}

	float DistanceToTarget = GetDistanceToTarget();

	// Always face target while chasing
	FaceTarget();

	// RANGED ATTACK - Check first if in optimal ranged distance and cooldown ready
	// Prefer ranged when not in melee range
	if (DistanceToTarget > MeleeRange && DistanceToTarget <= RangedRange && DistanceToTarget >= RangedMinRange)
	{
		if (RangedCooldownTimer <= 0.0f)
		{
			// Roll once when cooldown is ready
			if (FMath::FRand() < RangedAttackChance)
			{
				TryRangedAttack();
				return;
			}
			else
			{
				// Failed the roll, set a short cooldown so we don't spam roll every frame
				RangedCooldownTimer = 1.0f;
			}
		}
	}

	// MELEE ATTACK - In melee range
	if (DistanceToTarget <= MeleeRange)
	{
		if (MeleeCooldownTimer <= 0.0f)
		{
			TryMeleeAttack();
			return;
		}
	}

	// Move towards target
	FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	Direction.Z = 0.0f;

	AddMovementInput(Direction, 1.0f);
}

void AHalfManCharacter::UpdateMeleeAttack(float DeltaTime)
{
	// Attack is handled by timers, just face target
	FaceTarget();
}

void AHalfManCharacter::UpdateRangedAttack(float DeltaTime)
{
	// Attack is handled by timers, just face target
	FaceTarget();
}

void AHalfManCharacter::UpdateStaggered(float DeltaTime)
{
	StaggerTimer -= DeltaTime;

	if (StaggerTimer <= 0.0f)
	{
		// Return to chasing if we have a target
		if (CurrentTarget)
		{
			SetState(EHalfManState::Chasing);
		}
		else
		{
			SetState(EHalfManState::Idle);
		}
	}
}

// ==================== State Transitions ====================

void AHalfManCharacter::SetState(EHalfManState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	EHalfManState OldState = CurrentState;
	OnStateExit(OldState);

	CurrentState = NewState;
	OnStateEnter(NewState);
}

void AHalfManCharacter::OnStateEnter(EHalfManState NewState)
{
	switch (NewState)
	{
		case EHalfManState::FakeDead:
			// Disable targeting
			if (TargetableComponent)
			{
				TargetableComponent->SetTargetable(false);
			}
			// Disable gore trail
			if (GoreTrailComponent)
			{
				GoreTrailComponent->SetTrailActive(false);
			}
			break;

		case EHalfManState::Awakening:
			bHasAwakened = true;
			PlayAwakeningEffects();

			// Get awakening duration from montage
			if (AwakeningMontage)
			{
				AwakeningTimer = AwakeningMontage->GetPlayLength();
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_Play(AwakeningMontage);
				}
			}
			else
			{
				AwakeningTimer = 2.0f; // Default duration
			}
			break;

		case EHalfManState::Idle:
		case EHalfManState::Chasing:
			// Enable targeting now that we're active
			if (TargetableComponent)
			{
				TargetableComponent->SetTargetable(true);
			}
			// Enable gore trail
			if (GoreTrailComponent)
			{
				GoreTrailComponent->SetTrailActive(true);
			}
			// Look for target
			LookForTarget();
			break;

		case EHalfManState::MeleeAttack:
			bIsAttacking = true;
			break;

		case EHalfManState::RangedAttack:
			bIsAttacking = true;
			break;

		case EHalfManState::Staggered:
			StaggerTimer = StaggerDuration;
			bIsAttacking = false;

			// Play hit reaction
			if (HitReactionMontage)
			{
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_Play(HitReactionMontage);
				}
			}

			// Play hit sound
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
			}
			break;

		case EHalfManState::Dead:
			bIsDead = true;
			bIsAttacking = false;

			// Disable targeting
			if (TargetableComponent)
			{
				TargetableComponent->SetTargetable(false);
			}

			// Disable gore trail
			if (GoreTrailComponent)
			{
				GoreTrailComponent->SetTrailActive(false);
			}

			// Play death animation
			if (DeathMontage)
			{
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_Play(DeathMontage);
				}
			}

			// Play death sound
			if (DeathSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
			}

			// Disable collision
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// Stop movement
			GetCharacterMovement()->StopMovementImmediately();

			// Cleanup after a delay
			SetLifeSpan(10.0f);
			break;
	}
}

void AHalfManCharacter::OnStateExit(EHalfManState OldState)
{
	switch (OldState)
	{
		case EHalfManState::FakeDead:
			// Disable wake trigger - no longer needed
			if (WakeTriggerSphere)
			{
				WakeTriggerSphere->SetGenerateOverlapEvents(false);
			}
			break;

		case EHalfManState::MeleeAttack:
		case EHalfManState::RangedAttack:
			bIsAttacking = false;
			// Clear any pending attack timers
			GetWorld()->GetTimerManager().ClearTimer(MeleeHitTimerHandle);
			GetWorld()->GetTimerManager().ClearTimer(AttackEndTimerHandle);
			break;

		default:
			break;
	}
}

// ==================== Detection ====================

bool AHalfManCharacter::CanSeeTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	float Distance = ToTarget.Size();

	// Check distance
	if (Distance > SightRange)
	{
		return false;
	}

	// Check angle
	ToTarget.Normalize();
	float DotProduct = FVector::DotProduct(GetActorForwardVector(), ToTarget);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

	if (Angle > SightAngle * 0.5f)
	{
		return false;
	}

	// Check line of sight
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FVector Start = GetActorLocation() + FVector(0, 0, 50);
	FVector End = Target->GetActorLocation() + FVector(0, 0, 50);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
	{
		return HitResult.GetActor() == Target;
	}

	return true;
}

void AHalfManCharacter::LookForTarget()
{
	APawn* Player = GetPlayerPawn();
	if (Player && CanSeeTarget(Player))
	{
		CurrentTarget = Player;
	}
}

APawn* AHalfManCharacter::GetPlayerPawn() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		return PC->GetPawn();
	}
	return nullptr;
}

float AHalfManCharacter::GetDistanceToTarget() const
{
	if (!CurrentTarget)
	{
		return MAX_FLT;
	}
	return FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
}

// ==================== Combat ====================

void AHalfManCharacter::WakeUp()
{
	if (CurrentState == EHalfManState::FakeDead)
	{
		SetState(EHalfManState::Awakening);
	}
}

void AHalfManCharacter::TryMeleeAttack()
{
	if (bIsAttacking || MeleeCooldownTimer > 0.0f)
	{
		return;
	}

	SetState(EHalfManState::MeleeAttack);
	MeleeCooldownTimer = MeleeAttackCooldown;

	// Play attack sound
	if (MeleeAttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, MeleeAttackSound, GetActorLocation());
	}

	// Play attack montage
	float AttackDuration = 1.0f;
	if (MeleeAttackMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AttackDuration = AnimInstance->Montage_Play(MeleeAttackMontage);
		}
	}

	// Set up melee hit timer (at 40% of animation)
	float HitTime = AttackDuration * 0.4f;
	GetWorld()->GetTimerManager().SetTimer(MeleeHitTimerHandle, this, &AHalfManCharacter::OnMeleeAttackHit, HitTime, false);

	// Set up attack end timer
	GetWorld()->GetTimerManager().SetTimer(AttackEndTimerHandle, this, &AHalfManCharacter::OnMeleeAttackEnd, AttackDuration, false);
}

void AHalfManCharacter::TryRangedAttack()
{
	if (bIsAttacking || RangedCooldownTimer > 0.0f)
	{
		return;
	}

	SetState(EHalfManState::RangedAttack);
	RangedCooldownTimer = RangedAttackCooldown;

	// Play attack sound
	if (RangedAttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RangedAttackSound, GetActorLocation());
	}

	// Play attack montage
	float AttackDuration = 1.5f;
	if (RangedAttackMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AttackDuration = AnimInstance->Montage_Play(RangedAttackMontage);
		}
	}

	// Spawn projectile at 60% of animation (can also be done via anim notify)
	float SpawnTime = AttackDuration * 0.6f;
	FTimerHandle SpawnTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AHalfManCharacter::SpawnBileProjectile, SpawnTime, false);

	// Set up attack end timer
	GetWorld()->GetTimerManager().SetTimer(AttackEndTimerHandle, this, &AHalfManCharacter::OnRangedAttackEnd, AttackDuration, false);
}

void AHalfManCharacter::SpawnBileProjectile()
{
	if (!BileProjectileClass || !CurrentTarget)
	{
		return;
	}

	// Get spawn location from socket
	FVector SpawnLocation = GetActorLocation() + FVector(0, 0, 100); // Default
	if (GetMesh()->DoesSocketExist(ProjectileSpawnSocket))
	{
		SpawnLocation = GetMesh()->GetSocketLocation(ProjectileSpawnSocket);
	}

	// Calculate direction to target
	FVector Direction = (CurrentTarget->GetActorLocation() - SpawnLocation).GetSafeNormal();

	// Spawn projectile
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	ABileProjectile* Projectile = GetWorld()->SpawnActor<ABileProjectile>(
		BileProjectileClass,
		SpawnLocation,
		Direction.Rotation(),
		SpawnParams
	);

	if (Projectile)
	{
		Projectile->InitializeProjectile(this, Direction);
	}
}

void AHalfManCharacter::OnMeleeAttackHit()
{
	// Start melee trace
	if (MeleeTraceComponent)
	{
		MeleeTraceComponent->StartTrace();

		// Stop trace after a short duration
		FTimerHandle StopTraceHandle;
		GetWorld()->GetTimerManager().SetTimer(StopTraceHandle, [this]()
		{
			if (MeleeTraceComponent)
			{
				MeleeTraceComponent->StopTrace();
			}
		}, 0.2f, false);
	}
}

void AHalfManCharacter::OnMeleeAttackEnd()
{
	if (CurrentState == EHalfManState::MeleeAttack)
	{
		// Return to chasing
		if (CurrentTarget)
		{
			SetState(EHalfManState::Chasing);
		}
		else
		{
			SetState(EHalfManState::Idle);
		}
	}
}

void AHalfManCharacter::OnRangedAttackEnd()
{
	if (CurrentState == EHalfManState::RangedAttack)
	{
		// Return to chasing
		if (CurrentTarget)
		{
			SetState(EHalfManState::Chasing);
		}
		else
		{
			SetState(EHalfManState::Idle);
		}
	}
}

void AHalfManCharacter::FaceTarget()
{
	if (!CurrentTarget)
	{
		return;
	}

	FVector Direction = CurrentTarget->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;

	if (!Direction.IsNearlyZero())
	{
		FRotator TargetRotation = Direction.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 10.0f));
	}
}

// ==================== Health Callbacks ====================

void AHalfManCharacter::OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	// Taking damage wakes us up
	if (CurrentState == EHalfManState::FakeDead && Delta < 0.0f)
	{
		WakeUp();
		return;
	}

	// Stagger on damage (if not already staggered or dead)
	if (Delta < 0.0f && CurrentState != EHalfManState::Staggered && CurrentState != EHalfManState::Dead && CurrentState != EHalfManState::Awakening)
	{
		SetState(EHalfManState::Staggered);
	}
}

void AHalfManCharacter::OnDeath(AActor* KilledBy, AController* InstigatorController)
{
	SetState(EHalfManState::Dead);
}

// ==================== Wake Trigger ====================

void AHalfManCharacter::OnWakeTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only wake for player
	APawn* Player = GetPlayerPawn();
	if (OtherActor == Player && CurrentState == EHalfManState::FakeDead)
	{
		CurrentTarget = Player;
		WakeUp();
	}
}

// ==================== VFX/SFX ====================

void AHalfManCharacter::PlayAwakeningEffects()
{
	// Play awakening sound
	if (AwakeningSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AwakeningSound, GetActorLocation());
	}

	// Spawn gore effect
	if (AwakeningGoreEffect)
	{
		FVector SpawnLocation = GetActorLocation();
		if (GetMesh()->DoesSocketExist(AwakeningEffectSocket))
		{
			SpawnLocation = GetMesh()->GetSocketLocation(AwakeningEffectSocket);
		}
		SpawnGoreEffect(SpawnLocation);
	}
}

void AHalfManCharacter::SpawnGoreEffect(FVector Location)
{
	if (AwakeningGoreEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			AwakeningGoreEffect,
			Location,
			GetActorRotation()
		);
	}
}
