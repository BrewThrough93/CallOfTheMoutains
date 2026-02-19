// CallOfTheMoutains - Bile Projectile Implementation

#include "BileProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HealthComponent.h"

ABileProjectile::ABileProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(20.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	CollisionSphere->SetGenerateOverlapEvents(false);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionSphere;

	// Create projectile movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed * 1.5f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = GravityScale;

	// Create bile effect
	BileEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BileEffect"));
	BileEffect->SetupAttachment(RootComponent);
	BileEffect->bAutoActivate = true;

	// Create flight sound
	FlightSound = CreateDefaultSubobject<UAudioComponent>(TEXT("FlightSound"));
	FlightSound->SetupAttachment(RootComponent);
	FlightSound->bAutoActivate = true;
}

void ABileProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Bind hit event
	CollisionSphere->OnComponentHit.AddDynamic(this, &ABileProjectile::OnHit);

	// Apply configured settings to movement component
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed * 1.5f;
	ProjectileMovement->ProjectileGravityScale = GravityScale;

	// Set lifetime
	SetLifeSpan(Lifetime);
}

void ABileProjectile::InitializeProjectile(AActor* InOwner, FVector Direction)
{
	OwnerActor = InOwner;

	// Ignore collision with owner
	if (OwnerActor)
	{
		CollisionSphere->MoveIgnoreActors.Add(OwnerActor);
	}

	// Set velocity
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction * ProjectileSpeed;
	}
}

void ABileProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Prevent double hits
	if (bHasHit)
	{
		return;
	}

	// Don't hit owner
	if (OtherActor == OwnerActor)
	{
		return;
	}

	bHasHit = true;

	// Stop movement
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	// Apply damage if we hit something with health
	if (OtherActor)
	{
		ApplyDamage(OtherActor, Hit);
		ApplySlowDebuff(OtherActor);
	}

	// Spawn impact effects
	SpawnImpactEffects(Hit.ImpactPoint, Hit.ImpactNormal);

	// Destroy projectile
	DestroyProjectile();
}

void ABileProjectile::ApplyDamage(AActor* Target, const FHitResult& Hit)
{
	if (!Target)
	{
		return;
	}

	// Try to find health component
	UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();
	if (TargetHealth)
	{
		AController* InstigatorController = nullptr;
		if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
		{
			InstigatorController = OwnerPawn->GetController();
		}

		TargetHealth->TakeDamage(DirectDamage, OwnerActor, InstigatorController);
	}
	else
	{
		// Fallback to standard damage system
		FDamageEvent DamageEvent;
		if (DamageTypeClass)
		{
			DamageEvent.DamageTypeClass = DamageTypeClass;
		}

		AController* InstigatorController = nullptr;
		if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
		{
			InstigatorController = OwnerPawn->GetController();
		}

		Target->TakeDamage(DirectDamage, DamageEvent, InstigatorController, this);
	}
}

void ABileProjectile::ApplySlowDebuff(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	// Only apply slow to characters with movement component
	ACharacter* Character = Cast<ACharacter>(Target);
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// Store original speed
	float OriginalMaxWalkSpeed = Movement->MaxWalkSpeed;

	// Apply slow
	Movement->MaxWalkSpeed *= (1.0f - SlowPercent);

	// Set timer to restore speed
	FTimerHandle SlowTimerHandle;
	FTimerDelegate SlowDelegate;
	SlowDelegate.BindLambda([Movement, OriginalMaxWalkSpeed]()
	{
		if (Movement && IsValid(Movement))
		{
			Movement->MaxWalkSpeed = OriginalMaxWalkSpeed;
		}
	});

	GetWorld()->GetTimerManager().SetTimer(SlowTimerHandle, SlowDelegate, SlowDuration, false);
}

void ABileProjectile::SpawnImpactEffects(FVector Location, FVector Normal)
{
	// Spawn impact VFX
	if (ImpactEffect)
	{
		UNiagaraComponent* SpawnedEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactEffect,
			Location,
			Normal.Rotation(),
			ImpactEffectScale,
			true,
			true
		);
	}

	// Play impact sound
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ImpactSound,
			Location,
			ImpactSoundVolume
		);
	}
}

void ABileProjectile::DestroyProjectile()
{
	// Stop effects
	if (BileEffect)
	{
		BileEffect->Deactivate();
	}

	if (FlightSound)
	{
		FlightSound->Stop();
	}

	// Disable collision
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Destroy after a short delay to allow effects to finish
	SetLifeSpan(0.1f);
}
