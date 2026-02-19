// FireActor.cpp

#include "FireActor.h"
#include "HealthComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"

AFireActor::AFireActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create damage volume
	DamageVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageVolume"));
	DamageVolume->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	DamageVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	DamageVolume->SetGenerateOverlapEvents(true);
	RootComponent = DamageVolume;

	// Create fire effect (assign Niagara system in Blueprint)
	FireEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffect"));
	FireEffect->SetupAttachment(RootComponent);
	FireEffect->bAutoActivate = true;

	// Create fire sound (assign sound in Blueprint)
	FireSound = CreateDefaultSubobject<UAudioComponent>(TEXT("FireSound"));
	FireSound->SetupAttachment(RootComponent);
	FireSound->bAutoActivate = true;
}

void AFireActor::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap events
	DamageVolume->OnComponentBeginOverlap.AddDynamic(this, &AFireActor::OnOverlapBegin);
	DamageVolume->OnComponentEndOverlap.AddDynamic(this, &AFireActor::OnOverlapEnd);

	// Set initial state
	SetFireActive(bIsActive);
}

void AFireActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive || ActorsInFire.Num() == 0)
	{
		return;
	}

	// Accumulate time
	DamageTimer += DeltaTime;

	// Apply damage at interval
	if (DamageTimer >= DamageInterval)
	{
		ApplyDamageToOverlappingActors();
		DamageTimer = 0.0f;
	}
}

void AFireActor::SetFireActive(bool bActive)
{
	bIsActive = bActive;

	if (FireEffect)
	{
		if (bActive)
		{
			FireEffect->Activate(true);
		}
		else
		{
			FireEffect->Deactivate();
		}
	}

	if (FireSound)
	{
		if (bActive)
		{
			FireSound->Play();
		}
		else
		{
			FireSound->Stop();
		}
	}

	// Reset damage timer when activating
	if (bActive)
	{
		DamageTimer = 0.0f;
	}
}

void AFireActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		// Only track actors with health components
		if (OtherActor->FindComponentByClass<UHealthComponent>())
		{
			ActorsInFire.AddUnique(OtherActor);
		}
	}
}

void AFireActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		ActorsInFire.Remove(OtherActor);
	}
}

void AFireActor::ApplyDamageToOverlappingActors()
{
	// Create a copy to iterate over (in case actors die and get removed)
	TArray<AActor*> ActorsToProcess = ActorsInFire;

	for (AActor* Actor : ActorsToProcess)
	{
		if (!IsValid(Actor))
		{
			ActorsInFire.Remove(Actor);
			continue;
		}

		UHealthComponent* HealthComp = Actor->FindComponentByClass<UHealthComponent>();
		if (HealthComp && !HealthComp->IsDead())
		{
			// Apply fire damage
			HealthComp->TakeDamage(DamagePerTick, this, nullptr);
		}
		else if (HealthComp && HealthComp->IsDead())
		{
			// Remove dead actors from tracking
			ActorsInFire.Remove(Actor);
		}
	}
}
