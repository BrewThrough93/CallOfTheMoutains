// CallOfTheMoutains - Inventory Widget
// Elden Ring style: Category tabs, item grid left, details center, stats right

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class SVerticalBox;
class SHorizontalBox;
class SUniformGridPanel;
class SBorder;
class SImage;
class STextBlock;
class SScrollBox;

// Category filter tabs
UENUM(BlueprintType)
enum class EInventoryTab : uint8
{
	Equipped,  // Shows currently equipped items
	All,
	Weapons,
	Armor,
	Consumables,
	Materials,
	KeyItems
};

/**
 * Elden Ring style Inventory UI
 * - Category tabs at top
 * - Scrollable item grid on left
 * - Item details with large preview in center
 * - Character stats on right
 */
UCLASS()
class CALLOFTHEMOUTAINS_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory UI")
	void InitializeInventory(UInventoryComponent* InInventory, UEquipmentComponent* InEquipment);

	void RefreshAll();
	void RefreshInventoryGrid();
	void RefreshEquipmentDisplay();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	UEquipmentComponent* EquipmentComponent;

	// Current state
	int32 SelectedSlotIndex = 0;
	EInventoryTab CurrentTab = EInventoryTab::Equipped;  // Start on Equipped tab
	TArray<int32> FilteredSlotIndices; // Maps display index to actual inventory index
	TArray<EEquipmentSlot> FilteredEquipSlots; // Maps display index to equipment slot (for Equipped tab)

	// Input state tracking for tick-based polling
	bool bUpWasDown = false;
	bool bDownWasDown = false;
	bool bLeftWasDown = false;
	bool bRightWasDown = false;
	bool bQWasDown = false;
	bool bEWasDown = false;
	bool bEnterWasDown = false;
	bool bXWasDown = false;
	bool bEscWasDown = false;
	bool bTabWasDown = false;

	// Action menu state
	bool bActionMenuOpen = false;
	int32 ActionMenuSelection = 0;
	TArray<FString> CurrentActionOptions; // Dynamic based on item type

	// Slate widgets - Main structure
	TSharedPtr<SBorder> MainBackground;
	TSharedPtr<SHorizontalBox> TabBar;
	TSharedPtr<SScrollBox> ItemScrollBox;
	TSharedPtr<SVerticalBox> ItemListContainer;

	// Item details panel
	TSharedPtr<SImage> DetailItemIcon;
	TSharedPtr<STextBlock> DetailItemName;
	TSharedPtr<STextBlock> DetailItemType;
	TSharedPtr<STextBlock> DetailItemDesc;
	TSharedPtr<STextBlock> DetailItemStats;
	TSharedPtr<STextBlock> DetailItemEffect;
	FSlateBrush DetailIconBrush;

	// Stats panel
	TSharedPtr<STextBlock> StatHealth;
	TSharedPtr<STextBlock> StatStamina;
	TSharedPtr<STextBlock> StatDamage;
	TSharedPtr<STextBlock> StatDefense;
	TSharedPtr<STextBlock> StatPoise;
	TSharedPtr<STextBlock> StatWeight;

	// Item slot widgets (for the grid)
	TArray<TSharedPtr<SBorder>> SlotBorders;
	TArray<TSharedPtr<SImage>> SlotIcons;
	TArray<TSharedPtr<STextBlock>> SlotQuantities;
	TArray<TSharedPtr<STextBlock>> SlotEquippedBadges;
	TArray<FSlateBrush> SlotBrushes;

	// Tab buttons
	TArray<TSharedPtr<SBorder>> TabBorders;

	// Equipment panel slots (left side showing equipped items)
	TMap<EEquipmentSlot, TSharedPtr<SBorder>> EquipSlotBorders;
	TMap<EEquipmentSlot, TSharedPtr<SImage>> EquipSlotIcons;
	TMap<EEquipmentSlot, FSlateBrush> EquipSlotBrushes;
	EEquipmentSlot SelectedEquipSlot = EEquipmentSlot::None;
	bool bEquipPanelFocused = false; // True = focus on equipment panel, False = focus on inventory grid

	// Action menu widgets
	TSharedPtr<SBorder> ActionMenuPanel;
	TSharedPtr<SVerticalBox> ActionMenuContainer;
	TArray<TSharedPtr<SBorder>> ActionOptionBorders;
	TArray<TSharedPtr<STextBlock>> ActionOptionTexts;

	// Build functions
	TSharedRef<SWidget> BuildCategoryTabs();
	TSharedRef<SWidget> BuildEquipmentPanel();
	TSharedRef<SWidget> BuildEquipmentSlot(EEquipmentSlot SlotType, const FString& Label);
	TSharedRef<SWidget> BuildItemGrid();
	TSharedRef<SWidget> BuildItemSlot(int32 DisplayIndex);
	TSharedRef<SWidget> BuildDetailsPanel();
	TSharedRef<SWidget> BuildStatsPanel();
	TSharedRef<SWidget> BuildActionMenu();

	// Navigation
	void NavigateSelection(int32 Delta);
	void CycleTab(int32 Direction);
	void UpdateSelectionHighlight();
	void UpdateItemDetails();
	void UpdateFilteredItems();
	void UpdateTabHighlight();

	// Equipment panel navigation
	void NavigateEquipmentSlot(int32 Delta);
	void UpdateEquipmentHighlight();
	void RefreshEquipmentSlotIcons();
	void SwitchFocusPanel(); // Tab to switch between equipment panel and inventory grid
	TArray<EEquipmentSlot> GetEquipmentSlotOrder() const;

	// Actions
	void TryEquipSelected();
	void TryUseSelected();
	void TryUnequipSelected();
	void TryDropSelected(int32 Quantity = 1);
	void TrySplitSelected();

	// Action menu
	void ShowActionMenu();
	void HideActionMenu();
	void NavigateActionMenu(int32 Delta);
	void ExecuteSelectedAction();
	void PopulateActionOptions();
	void UpdateActionMenuHighlight();
	bool IsSelectedItemEquipped() const;
	bool IsShiftHeld() const;

	// Callbacks
	UFUNCTION()
	void OnInventoryChanged();

	UFUNCTION()
	void OnEquipmentChanged(EEquipmentSlot SlotType, FName NewItemID);

	// Helpers
	static FLinearColor GetRarityColor(EItemRarity Rarity);
	static FString GetCategoryName(EInventoryTab Tab);
	bool ItemMatchesFilter(const FItemData& ItemData) const;

	// Layout constants
	static constexpr float SLOT_SIZE = 64.0f;
	static constexpr int32 GRID_COLUMNS = 5;
	static constexpr int32 VISIBLE_ROWS = 6;
};
