// CallOfTheMoutains - Health Component Implementation

#include "HealthComponent.h"
#include "FloatingHealthBar.h"
#include "TargetableComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize health
	if (StartingHealth > 0.0f)
	{
		CurrentHealth = FMath::Min(StartingHealth, MaxHealth);
	}
	else
	{
		CurrentHealth = MaxHealth;
	}

	// Initialize stamina
	if (StartingStamina > 0.0f)
	{
		CurrentStamina = FMath::Min(StartingStamina, MaxStamina);
	}
	else
	{
		CurrentStamina = MaxStamina;
	}

	// Create floating health bar if enabled (but not for bosses)
	if (bShowFloatingHealthBar && !bIsBoss)
	{
		CreateFloatingHealthBar();

		// Bind to lock-on events if we only show when locked on
		if (bOnlyShowWhenLockedOn)
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				CachedTargetableComponent = Owner->FindComponentByClass<UTargetableComponent>();
				if (CachedTargetableComponent)
				{
					CachedTargetableComponent->OnTargetStateChanged.AddDynamic(this, &UHealthComponent::OnLockOnStateChanged);
				}
			}
		}
	}
}

float UHealthComponent::TakeDamage(float Damage, AActor* DamageCauser, AController* InstigatorController)
{
	// Can't damage if already dead or can't be damaged
	if (bIsDead || !bCanBeDamaged || Damage <= 0.0f)
	{
		return 0.0f;
	}

	// Calculate actual damage
	float ActualDamage = CalculateDamage(Damage);

	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	// Store old health for delta calculation
	float OldHealth = CurrentHealth;

	// Apply damage (but don't go below 0 if invincible)
	if (bInvincible)
	{
		CurrentHealth = FMath::Max(1.0f, CurrentHealth - ActualDamage);
	}
	else
	{
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);
	}

	float Delta = CurrentHealth - OldHealth;

	// Cache the damage causer for death impulse direction
	LastDamageCauser = DamageCauser;

	// Set bIsDead BEFORE broadcasting events so IsDead() returns true during callbacks
	bool bJustDied = false;
	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		bJustDied = true;
	}

	// Broadcast events
	OnDamageReceived.Broadcast(ActualDamage, DamageCauser, InstigatorController);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, Delta, DamageCauser);

	// Update floating health bar
	UpdateFloatingHealthBar();

	// Broadcast death event after all damage events
	if (bJustDied)
	{
		OnDeath.Broadcast(DamageCauser, InstigatorController);
		HandleDeath(DamageCauser, InstigatorController);
	}

	return ActualDamage;
}

float UHealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.0f)
	{
		return 0.0f;
	}

	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
	float ActualHeal = CurrentHealth - OldHealth;

	if (ActualHeal > 0.0f)
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, ActualHeal, nullptr);

		// Update floating health bar
		UpdateFloatingHealthBar();
	}

	return ActualHeal;
}

void UHealthComponent::HealToFull()
{
	Heal(MaxHealth - CurrentHealth);
}

void UHealthComponent::Kill(AActor* Killer, AController* InstigatorController)
{
	if (bIsDead)
	{
		return;
	}

	CurrentHealth = 0.0f;
	bIsDead = true;
	LastDamageCauser = Killer;

	// Broadcast events
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, -MaxHealth, Killer);
	OnDeath.Broadcast(Killer, InstigatorController);

	// Handle physical death effects
	HandleDeath(Killer, InstigatorController);
}

void UHealthComponent::Revive(float HealthAmount)
{
	if (!bIsDead)
	{
		return;
	}

	bIsDead = false;

	if (HealthAmount > 0.0f)
	{
		CurrentHealth = FMath::Min(HealthAmount, MaxHealth);
	}
	else
	{
		CurrentHealth = MaxHealth;
	}

	OnRevive.Broadcast();
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth, nullptr);
}

void UHealthComponent::SetHealth(float NewHealth)
{
	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	// Set bIsDead BEFORE broadcasting events
	bool bJustDied = false;
	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		bJustDied = true;
	}

	if (CurrentHealth != OldHealth)
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - OldHealth, nullptr);
	}

	// Broadcast death event after health changed
	if (bJustDied)
	{
		OnDeath.Broadcast(nullptr, nullptr);
	}
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bScaleCurrentHealth)
{
	if (NewMaxHealth <= 0.0f)
	{
		return;
	}

	float OldMaxHealth = MaxHealth;
	MaxHealth = NewMaxHealth;

	if (bScaleCurrentHealth && OldMaxHealth > 0.0f)
	{
		// Scale current health proportionally
		float HealthPercent = CurrentHealth / OldMaxHealth;
		CurrentHealth = HealthPercent * MaxHealth;
	}
	else
	{
		// Clamp current health to new max
		CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
	}

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.0f, nullptr);
}

float UHealthComponent::GetHealthPercent() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return CurrentHealth / MaxHealth;
}

