// CallOfTheMoutains - The Forgotten Enemy Character Implementation

#include "ForgottenCharacter.h"
#include "HealthComponent.h"
#include "FootstepComponent.h"
#include "MeleeTraceComponent.h"
#include "TargetableComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"

AForgottenCharacter::AForgottenCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create health component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 100.0f;
	HealthComponent->bShowFloatingHealthBar = true;

	// Create footstep component
	FootstepComponent = CreateDefaultSubobject<UFootstepComponent>(TEXT("FootstepComponent"));

	// Create melee trace component - configured for unarmed combat
	MeleeTraceComponent = CreateDefaultSubobject<UMeleeTraceComponent>(TEXT("MeleeTraceComponent"));
	MeleeTraceComponent->bUseWeaponDamage = false; // Use BaseDamage instead of weapon stats
	MeleeTraceComponent->BaseDamage = AttackDamage;
	// Configure for unarmed attacks using character mesh sockets
	MeleeTraceComponent->MeshSource = EMeleeTraceMeshSource::CharacterMesh;
	MeleeTraceComponent->TraceMode = EMeleeTraceMode::Spherical; // Sphere trace for fist/claw
	MeleeTraceComponent->StartSocket = FName(TEXT("hand_r")); // Right hand socket
	MeleeTraceComponent->TraceRadius = 30.0f; // Larger radius for fist attacks
	MeleeTraceComponent->bDrawDebug = true; // Enable debug to verify it's working

	// Create targetable component for lock-on
	TargetableComponent = CreateDefaultSubobject<UTargetableComponent>(TEXT("TargetableComponent"));
	TargetableComponent->TargetOffset = FVector(0.0f, 0.0f, 60.0f); // Chest height

	// Configure movement for slow zombie
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = PatrolSpeed;
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 180.0f, 0.0f);
	}

	// Don't rotate with controller
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
}

void AForgottenCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Bind health component events
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &AForgottenCharacter::OnTakeDamage);
		HealthComponent->OnDeath.AddDynamic(this, &AForgottenCharacter::OnDeath);
	}

	// Initialize ambient sound timer with some randomness
	AmbientSoundTimer = FMath::RandRange(2.0f, AmbientSoundInterval);

	// Start in idle state
	SetState(EForgottenState::Idle);
}

void AForgottenCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	// Update attack cooldown
	if (AttackCooldownTimer > 0.0f)
	{
		AttackCooldownTimer -= DeltaTime;
	}

	// Update ambient sounds
	AmbientSoundTimer -= DeltaTime;
	if (AmbientSoundTimer <= 0.0f)
	{
		PlayAmbientSound();
		AmbientSoundTimer = AmbientSoundInterval + FMath::RandRange(-2.0f, 2.0f);
	}

	// State machine
	switch (CurrentState)
	{
	case EForgottenState::Idle:
	case EForgottenState::Patrolling:
		UpdateIdle(DeltaTime);
		break;

	case EForgottenState::Chasing:
		UpdateChasing(DeltaTime);
		break;

	case EForgottenState::Attacking:
		UpdateAttacking(DeltaTime);
		break;

	case EForgottenState::Staggered:
		UpdateStaggered(DeltaTime);
		break;

	case EForgottenState::Dead:
		// Do nothing
		break;
	}
}

void AForgottenCharacter::UpdateIdle(float DeltaTime)
{
	// Look for player
	LookForTarget();

	// If we found a target, start chasing
	if (CurrentTarget)
	{
		SetState(EForgottenState::Chasing);

		// Play alert sound
		if (AlertSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, AlertSound, GetActorLocation());
		}
	}
}

void AForgottenCharacter::UpdateChasing(float DeltaTime)
{
	// Check if we still have a target
	if (!CurrentTarget)
	{
		// Use chase memory to go to last known location
		ChaseMemoryTimer -= DeltaTime;
		if (ChaseMemoryTimer <= 0.0f)
		{
			SetState(EForgottenState::Idle);
			return;
		}
	}
	else
	{
		// Update last known location
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		ChaseMemoryTimer = ChaseMemoryDuration;

		// Check if we can still see target
		if (!CanSeeTarget(CurrentTarget))
		{
			// Lost sight, start memory timer
			CurrentTarget = nullptr;
		}
	}

	// Check if in attack range
	float Distance = GetDistanceToTarget();
	if (Distance <= AttackRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("Forgotten: In attack range (%.1f <= %.1f), attempting attack"), Distance, AttackRange);
		TryAttack();
	}
	else
	{
		// Move toward target/last known location
		MoveTowardTarget(DeltaTime);
	}
}

void AForgottenCharacter::UpdateAttacking(float DeltaTime)
{
	// Face the target while attacking
	if (CurrentTarget)
	{
		FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0;
		if (!ToTarget.IsNearlyZero())
		{
			FRotator TargetRotation = ToTarget.Rotation();
			FRotator CurrentRotation = GetActorRotation();
			FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f);
			SetActorRotation(FRotator(0, NewRotation.Yaw, 0));
		}
	}

	// Attack state is managed by animation callbacks (OnAttackEnd)
}

