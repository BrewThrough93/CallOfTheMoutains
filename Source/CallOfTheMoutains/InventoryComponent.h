// CallOfTheMoutains - Inventory Component for item storage

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "InventoryComponent.generated.h"

class UDataTable;
class AItemPickup;

// Delegate for inventory changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, FName, ItemID, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, FName, ItemID, int32, Quantity);

/**
 * Manages player inventory - item storage, adding, removing, stacking
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** Enable debug mode - creates test items if no DataTable (DISABLE THIS TO USE YOUR REAL DATATABLE) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Debug")
	bool bDebugMode = false;

	/** Create a runtime DataTable with test items (for debugging) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void CreateDebugDataTable();

	/** Add test items to inventory for debugging */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void AddDebugItems();
	// ==================== Configuration ====================

	/** Reference to the item data table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	UDataTable* ItemDataTable;

	/** Maximum inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config", meta = (ClampMin = "1"))
	int32 MaxSlots = 40;

	// ==================== Delegates ====================

	/** Called when inventory contents change */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	/** Called when an item is added */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAdded OnItemAdded;

	/** Called when an item is removed */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemoved OnItemRemoved;

	// ==================== Core Functions ====================

	/** Add item to inventory. Returns quantity actually added */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemID, int32 Quantity = 1);

	/** Remove item from inventory. Returns quantity actually removed */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(FName ItemID, int32 Quantity = 1);

	/** Remove item at specific slot index */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAtSlot(int32 SlotIndex, int32 Quantity = 1);

	/** Check if inventory has item */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

	/** Get quantity of specific item in inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCount(FName ItemID) const;

	/** Get item data from data table */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemData(FName ItemID, FItemData& OutItemData) const;

	/** Get all inventory slots */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FInventorySlot> GetAllSlots() const { return InventorySlots; }

	/** Get slot at index */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventorySlot GetSlotAtIndex(int32 Index) const;

	/** Get slots filtered by category */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FInventorySlot> GetSlotsByCategory(EItemCategory Category) const;

	/** Get slots filtered by equipment slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FInventorySlot> GetSlotsByEquipmentSlot(EEquipmentSlot EquipSlot) const;

	/** Check if inventory is full */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsFull() const;

	/** Get number of used slots */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetUsedSlotCount() const;

	/** Swap two inventory slots */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapSlots(int32 IndexA, int32 IndexB);

	/** Sort inventory by category and name */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SortInventory();

	/** Clear all inventory slots */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	/** Set inventory slots directly (for save/load) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventorySlots(const TArray<FInventorySlot>& NewSlots);

	/** Drop item from inventory - spawns ItemPickup in world */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AItemPickup* DropItem(FName ItemID, int32 Quantity = 1, FVector DropOffset = FVector(100.0f, 0.0f, 50.0f));

	/** Drop item at specific slot index */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AItemPickup* DropItemAtSlot(int32 SlotIndex, int32 Quantity = 1, FVector DropOffset = FVector(100.0f, 0.0f, 50.0f));

protected:
	/** Inventory storage */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> InventorySlots;

	/** Find first slot containing item */
	int32 FindSlotWithItem(FName ItemID) const;

	/** Find first empty slot */
	int32 FindEmptySlot() const;

	/** Find slot with stackable space for item */
	int32 FindStackableSlot(FName ItemID, const FItemData& ItemData) const;
};
