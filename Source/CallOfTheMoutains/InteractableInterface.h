// CallOfTheMoutains - Interactable Interface
// Any actor that can be interacted with implements this interface

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

// Forward declarations
class APawn;

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for all interactable objects in the game
 * Implement this on actors that can be interacted with (pickups, doors, NPCs, etc.)
 */
class CALLOFTHEMOUTAINS_API IInteractableInterface
{
	GENERATED_BODY()

public:
	/**
	 * Called when player looks at this interactable
	 * @param Interactor The pawn looking at this object
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnFocused(APawn* Interactor);

	/**
	 * Called when player looks away from this interactable
	 * @param Interactor The pawn that was looking at this object
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnUnfocused(APawn* Interactor);

	/**
	 * Called when player interacts with this object
	 * @param Interactor The pawn interacting with this object
	 * @return True if interaction was successful
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool OnInteract(APawn* Interactor);

	/**
	 * Get the interaction prompt text to display
	 * @return The text to show the player (e.g., "Pick up Sword", "Open Door")
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;

	/**
	 * Check if this object can currently be interacted with
	 * @param Interactor The pawn trying to interact
	 * @return True if interaction is possible
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(APawn* Interactor) const;
};
