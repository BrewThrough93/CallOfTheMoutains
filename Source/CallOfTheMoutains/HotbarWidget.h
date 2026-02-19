// CallOfTheMoutains - Souls-like Hotbar HUD Widget
// D-Pad style: Up=Consumable, Right=Primary, Left=OffHand, Down=Special
// Uses Slate widgets for reliable programmatic UI

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "HotbarWidget.generated.h"

class UEquipmentComponent;
class UInventoryComponent;
class SBox;
class SBorder;
class SImage;
class STextBlock;
class SOverlay;

/**
 * Souls-like hotbar HUD widget - builds UI with Slate
 * Positioned bottom-left, D-pad layout
 */
UCLASS()
class CALLOFTHEMOUTAINS_API UHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Initialize with component references */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void InitializeHotbar(UEquipmentComponent* InEquipment, UInventoryComponent* InInventory);

	/** Update all slot visuals */
	void UpdateAllSlots();

	/** Update specific slot visual */
	void UpdateSlot(EHotbarSlot SlotType);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY()
	UEquipmentComponent* EquipmentComponent;

	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	// Style colors
	FLinearColor BackgroundColor;
	FLinearColor BorderColor;
	FLinearColor EmptySlotColor;

	// Slate widgets - D-pad slots
	TSharedPtr<SBorder> UpSlotBorder;
	TSharedPtr<SBorder> DownSlotBorder;
	TSharedPtr<SBorder> LeftSlotBorder;
	TSharedPtr<SBorder> RightSlotBorder;

	TSharedPtr<SImage> UpSlotIcon;
	TSharedPtr<SImage> DownSlotIcon;
	TSharedPtr<SImage> LeftSlotIcon;
	TSharedPtr<SImage> RightSlotIcon;

	TSharedPtr<STextBlock> UpSlotQuantity;
	TSharedPtr<STextBlock> DownSlotQuantity;

	// Brushes for item icons
	FSlateBrush UpIconBrush;
	FSlateBrush DownIconBrush;
	FSlateBrush LeftIconBrush;
	FSlateBrush RightIconBrush;

	/** Build a single D-pad slot */
	TSharedRef<SWidget> BuildSlot(TSharedPtr<SBorder>& OutBorder, TSharedPtr<SImage>& OutIcon,
		TSharedPtr<STextBlock>& OutQuantity, FSlateBrush& OutBrush, bool bShowQuantity, const FString& KeyHint);

	/** Get slot elements by type */
	void GetSlotElements(EHotbarSlot SlotType, TSharedPtr<SBorder>*& OutBorder, TSharedPtr<SImage>*& OutIcon,
		TSharedPtr<STextBlock>*& OutQuantity, FSlateBrush*& OutBrush);

	/** Get item data for slot */
	bool GetSlotItemData(EHotbarSlot SlotType, FItemData& OutItemData) const;

	/** Called when hotbar changes */
	UFUNCTION()
	void OnHotbarChanged(EHotbarSlot SlotType);

	static constexpr float SLOT_SIZE = 48.0f;
	static constexpr float SPACING = 3.0f;
};
