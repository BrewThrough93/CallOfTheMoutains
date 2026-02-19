// CallOfTheMoutains - Combat Input Component
// Add this component to any character to enable combat input (LMB, RMB, Q, C)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatInputComponent.generated.h"

class UEquipmentComponent;

/**
 * Handles combat input via direct key polling
 * LMB = Light Attack
 * RMB = Heavy Attack
 * Q = Guard (hold)
 * C = Stow/Draw Weapons
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UCombatInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatInputComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Cached reference to EquipmentComponent (on controller) */
	UPROPERTY()
	UEquipmentComponent* EquipmentComponent;

	/** Enable debug messages on screen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Debug")
	bool bShowDebugMessages = false;

protected:
	// Key state tracking (to detect press, not hold)
	bool bLeftMouseWasDown = false;
	bool bRightMouseWasDown = false;
	bool bQKeyWasDown = false;
	bool bCKeyWasDown = false;

	/** Process combat input each frame */
	void HandleCombatInput();
};
