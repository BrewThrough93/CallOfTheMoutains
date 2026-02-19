// CallOfTheMoutains - Hotbar Widget Implementation
// Dark Souls D-pad style with improved visual polish

#include "HotbarWidget.h"
#include "UIStyle.h"
#include "EquipmentComponent.h"
#include "InventoryComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SCanvas.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"

void UHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UHotbarWidget::NativeDestruct()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->OnHotbarChanged.RemoveDynamic(this, &UHotbarWidget::OnHotbarChanged);
	}
	Super::NativeDestruct();
}

void UHotbarWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	UpSlotBorder.Reset();
	DownSlotBorder.Reset();
	LeftSlotBorder.Reset();
	RightSlotBorder.Reset();
	UpSlotIcon.Reset();
	DownSlotIcon.Reset();
	LeftSlotIcon.Reset();
	RightSlotIcon.Reset();
	UpSlotQuantity.Reset();
	DownSlotQuantity.Reset();
}

TSharedRef<SWidget> UHotbarWidget::RebuildWidget()
{
	using namespace COTMStyle;

	const float CenterOffset = SLOT_SIZE + SPACING;
	const float TotalSize = (SLOT_SIZE * 3) + (SPACING * 2);

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Create the D-pad hotbar content
	TSharedRef<SWidget> HotbarContent = SNew(SBox)
		.WidthOverride(TotalSize)
		.HeightOverride(TotalSize)
		[
			SNew(SCanvas)
			// UP slot (Special/Spell)
			+ SCanvas::Slot()
			.Position(FVector2D(CenterOffset, 0.0f))
			.Size(FVector2D(SLOT_SIZE, SLOT_SIZE))
			[
				BuildSlot(UpSlotBorder, UpSlotIcon, UpSlotQuantity, UpIconBrush, false, TEXT(""))
			]
			// DOWN slot (Consumable)
			+ SCanvas::Slot()
			.Position(FVector2D(CenterOffset, CenterOffset * 2))
			.Size(FVector2D(SLOT_SIZE, SLOT_SIZE))
			[
				BuildSlot(DownSlotBorder, DownSlotIcon, DownSlotQuantity, DownIconBrush, true, TEXT(""))
			]
			// LEFT slot (Off-hand)
			+ SCanvas::Slot()
			.Position(FVector2D(0.0f, CenterOffset))
			.Size(FVector2D(SLOT_SIZE, SLOT_SIZE))
			[
				BuildSlot(LeftSlotBorder, LeftSlotIcon, UpSlotQuantity, LeftIconBrush, false, TEXT(""))
			]
			// RIGHT slot (Primary)
			+ SCanvas::Slot()
			.Position(FVector2D(CenterOffset * 2, CenterOffset))
			.Size(FVector2D(SLOT_SIZE, SLOT_SIZE))
			[
				BuildSlot(RightSlotBorder, RightSlotIcon, DownSlotQuantity, RightIconBrush, false, TEXT(""))
			]
		];

	// Wrap in a full-screen container that positions the hotbar at bottom-left
	// The outer SBox fills the entire viewport, inner content aligns within it
	return SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(24.0f, 0.0f, 0.0f, 24.0f))
			[
				HotbarContent
			]
		];
}

TSharedRef<SWidget> UHotbarWidget::BuildSlot(TSharedPtr<SBorder>& OutBorder, TSharedPtr<SImage>& OutIcon,
	TSharedPtr<STextBlock>& OutQuantity, FSlateBrush& OutBrush, bool bShowQuantity, const FString& KeyHint)
{
	using namespace COTMStyle;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Simple two-layer slot: border + background
	TSharedRef<SWidget> SlotWidget = SNew(SOverlay)
		// Layer 0: Border and background
		+ SOverlay::Slot()
		[
			SAssignNew(OutBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(Colors::BorderIron())
			.Padding(FMargin(1.0f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(Colors::BackgroundSlot())
			]
		]
		// Layer 1: Item icon
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(6.0f))
		[
			SAssignNew(OutIcon, SImage)
			.Image(&OutBrush)
			.Visibility(EVisibility::Collapsed)
		];

	// Quantity text for consumables (smaller, bottom-right)
	if (bShowQuantity)
	{
		TSharedRef<SOverlay> SlotOverlay = StaticCastSharedRef<SOverlay>(SlotWidget);
		SlotOverlay->AddSlot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(0, 0, 3.0f, 2.0f))
			[
				SAssignNew(OutQuantity, STextBlock)
				.Text(FText::GetEmpty())
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::TextPrimary()))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
			];
	}

	return SlotWidget;
}

void UHotbarWidget::InitializeHotbar(UEquipmentComponent* InEquipment, UInventoryComponent* InInventory)
{
	EquipmentComponent = InEquipment;
	InventoryComponent = InInventory;

	if (EquipmentComponent)
	{
		EquipmentComponent->OnHotbarChanged.AddDynamic(this, &UHotbarWidget::OnHotbarChanged);
	}

	UpdateAllSlots();
}

