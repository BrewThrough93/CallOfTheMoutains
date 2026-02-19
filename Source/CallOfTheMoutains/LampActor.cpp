// CallOfTheMoutains - Lamp Actor Implementation

#include "LampActor.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ALampActor::ALampActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root scene component
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Create lamp mesh
	LampMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LampMesh"));
	LampMesh->SetupAttachment(RootScene);
	LampMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create point light
	LampLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LampLight"));
	LampLight->SetupAttachment(LampMesh);
	LampLight->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f)); // Offset light above lamp mesh
	LampLight->SetIntensity(0.0f); // Start off
	LampLight->SetLightColor(LightColor);
	LampLight->SetAttenuationRadius(LightRadius);
	LampLight->SetCastShadows(true);
	LampLight->SetVisibility(false);
}

void ALampActor::BeginPlay()
{
	Super::BeginPlay();

	// Apply initial configuration
	LampLight->SetLightColor(LightColor);
	LampLight->SetAttenuationRadius(LightRadius);

	// Ensure lamp starts in the correct state
	UpdateLightState();
}

void ALampActor::ToggleLamp()
{
	if (bIsLampOn)
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}

void ALampActor::TurnOn()
{
	if (!bIsLampOn)
	{
		bIsLampOn = true;
		UpdateLightState();

		// Play turn on sound
		if (TurnOnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TurnOnSound, GetActorLocation(), SoundVolume);
		}
	}
}

void ALampActor::TurnOff()
{
	if (bIsLampOn)
	{
		bIsLampOn = false;
		UpdateLightState();

		// Play turn off sound
		if (TurnOffSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TurnOffSound, GetActorLocation(), SoundVolume);
		}
	}
}

void ALampActor::UpdateLightState()
{
	if (LampLight)
	{
		if (bIsLampOn)
		{
			LampLight->SetIntensity(LightIntensity);
			LampLight->SetVisibility(true);
		}
		else
		{
			LampLight->SetIntensity(0.0f);
			LampLight->SetVisibility(false);
		}
	}
}

void ALampActor::AttachToCharacter(ACharacter* Character)
{
	// Use default socket name
	AttachToCharacterAtSocket(Character, AttachSocketName);
}

void ALampActor::AttachToCharacterAtSocket(ACharacter* Character, FName SocketName)
{
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	// Use provided socket, or fallback to default if not valid
	FName SocketToUse = SocketName;
	if (SocketToUse.IsNone() || !CharacterMesh->DoesSocketExist(SocketToUse))
	{
		// Try default socket
		SocketToUse = AttachSocketName;
		if (!CharacterMesh->DoesSocketExist(SocketToUse))
		{
			// Fallback to common socket names
			if (CharacterMesh->DoesSocketExist(TEXT("hand_l")))
			{
				SocketToUse = TEXT("hand_l");
			}
			else if (CharacterMesh->DoesSocketExist(TEXT("LeftHand")))
			{
				SocketToUse = TEXT("LeftHand");
			}
		}
	}

	// Attach to character's skeletal mesh at socket, using socket's rotation and scale
	AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketToUse);
}

void ALampActor::DetachFromCharacter()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ALampActor::SetLightProperties(FLinearColor NewColor, float NewIntensity, float NewRadius)
{
	LightColor = NewColor;
	LightIntensity = NewIntensity;
	LightRadius = NewRadius;

	if (LampLight)
	{
		LampLight->SetLightColor(LightColor);
		LampLight->SetAttenuationRadius(LightRadius);

		if (bIsLampOn)
		{
			LampLight->SetIntensity(LightIntensity);
		}
	}
}

void ALampActor::SetLampMesh(UStaticMesh* NewMesh)
{
	if (LampMesh && NewMesh)
	{
		LampMesh->SetStaticMesh(NewMesh);
		LampMesh->SetVisibility(true);
	}
}
