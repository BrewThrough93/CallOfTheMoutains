// CallOfTheMoutains - Targetable Component for Lock-On System

#include "TargetableComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Character.h"

UTargetableComponent::UTargetableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTargetableComponent::BeginPlay()
{
	Super::BeginPlay();

	// Create visual indicator components
	CreateIndicatorComponents();

	// Start with indicators hidden
	HideIndicator();
}

FVector UTargetableComponent::GetTargetLocation() const
{
	if (AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation() + TargetOffset;
	}
	return FVector::ZeroVector;
}

void UTargetableComponent::NotifyTargeted()
{
	bIsCurrentlyTargeted = true;
	ShowIndicator();
	OnTargeted();
	OnTargetStateChanged.Broadcast(true);
}

void UTargetableComponent::NotifyTargetLost()
{
	bIsCurrentlyTargeted = false;
	HideIndicator();
	OnTargetLost();
	OnTargetStateChanged.Broadcast(false);
}

void UTargetableComponent::SetTargetable(bool bNewTargetable)
{
	bool bWasTargetable = bCanBeTargeted;
	bCanBeTargeted = bNewTargetable;

	// If we became non-targetable while being targeted, notify immediately
	// This ensures lock-on is released right away instead of waiting for tick
	if (bWasTargetable && !bNewTargetable && bIsCurrentlyTargeted)
	{
		NotifyTargetLost();
	}
}

void UTargetableComponent::CreateIndicatorComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Find attachment parent - either skeletal mesh bone or root component
	USceneComponent* AttachParent = nullptr;
	FName AttachSocket = NAME_None;

	if (bAttachToBone)
	{
		USkeletalMeshComponent* SkelMesh = GetOwnerSkeletalMesh();
		if (SkelMesh && SkelMesh->DoesSocketExist(AttachBoneName))
		{
			AttachParent = SkelMesh;
			AttachSocket = AttachBoneName;
		}
		else
		{
			// Fall back to root if bone not found
			AttachParent = Owner->GetRootComponent();
		}
	}
	else
	{
		AttachParent = Owner->GetRootComponent();
	}

	if (!AttachParent)
	{
		return;
	}

	// Create billboard sprite component for lock-on indicator
	if (bShowLockOnIndicator)
	{
		LockOnSpriteComponent = NewObject<UBillboardComponent>(Owner, TEXT("LockOnSprite"));
		if (LockOnSpriteComponent)
		{
			LockOnSpriteComponent->RegisterComponent();

			// Attach to bone or root
			if (AttachSocket != NAME_None)
			{
				LockOnSpriteComponent->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
			}
			else
			{
				LockOnSpriteComponent->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
			}

			LockOnSpriteComponent->SetRelativeLocation(IndicatorOffset);
			LockOnSpriteComponent->SetRelativeScale3D(FVector(SpriteScale));

			// Load and set sprite texture if specified
			if (!LockOnSprite.IsNull())
			{
				UTexture2D* SpriteTexture = LockOnSprite.LoadSynchronous();
				if (SpriteTexture)
				{
					LockOnSpriteComponent->SetSprite(SpriteTexture);
				}
			}

			// Billboard should always face camera
			LockOnSpriteComponent->SetHiddenInGame(false);
		}
	}

	// Create point light component for lock-on glow
	if (bShowLockOnLight)
	{
		LockOnLightComponent = NewObject<UPointLightComponent>(Owner, TEXT("LockOnLight"));
		if (LockOnLightComponent)
		{
			LockOnLightComponent->RegisterComponent();

			// Attach to bone or root
			if (AttachSocket != NAME_None)
			{
				LockOnLightComponent->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
			}
			else
			{
				LockOnLightComponent->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
			}

			LockOnLightComponent->SetRelativeLocation(IndicatorOffset);
			LockOnLightComponent->SetLightColor(LockOnLightColor);
			LockOnLightComponent->SetIntensity(LockOnLightIntensity);
			LockOnLightComponent->SetAttenuationRadius(LockOnLightRadius);
			LockOnLightComponent->SetCastShadows(false);
		}
	}
}

void UTargetableComponent::ShowIndicator()
{
	if (!bShowLockOnIndicator && !bShowLockOnLight)
	{
		return;
	}

	if (LockOnSpriteComponent)
	{
		LockOnSpriteComponent->SetVisibility(true);
	}

	if (LockOnLightComponent)
	{
		LockOnLightComponent->SetVisibility(true);
	}
}

void UTargetableComponent::HideIndicator()
{
	if (LockOnSpriteComponent)
	{
		LockOnSpriteComponent->SetVisibility(false);
	}

	if (LockOnLightComponent)
	{
		LockOnLightComponent->SetVisibility(false);
	}
}

USkeletalMeshComponent* UTargetableComponent::GetOwnerSkeletalMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// If owner is a character, get the mesh component
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		return Character->GetMesh();
	}

	// Otherwise try to find any skeletal mesh component
	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}
