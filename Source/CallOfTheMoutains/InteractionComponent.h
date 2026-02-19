// CallOfTheMoutains - Interaction Component
// Handles detection of interactable objects via line trace

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class IInteractableInterface;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFocusChanged, AActor*, FocusedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionPromptChanged, bool, bShowPrompt, FText, PromptText);

/**
 * Component that handles interaction detection and execution
 * Add to player character/pawn, uses camera forward direction for line trace
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== Settings ====================

	/** How far the player can interact with objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange = 300.0f;

	/** Radius of sphere trace for interaction detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRadius = 50.0f;

	/** How often to check for interactables (performance optimization) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float TraceInterval = 0.1f;

	/** Collision channel to use for traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Show debug traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug")
	bool bShowDebugTraces = false;

	// ==================== State ====================

	/** Currently focused interactable actor */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	AActor* FocusedActor = nullptr;

	// ==================== Functions ====================

	/** Try to interact with the currently focused object */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	/** Get the current interaction prompt */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FText GetCurrentPrompt() const;

	/** Check if there's an interactable in focus */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool HasInteractableInFocus() const { return FocusedActor != nullptr; }

	/** Force update the interaction trace */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ForceTraceUpdate();

	// ==================== Events ====================

	/** Fired when the focused interactable changes */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableFocusChanged OnInteractableFocusChanged;

	/** Fired when the interaction prompt should be shown/hidden */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractionPromptChanged OnInteractionPromptChanged;

private:
	/** Perform interaction trace */
	void PerformInteractionTrace();

	/** Update the focused actor */
	void SetFocusedActor(AActor* NewFocusedActor);

	/** Get the camera component (for trace direction) */
	UCameraComponent* GetCamera() const;

	/** Get trace start/end positions */
	void GetTracePositions(FVector& OutStart, FVector& OutEnd) const;

	// Timing
	float TimeSinceLastTrace = 0.0f;

	// Cache
	UPROPERTY()
	AActor* LastFocusedActor = nullptr;
};
