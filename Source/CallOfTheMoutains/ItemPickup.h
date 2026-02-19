// CallOfTheMoutains - Item Pickup Actor
// Uses overlap detection for interaction, E key to pick up

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemTypes.h"
#include "ItemPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPickupFocused, AItemPickup*, Pickup, bool, bFocused);

/**
 * World-placed item pickup
 * Uses overlap detection - shows prompt when player is nearby
 * Press E to pick up
 */
UCLASS()
class CALLOFTHEMOUTAINS_API AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ==================== Components ====================

	/** Interaction trigger sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionSphere;

	/** Visual mesh for the item (physics enabled) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ItemMesh;

	/** Point light for rarity glow */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* RarityLight;

	// ==================== Item Configuration ====================

	/** Reference to item data table (required to get item names/data) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UDataTable* ItemDataTable;

	/** The ID of the item to give (must match a row in the items DataTable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID;

	/** How many of this item to give */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** Interaction range (sphere radius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float InteractionRadius = 150.0f;

	/** Should the pickup respawn after being collected? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bRespawns = false;

	/** Respawn delay in seconds (if bRespawns is true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "bRespawns", ClampMin = "1.0"))
	float RespawnDelay = 30.0f;

	// ==================== Light Settings ====================

	/** Light intensity for rarity glow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Light")
	float LightIntensity = 1000.0f;

	/** Light attenuation radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Light")
	float LightRadius = 200.0f;

	// ==================== Events ====================

	UPROPERTY(BlueprintAssignable, Category = "Item")
	FOnPickupFocused OnPickupFocused;

	// ==================== Functions ====================

	/** Get the item data for this pickup */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool GetItemData(FItemData& OutItemData) const;

	/** Get the interaction prompt text */
	UFUNCTION(BlueprintCallable, Category = "Item")
	FText GetPickupPrompt() const;

	/** Try to pick up this item (called when E is pressed) */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool TryPickup(APawn* Interactor);

	/** Check if player is in range */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsPlayerInRange() const { return OverlappingPawn != nullptr; }

	/** Get the overlapping pawn */
	UFUNCTION(BlueprintCallable, Category = "Item")
	APawn* GetOverlappingPawn() const { return OverlappingPawn; }

	/** Check if this pickup has been collected */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsCollected() const { return bIsCollected; }

	/** Get the item ID */
	UFUNCTION(BlueprintCallable, Category = "Item")
	FName GetItemID() const { return ItemID; }

	/** Set the item (for runtime spawning) */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetItem(FName NewItemID, int32 NewQuantity = 1);

	/** Get color based on item rarity */
	UFUNCTION(BlueprintCallable, Category = "Item")
	static FLinearColor GetRarityColor(EItemRarity Rarity);

	/** Update the point light color based on item rarity */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void UpdateRarityLight();

protected:
	/** Called when something enters the interaction sphere */
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Called when something leaves the interaction sphere */
	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/** Handle respawning */
	void Respawn();

	/** Hide the pickup (after being collected) */
	void HidePickup();

	/** Show the pickup */
	void ShowPickup();

	/** Handle E key input for pickup */
	void CheckForPickupInput();

	// State
	bool bIsCollected = false;

	UPROPERTY()
	APawn* OverlappingPawn = nullptr;

	// Input tracking
	bool bEKeyWasDown = false;

	// Timer handle for respawn
	FTimerHandle RespawnTimerHandle;
};