void AForgottenCharacter::UpdateStaggered(float DeltaTime)
{
	StaggerTimer -= DeltaTime;
	if (StaggerTimer <= 0.0f)
	{
		// Return to chasing if we have a target, otherwise idle
		if (CurrentTarget)
		{
			SetState(EForgottenState::Chasing);
		}
		else
		{
			SetState(EForgottenState::Idle);
		}
	}
}

void AForgottenCharacter::LookForTarget()
{
	APawn* Player = GetPlayerPawn();
	if (!Player)
	{
		return;
	}

	// Check sight
	if (CanSeeTarget(Player))
	{
		CurrentTarget = Player;
		LastKnownTargetLocation = Player->GetActorLocation();
		ChaseMemoryTimer = ChaseMemoryDuration;
	}
}

bool AForgottenCharacter::CanSeeTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	FVector MyLocation = GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	float Distance = FVector::Dist(MyLocation, TargetLocation);

	// Check distance
	if (Distance > SightRange)
	{
		return false;
	}

	// Check angle (field of view)
	FVector ToTarget = (TargetLocation - MyLocation).GetSafeNormal();
	FVector Forward = GetActorForwardVector();
	float DotProduct = FVector::DotProduct(Forward, ToTarget);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

	if (Angle > SightAngle * 0.5f)
	{
		return false;
	}

	// Line of sight check
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, MyLocation + FVector(0, 0, 50), TargetLocation + FVector(0, 0, 50), ECC_Visibility, QueryParams))
	{
		// Check if we hit the target or something blocking
		if (HitResult.GetActor() != Target)
		{
			return false;
		}
	}

	return true;
}

bool AForgottenCharacter::IsInAttackRange() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	return GetDistanceToTarget() <= AttackRange;
}

float AForgottenCharacter::GetDistanceToTarget() const
{
	if (!CurrentTarget)
	{
		return FLT_MAX;
	}

	return FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
}

void AForgottenCharacter::MoveTowardTarget(float DeltaTime)
{
	FVector TargetLocation = CurrentTarget ? CurrentTarget->GetActorLocation() : LastKnownTargetLocation;
	FVector MyLocation = GetActorLocation();

	FVector Direction = (TargetLocation - MyLocation).GetSafeNormal2D();

	// Move using AddMovementInput for proper character movement
	AddMovementInput(Direction, 1.0f);

	// Rotate toward target
	if (!Direction.IsNearlyZero())
	{
		FRotator TargetRotation = Direction.Rotation();
		FRotator CurrentRotation = GetActorRotation();
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 3.0f);
		SetActorRotation(FRotator(0, NewRotation.Yaw, 0));
	}
}

void AForgottenCharacter::SetState(EForgottenState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	EForgottenState OldState = CurrentState;
	CurrentState = NewState;

	// Update movement speed based on state
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		switch (NewState)
		{
		case EForgottenState::Idle:
		case EForgottenState::Patrolling:
			Movement->MaxWalkSpeed = PatrolSpeed;
			break;

		case EForgottenState::Chasing:
			Movement->MaxWalkSpeed = ChaseSpeed;
			break;

		case EForgottenState::Attacking:
		case EForgottenState::Staggered:
		case EForgottenState::Dead:
			Movement->MaxWalkSpeed = 0.0f;
			break;
		}
	}
}

void AForgottenCharacter::TryAttack()
{
	// Check cooldown
	if (AttackCooldownTimer > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Forgotten: Attack on cooldown (%.1f remaining)"), AttackCooldownTimer);
		return;
	}

	// Check if already attacking
	if (bIsAttacking)
	{
		UE_LOG(LogTemp, Warning, TEXT("Forgotten: Already attacking, skipping"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Forgotten: Starting attack!"));

	// Start attack
	bIsAttacking = true;
	SetState(EForgottenState::Attacking);
	AttackCooldownTimer = AttackCooldown;

	// Play attack sound
	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}

	// Play attack montage
	if (AttackMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			float MontageLength = AnimInstance->Montage_Play(AttackMontage);
			UE_LOG(LogTemp, Warning, TEXT("Forgotten: Playing attack montage (length: %.2f)"), MontageLength);

			// Set timer for damage at roughly mid-point of animation
			float HitTime = MontageLength * 0.4f;
			GetWorld()->GetTimerManager().SetTimer(
				AttackTimerHandle,
				this,
				&AForgottenCharacter::OnAttackTimerHit,
				HitTime,
				false
			);

			// Set timer for attack end (montage finished)
			FTimerHandle EndTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(
				EndTimerHandle,
				this,
				&AForgottenCharacter::OnAttackTimerEnd,
				MontageLength,
				false
			);
		}
	}
	else
	{
		// No montage - use timer for attack sequence
		// First timer for the hit (wind-up)
		GetWorld()->GetTimerManager().SetTimer(
			AttackTimerHandle,
			this,
			&AForgottenCharacter::OnAttackTimerHit,
			0.3f,
			false
		);
	}
}

void AForgottenCharacter::OnAttackTimerHit()
{
	OnAttackHit();

	// Set timer for attack end
	GetWorld()->GetTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&AForgottenCharacter::OnAttackTimerEnd,
		0.3f,
		false
	);
}