void UHealthComponent::HandleDeath(AActor* Killer, AController* InstigatorController)
{
	// Note: bIsDead is now set before this is called in TakeDamage/Kill/SetHealth
	// This function handles the physical death effects (ragdoll, AI stop, etc.)

	AActor* Owner = GetOwner();

	// Immediately make this target non-targetable to clear any lock-on
	if (Owner)
	{
		if (UTargetableComponent* Targetable = Owner->FindComponentByClass<UTargetableComponent>())
		{
			Targetable->SetTargetable(false);
		}
	}

	// Stop AI behavior first (before ragdoll so animations stop cleanly)
	if (bStopAIOnDeath)
	{
		StopAIBehavior();
	}

	// Enable ragdoll physics
	if (bRagdollOnDeath)
	{
		EnableRagdoll();
	}

	// Disable collision if configured
	if (bDisableCollisionOnDeath && Owner)
	{
		Owner->SetActorEnableCollision(false);
	}

	// Set up destruction timer if configured
	if (DestroyAfterDeathDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DestroyAfterDeathTimerHandle,
			this,
			&UHealthComponent::DestroyOwnerActor,
			DestroyAfterDeathDelay,
			false
		);
	}
}

float UHealthComponent::CalculateDamage(float RawDamage) const
{
	// Apply defense (flat reduction)
	float DamageAfterDefense = FMath::Max(0.0f, RawDamage - Defense);

	// Apply damage multiplier
	float FinalDamage = DamageAfterDefense * DamageMultiplier;

	return FinalDamage;
}

// ==================== Stamina Functions ====================

bool UHealthComponent::UseStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}

	if (CurrentStamina < Amount)
	{
		return false; // Not enough stamina
	}

	float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);
	float Delta = CurrentStamina - OldStamina;

	// Stop regeneration and start delay timer
	bIsRegeneratingStamina = false;
	GetWorld()->GetTimerManager().ClearTimer(StaminaRegenTimerHandle);

	if (bStaminaRegenEnabled && StaminaRegenDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			StaminaRegenTimerHandle,
			this,
			&UHealthComponent::StartStaminaRegen,
			StaminaRegenDelay,
			false
		);
	}
	else if (bStaminaRegenEnabled)
	{
		bIsRegeneratingStamina = true;
	}

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, Delta);

	return true;
}

float UHealthComponent::RestoreStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return 0.0f;
	}

	float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Min(CurrentStamina + Amount, MaxStamina);
	float ActualRestore = CurrentStamina - OldStamina;

	if (ActualRestore > 0.0f)
	{
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, ActualRestore);
	}

	return ActualRestore;
}

void UHealthComponent::RestoreStaminaToFull()
{
	RestoreStamina(MaxStamina - CurrentStamina);
}

void UHealthComponent::SetStamina(float NewStamina)
{
	float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);

	if (CurrentStamina != OldStamina)
	{
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, CurrentStamina - OldStamina);
	}
}

float UHealthComponent::GetStaminaPercent() const
{
	if (MaxStamina <= 0.0f)
	{
		return 0.0f;
	}
	return CurrentStamina / MaxStamina;
}

void UHealthComponent::StartStaminaRegen()
{
	bIsRegeneratingStamina = true;
}

void UHealthComponent::UpdateStaminaRegen(float DeltaTime)
{
	if (!bIsRegeneratingStamina || !bStaminaRegenEnabled || bIsDead)
	{
		return;
	}

	if (CurrentStamina >= MaxStamina)
	{
		bIsRegeneratingStamina = false;
		return;
	}

	float RegenAmount = StaminaRegenRate * DeltaTime;
	float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Min(CurrentStamina + RegenAmount, MaxStamina);

	if (CurrentStamina != OldStamina)
	{
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, CurrentStamina - OldStamina);
	}

	if (CurrentStamina >= MaxStamina)
	{
		bIsRegeneratingStamina = false;
	}
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateStaminaRegen(DeltaTime);
}

// ==================== Floating Health Bar ====================

void UHealthComponent::CreateFloatingHealthBar()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Create widget component
	FloatingHealthBarComponent = NewObject<UWidgetComponent>(Owner, TEXT("FloatingHealthBar"));
	if (!FloatingHealthBarComponent)
	{
		return;
	}

	FloatingHealthBarComponent->RegisterComponent();
	FloatingHealthBarComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	FloatingHealthBarComponent->SetRelativeLocation(FloatingBarOffset);
	FloatingHealthBarComponent->SetDrawSize(FloatingBarDrawSize);
	FloatingHealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	FloatingHealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bFloatingBarFaceCamera)
	{
		FloatingHealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	}
	else
	{
		FloatingHealthBarComponent->SetWidgetSpace(EWidgetSpace::World);
	}

	// Create the widget
	FloatingHealthBarWidget = CreateWidget<UFloatingHealthBar>(GetWorld(), UFloatingHealthBar::StaticClass());
	if (FloatingHealthBarWidget)
	{
		FloatingHealthBarComponent->SetWidget(FloatingHealthBarWidget);

		// Set initial health
		FloatingHealthBarWidget->SetHealthPercent(GetHealthPercent(), false);

		// Handle initial visibility
		// If only showing when locked on, start hidden
		if (bOnlyShowWhenLockedOn)
		{
			HideFloatingBar();
		}
		else if (bHideBarAtFullHealth && IsFullHealth())
		{
			HideFloatingBar();
		}
	}
}