void UHotbarWidget::UpdateAllSlots()
{
	UpdateSlot(EHotbarSlot::Special);       // Up
	UpdateSlot(EHotbarSlot::PrimaryWeapon); // Right
	UpdateSlot(EHotbarSlot::OffHand);       // Left
	UpdateSlot(EHotbarSlot::Consumable);    // Down
}

void UHotbarWidget::UpdateSlot(EHotbarSlot SlotType)
{
	TSharedPtr<SBorder>* Border = nullptr;
	TSharedPtr<SImage>* Icon = nullptr;
	TSharedPtr<STextBlock>* Quantity = nullptr;
	FSlateBrush* Brush = nullptr;

	GetSlotElements(SlotType, Border, Icon, Quantity, Brush);

	if (!Icon || !Icon->IsValid()) return;

	FItemData ItemData;
	if (GetSlotItemData(SlotType, ItemData))
	{
		// Use IsNull() to check if path is set, NOT IsValid() which checks if loaded
		if (!ItemData.Icon.IsNull())
		{
			UTexture2D* IconTexture = ItemData.Icon.LoadSynchronous();
			if (IconTexture && Brush)
			{
				Brush->SetResourceObject(IconTexture);
				Brush->ImageSize = FVector2D(IconTexture->GetSizeX(), IconTexture->GetSizeY());
				Brush->DrawAs = ESlateBrushDrawType::Image;
				(*Icon)->SetImage(Brush);
				(*Icon)->SetColorAndOpacity(FLinearColor::White);
				(*Icon)->SetVisibility(EVisibility::Visible);
			}
			else
			{
				(*Icon)->SetVisibility(EVisibility::Collapsed);
			}
		}
		else
		{
			// No icon path set - show placeholder color
			(*Icon)->SetColorAndOpacity(FLinearColor(0.4f, 0.35f, 0.3f, 1.0f));
			(*Icon)->SetVisibility(EVisibility::Visible);
		}

		if (Quantity && Quantity->IsValid() && InventoryComponent)
		{
			FName ItemID = EquipmentComponent->GetCurrentHotbarItem(SlotType);
			int32 Count = InventoryComponent->GetItemCount(ItemID);
			(*Quantity)->SetText(FText::AsNumber(Count));
		}
	}
	else
	{
		(*Icon)->SetVisibility(EVisibility::Collapsed);
		if (Quantity && Quantity->IsValid())
		{
			(*Quantity)->SetText(FText::GetEmpty());
		}
	}
}

void UHotbarWidget::GetSlotElements(EHotbarSlot SlotType, TSharedPtr<SBorder>*& OutBorder,
	TSharedPtr<SImage>*& OutIcon, TSharedPtr<STextBlock>*& OutQuantity, FSlateBrush*& OutBrush)
{
	switch (SlotType)
	{
		case EHotbarSlot::Special:  // UP slot (spells)
			OutBorder = &UpSlotBorder;
			OutIcon = &UpSlotIcon;
			OutQuantity = &UpSlotQuantity;
			OutBrush = &UpIconBrush;
			break;
		case EHotbarSlot::Consumable:  // DOWN slot (consumables)
			OutBorder = &DownSlotBorder;
			OutIcon = &DownSlotIcon;
			OutQuantity = &DownSlotQuantity;
			OutBrush = &DownIconBrush;
			break;
		case EHotbarSlot::PrimaryWeapon:  // RIGHT slot
			OutBorder = &RightSlotBorder;
			OutIcon = &RightSlotIcon;
			OutQuantity = nullptr;
			OutBrush = &RightIconBrush;
			break;
		case EHotbarSlot::OffHand:  // LEFT slot
			OutBorder = &LeftSlotBorder;
			OutIcon = &LeftSlotIcon;
			OutQuantity = nullptr;
			OutBrush = &LeftIconBrush;
			break;
		default:
			OutBorder = nullptr;
			OutIcon = nullptr;
			OutQuantity = nullptr;
			OutBrush = nullptr;
			break;
	}
}

void UHotbarWidget::OnHotbarChanged(EHotbarSlot SlotType)
{
	UpdateSlot(SlotType);
}

bool UHotbarWidget::GetSlotItemData(EHotbarSlot SlotType, FItemData& OutItemData) const
{
	if (!EquipmentComponent)
	{
		return false;
	}

	FName ItemID = NAME_None;

	// First try the hotbar rotation array
	ItemID = EquipmentComponent->GetCurrentHotbarItem(SlotType);

	// If no item in hotbar array, check the equipped item for weapon slots
	if (ItemID.IsNone())
	{
		EEquipmentSlot EquipSlot = EEquipmentSlot::None;
		switch (SlotType)
		{
			case EHotbarSlot::PrimaryWeapon:
				EquipSlot = EEquipmentSlot::PrimaryWeapon;
				break;
			case EHotbarSlot::OffHand:
				EquipSlot = EEquipmentSlot::OffHand;
				break;
			default:
				break;
		}

		if (EquipSlot != EEquipmentSlot::None)
		{
			ItemID = EquipmentComponent->GetEquippedItem(EquipSlot);
		}
	}

	if (ItemID.IsNone())
	{
		return false;
	}

	// Get item data from equipment component (it has access to DataTable)
	return EquipmentComponent->GetItemData(ItemID, OutItemData);
}
