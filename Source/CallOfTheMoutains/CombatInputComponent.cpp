// CallOfTheMoutains - Combat Input Component Implementation

#include "CombatInputComponent.h"
#include "EquipmentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

UCombatInputComponent::UCombatInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCombatInputComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find EquipmentComponent on the controller
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		AController* Controller = OwnerPawn->GetController();
		if (Controller)
		{
			EquipmentComponent = Controller->FindComponentByClass<UEquipmentComponent>();
		}
	}
}

void UCombatInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HandleCombatInput();
}

void UCombatInputComponent::HandleCombatInput()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return;
	}

	// Try to find EquipmentComponent if we don't have it yet
	if (!EquipmentComponent)
	{
		EquipmentComponent = PC->FindComponentByClass<UEquipmentComponent>();
		if (!EquipmentComponent)
		{
			return;
		}
	}

	// Get input states
	bool bLMB = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	bool bRMB = PC->IsInputKeyDown(EKeys::RightMouseButton);
	bool bQ = PC->IsInputKeyDown(EKeys::Q);
	bool bC = PC->IsInputKeyDown(EKeys::C);

	// Left Mouse Button - Light Attack (on press)
	if (bLMB && !bLeftMouseWasDown)
	{
		EquipmentComponent->LightAttack();
	}
	bLeftMouseWasDown = bLMB;

	// Right Mouse Button - Heavy Attack (on press)
	if (bRMB && !bRightMouseWasDown)
	{
		EquipmentComponent->HeavyAttack();
	}
	bRightMouseWasDown = bRMB;

	// Q Key - Guard (hold)
	if (bQ && !bQKeyWasDown)
	{
		EquipmentComponent->StartGuard();
	}
	else if (!bQ && bQKeyWasDown)
	{
		EquipmentComponent->StopGuard();
	}
	bQKeyWasDown = bQ;

	// C Key - Stow/Draw Weapons (on press)
	if (bC && !bCKeyWasDown)
	{
		EquipmentComponent->ToggleWeaponStow();
	}
	bCKeyWasDown = bC;
}