void UHealthComponent::UpdateFloatingHealthBar()
{
	if (!FloatingHealthBarWidget)
	{
		return;
	}

	// Update the health display
	FloatingHealthBarWidget->SetHealthPercent(GetHealthPercent(), true);

	// If only showing when locked on, don't mess with visibility here
	// That's handled by OnLockOnStateChanged
	if (bOnlyShowWhenLockedOn)
	{
		return;
	}

	// Handle visibility based on full health setting
	if (bHideBarAtFullHealth)
	{
		// Clear any existing hide timer
		GetWorld()->GetTimerManager().ClearTimer(HideBarTimerHandle);

		if (IsFullHealth())
		{
			// Start timer to hide bar after delay
			if (HideBarDelay > 0.0f)
			{
				GetWorld()->GetTimerManager().SetTimer(
					HideBarTimerHandle,
					this,
					&UHealthComponent::HideFloatingBar,
					HideBarDelay,
					false
				);
			}
			else
			{
				HideFloatingBar();
			}
		}
		else
		{
			// Show bar when not at full health
			ShowFloatingBar();
		}
	}
}

void UHealthComponent::HideFloatingBar()
{
	if (FloatingHealthBarWidget)
	{
		FloatingHealthBarWidget->SetBarVisible(false);
	}
}

void UHealthComponent::ShowFloatingBar()
{
	if (FloatingHealthBarWidget)
	{
		FloatingHealthBarWidget->SetBarVisible(true);
	}
}

void UHealthComponent::OnLockOnStateChanged(bool bIsLockedOn)
{
	bIsCurrentlyLockedOn = bIsLockedOn;

	if (!FloatingHealthBarWidget || !bOnlyShowWhenLockedOn)
	{
		return;
	}

	if (bIsLockedOn)
	{
		// Show bar and update health
		ShowFloatingBar();
		FloatingHealthBarWidget->SetHealthPercent(GetHealthPercent(), false);
	}
	else
	{
		// Hide bar when lock-on is lost
		HideFloatingBar();
	}
}

// ==================== Death/Ragdoll Functions ====================

void UHealthComponent::EnableRagdoll()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Try to get the character and its mesh
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (Character)
	{
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		if (Mesh)
		{
			// Stop any current animation montages
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->StopAllMontages(0.0f);
			}

			// Disable capsule collision so ragdoll doesn't fight it
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}

			// Disable character movement
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->DisableMovement();
				Movement->StopMovementImmediately();
			}

			// Enable physics simulation on mesh
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Mesh->SetSimulatePhysics(true);
			Mesh->SetAllBodiesSimulatePhysics(true);
			Mesh->SetAllBodiesBelowSimulatePhysics(NAME_None, true, true);
			Mesh->WakeAllRigidBodies();

			// Apply death impulse if configured
			if (bApplyDeathImpulse && DeathImpulseStrength > 0.0f && LastDamageCauser)
			{
				FVector ImpulseDirection = Owner->GetActorLocation() - LastDamageCauser->GetActorLocation();
				ImpulseDirection.Z = 0.3f; // Add slight upward component
				ImpulseDirection.Normalize();

				Mesh->AddImpulse(ImpulseDirection * DeathImpulseStrength, NAME_None, true);
			}
		}
	}
	else
	{
		// For non-character actors, try to find a skeletal mesh component
		USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
		if (Mesh)
		{
			Mesh->SetSimulatePhysics(true);
			Mesh->SetAllBodiesSimulatePhysics(true);
			Mesh->WakeAllRigidBodies();

			if (bApplyDeathImpulse && DeathImpulseStrength > 0.0f && LastDamageCauser)
			{
				FVector ImpulseDirection = Owner->GetActorLocation() - LastDamageCauser->GetActorLocation();
				ImpulseDirection.Z = 0.3f;
				ImpulseDirection.Normalize();

				Mesh->AddImpulse(ImpulseDirection * DeathImpulseStrength, NAME_None, true);
			}
		}
	}
}

void UHealthComponent::StopAIBehavior()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Get the pawn
	APawn* Pawn = Cast<APawn>(Owner);
	if (!Pawn)
	{
		return;
	}

	// Get the AI controller
	AAIController* AIController = Cast<AAIController>(Pawn->GetController());
	if (AIController)
	{
		// Stop the behavior tree
		if (UBrainComponent* BrainComp = AIController->GetBrainComponent())
		{
			BrainComp->StopLogic(TEXT("Death"));
		}

		// Clear any focus
		AIController->ClearFocus(EAIFocusPriority::Gameplay);

		// Stop movement
		AIController->StopMovement();

		// Unpossess the pawn so AI can't control it anymore
		AIController->UnPossess();
	}

	// Also stop any animations on the character
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (Character)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->StopAllMontages(0.0f);
			}
		}
	}
}

void UHealthComponent::DestroyOwnerActor()
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->Destroy();
	}
}