void AForgottenCharacter::OnAttackTimerEnd()
{
	OnAttackEnd();
}

void AForgottenCharacter::OnAttackHit()
{
	UE_LOG(LogTemp, Warning, TEXT("Forgotten: OnAttackHit called - starting melee trace"));

	// Start the melee trace for hit detection
	if (MeleeTraceComponent)
	{
		MeleeTraceComponent->BaseDamage = AttackDamage; // Ensure damage is set
		MeleeTraceComponent->StartTrace();

		// Stop trace after a short window (the swing duration)
		FTimerHandle TraceStopHandle;
		GetWorld()->GetTimerManager().SetTimer(
			TraceStopHandle,
			[this]()
			{
				if (MeleeTraceComponent)
				{
					MeleeTraceComponent->StopTrace();
					UE_LOG(LogTemp, Warning, TEXT("Forgotten: Melee trace stopped"));
				}
			},
			0.2f, // Trace active for 0.2 seconds
			false
		);
	}
}

void AForgottenCharacter::OnAttackEnd()
{
	bIsAttacking = false;

	// Make sure trace is stopped
	if (MeleeTraceComponent && MeleeTraceComponent->IsTracing())
	{
		MeleeTraceComponent->StopTrace();
	}

	// Return to chasing or idle
	if (CurrentTarget && CanSeeTarget(CurrentTarget))
	{
		SetState(EForgottenState::Chasing);
	}
	else
	{
		SetState(EForgottenState::Idle);
	}
}

void AForgottenCharacter::AlertToLocation(FVector Location)
{
	// If we don't have a target, investigate the location
	if (!CurrentTarget && CurrentState == EForgottenState::Idle)
	{
		LastKnownTargetLocation = Location;
		ChaseMemoryTimer = ChaseMemoryDuration;
		SetState(EForgottenState::Chasing);
	}
}

float AForgottenCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("Forgotten::TakeDamage called with %.1f damage from %s"),
		DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("nullptr"));

	// Forward damage to health component
	if (HealthComponent && !HealthComponent->IsDead())
	{
		float ActualDamage = HealthComponent->TakeDamage(DamageAmount, DamageCauser, EventInstigator);
		UE_LOG(LogTemp, Warning, TEXT("Forgotten: HealthComponent processed damage, actual: %.1f, health now: %.1f/%.1f, IsDead: %s"),
			ActualDamage, HealthComponent->GetHealth(), HealthComponent->GetMaxHealth(),
			HealthComponent->IsDead() ? TEXT("true") : TEXT("false"));
		return ActualDamage;
	}

	UE_LOG(LogTemp, Warning, TEXT("Forgotten: No HealthComponent or already dead"));
	return 0.0f;
}

void AForgottenCharacter::OnTakeDamage(float CurrentHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("Forgotten::OnTakeDamage callback - Health: %.1f/%.1f, Delta: %.1f, bIsDead: %s"),
		CurrentHealth, MaxHealth, Delta, bIsDead ? TEXT("true") : TEXT("false"));

	if (bIsDead)
	{
		return;
	}

	// If we don't have a target and got damaged, turn toward damage source
	if (!CurrentTarget && DamageCauser)
	{
		CurrentTarget = DamageCauser;
		LastKnownTargetLocation = DamageCauser->GetActorLocation();
		ChaseMemoryTimer = ChaseMemoryDuration;
	}

	// Enter stagger state (unless already dead)
	if (CurrentHealth > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Forgotten: Entering stagger state, playing hit reaction"));
		SetState(EForgottenState::Staggered);
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
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Forgotten: No HitReactionMontage set!"));
		}
	}
}

void AForgottenCharacter::OnDeath(AActor* KilledBy, AController* InstigatorController)
{
	UE_LOG(LogTemp, Warning, TEXT("Forgotten::OnDeath called! Killed by: %s"),
		KilledBy ? *KilledBy->GetName() : TEXT("nullptr"));

	bIsDead = true;
	SetState(EForgottenState::Dead);
	bIsAttacking = false;

	// Disable lock-on targeting
	if (TargetableComponent)
	{
		TargetableComponent->SetTargetable(false);
	}

	// Play death montage
	if (DeathMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(DeathMontage);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Forgotten: No DeathMontage set!"));
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Stop movement
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	// Destroy after delay (let death animation play)
	SetLifeSpan(10.0f);
}

void AForgottenCharacter::PlayAmbientSound()
{
	if (AmbientSounds.Num() > 0 && CurrentState != EForgottenState::Dead)
	{
		int32 Index = FMath::RandRange(0, AmbientSounds.Num() - 1);
		if (AmbientSounds[Index])
		{
			UGameplayStatics::PlaySoundAtLocation(this, AmbientSounds[Index], GetActorLocation(), FMath::RandRange(0.8f, 1.0f));
		}
	}
}

APawn* AForgottenCharacter::GetPlayerPawn() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	return PC ? PC->GetPawn() : nullptr;
}
