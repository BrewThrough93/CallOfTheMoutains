// CallOfTheMoutains - Test Dummy for Lock-On Testing

#include "TestDummyActor.h"
#include "TargetableComponent.h"
#include "HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"

ATestDummyActor::ATestDummyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create capsule collision as root - REQUIRED for sphere overlap detection
	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision"));
	CapsuleCollision->InitCapsuleSize(50.0f, 100.0f); // Radius 50, half-height 100
	// Use custom collision settings for reliable detection
	CapsuleCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleCollision->SetCollisionObjectType(ECC_WorldDynamic);
	CapsuleCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
	CapsuleCollision->SetGenerateOverlapEvents(true);
	CapsuleCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f)); // Raise to ground level
	RootComponent = CapsuleCollision;

	// Create mesh
	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
	DummyMesh->SetupAttachment(RootComponent);

	// Set default cylinder mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		DummyMesh->SetStaticMesh(CylinderMesh.Object);
		DummyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 2.0f));
		DummyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f)); // Capsule handles positioning
		DummyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Capsule handles collision
	}

	// Create targetable component - offset at chest height
	TargetableComponent = CreateDefaultSubobject<UTargetableComponent>(TEXT("TargetableComponent"));
	TargetableComponent->TargetOffset = FVector(0.0f, 0.0f, 50.0f); // Lower chest level

	// Create health component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 100.0f;

	// Create lock-on point light (hidden by default)
	LockOnLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LockOnLight"));
	LockOnLight->SetupAttachment(RootComponent);
	LockOnLight->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f)); // Relative to capsule center
	LockOnLight->SetIntensity(0.0f); // Off by default
	LockOnLight->SetLightColor(FLinearColor::Red);
	LockOnLight->SetAttenuationRadius(200.0f);
	LockOnLight->SetCastShadows(false);

	// Create lock-on sprite indicator
	LockOnSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("LockOnSprite"));
	LockOnSprite->SetupAttachment(RootComponent);
	LockOnSprite->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f)); // Relative to capsule center
	LockOnSprite->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
	LockOnSprite->SetHiddenInGame(true); // Hidden by default

	// Try to load a default crosshair texture
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshairTexture(TEXT("/Engine/EngineResources/Cursors/Arrow"));
	if (CrosshairTexture.Succeeded())
	{
		LockOnSprite->SetSprite(CrosshairTexture.Object);
	}

	// Create lock-on indicator (widget above head) - keep as backup
	LockOnIndicator = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnIndicator"));
	LockOnIndicator->SetupAttachment(RootComponent);
	LockOnIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f)); // Above capsule
	LockOnIndicator->SetWidgetSpace(EWidgetSpace::Screen);
	LockOnIndicator->SetDrawSize(FVector2D(50.0f, 50.0f));
	LockOnIndicator->SetVisibility(false);
}

void ATestDummyActor::BeginPlay()
{
	Super::BeginPlay();

	// Bind to target state changes
	if (TargetableComponent)
	{
		TargetableComponent->OnTargetStateChanged.AddDynamic(this, &ATestDummyActor::OnTargetStateChanged);
	}

	// Bind to health events
	if (HealthComponent)
	{
		HealthComponent->OnDamageReceived.AddDynamic(this, &ATestDummyActor::OnDamageReceived);
		HealthComponent->OnDeath.AddDynamic(this, &ATestDummyActor::OnDeath);
	}
}

void ATestDummyActor::SetLockOnIndicatorVisible(bool bVisible)
{
	// Show/hide point light
	if (LockOnLight)
	{
		LockOnLight->SetIntensity(bVisible ? TargetedLightIntensity : 0.0f);
		LockOnLight->SetLightColor(TargetedLightColor);
	}

	// Show/hide sprite
	if (LockOnSprite)
	{
		LockOnSprite->SetHiddenInGame(!bVisible);
	}

	// Show/hide widget
	if (LockOnIndicator)
	{
		LockOnIndicator->SetVisibility(bVisible);
	}
}

void ATestDummyActor::OnTargetStateChanged(bool bIsTargeted)
{
	SetLockOnIndicatorVisible(bIsTargeted);

	// Visual feedback - change mesh color when targeted
	if (DummyMesh)
	{
		UMaterialInstanceDynamic* DynamicMaterial = DummyMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			if (bIsTargeted)
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Red);
			}
			else
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Gray);
			}
		}
	}
}

void ATestDummyActor::OnDamageReceived(float Damage, AActor* DamageCauser, AController* InstigatorController)
{
	// Flash white when hit
	if (DummyMesh)
	{
		UMaterialInstanceDynamic* DynamicMaterial = DummyMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::White);

			// Reset color after short delay
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, [this]()
			{
				if (DummyMesh && HealthComponent && HealthComponent->IsAlive())
				{
					UMaterialInstanceDynamic* Mat = Cast<UMaterialInstanceDynamic>(DummyMesh->GetMaterial(0));
					if (Mat)
					{
						Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Gray);
					}
				}
			}, 0.1f, false);
		}
	}

}

void ATestDummyActor::OnDeath(AActor* KilledBy, AController* InstigatorController)
{
	// Turn black when dead
	if (DummyMesh)
	{
		UMaterialInstanceDynamic* DynamicMaterial = DummyMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Black);
		}
	}

	// Disable targeting
	if (TargetableComponent)
	{
		TargetableComponent->bCanBeTargeted = false;
	}

	// Hide lock-on indicator
	SetLockOnIndicatorVisible(false);

	if (bRespawns)
	{
		// Hide the dummy
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);

		// Schedule respawn
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ATestDummyActor::Respawn, RespawnDelay, false);
	}
	else
	{
		// Destroy after short delay
		SetLifeSpan(2.0f);
	}
}

void ATestDummyActor::Respawn()
{
	// Show the dummy
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// Revive with full health
	if (HealthComponent)
	{
		HealthComponent->Revive();
	}

	// Re-enable targeting
	if (TargetableComponent)
	{
		TargetableComponent->bCanBeTargeted = true;
	}

	// Reset to default color
	if (DummyMesh)
	{
		UMaterialInstanceDynamic* DynamicMaterial = DummyMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Gray);
		}
	}
}
