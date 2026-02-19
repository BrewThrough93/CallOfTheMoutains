// CallOfTheMoutains - Inventory Widget Implementation
// Elden Ring style layout

#include "InventoryWidget.h"
#include "UIStyle.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

void UInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Only process input when visible
	if (GetVisibility() != ESlateVisibility::Visible)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	// Escape - close action menu or close inventory
	bool bEscDown = PC->IsInputKeyDown(EKeys::Escape);
	if (bEscDown && !bEscWasDown)
	{
		if (bActionMenuOpen)
		{
			HideActionMenu();
		}
		// Note: Closing inventory is handled by SoulsLikeCharacter via I key
	}
	bEscWasDown = bEscDown;

	// X - Drop item (Shift+X = drop all)
	bool bXDown = PC->IsInputKeyDown(EKeys::X);
	if (bXDown && !bXWasDown && !bActionMenuOpen)
	{
		if (IsShiftHeld())
		{
			// Drop all
			if (FilteredSlotIndices.IsValidIndex(SelectedSlotIndex) && InventoryComponent)
			{
				int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
				TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
				if (AllSlots.IsValidIndex(ActualIdx))
				{
					TryDropSelected(AllSlots[ActualIdx].Quantity);
				}
			}
		}
		else
		{
			// Drop one
			TryDropSelected(1);
		}
	}
	bXWasDown = bXDown;

	// If action menu is open, handle menu navigation
	if (bActionMenuOpen)
	{
		// Navigate action menu up/down (arrow keys only - WASD is for player movement)
		bool bUpDown = PC->IsInputKeyDown(EKeys::Up);
		if (bUpDown && !bUpWasDown)
		{
			NavigateActionMenu(-1);
		}
		bUpWasDown = bUpDown;

		bool bDownDown = PC->IsInputKeyDown(EKeys::Down);
		if (bDownDown && !bDownWasDown)
		{
			NavigateActionMenu(1);
		}
		bDownWasDown = bDownDown;

		// Enter - execute selected action
		bool bEnterDown = PC->IsInputKeyDown(EKeys::Enter);
		if (bEnterDown && !bEnterWasDown)
		{
			ExecuteSelectedAction();
		}
		bEnterWasDown = bEnterDown;

		return; // Don't process grid navigation while menu is open
	}

	// Tab - Switch between equipment panel and inventory grid
	bool bTabDown = PC->IsInputKeyDown(EKeys::Tab);
	if (bTabDown && !bTabWasDown)
	{
		SwitchFocusPanel();
	}
	bTabWasDown = bTabDown;

	// Navigation depends on which panel is focused
	bool bUpDown = PC->IsInputKeyDown(EKeys::Up);
	if (bUpDown && !bUpWasDown)
	{
		if (bEquipPanelFocused)
		{
			NavigateEquipmentSlot(-1);
		}
		else
		{
			NavigateSelection(-GRID_COLUMNS);
		}
	}
	bUpWasDown = bUpDown;

	bool bDownDown = PC->IsInputKeyDown(EKeys::Down);
	if (bDownDown && !bDownWasDown)
	{
		if (bEquipPanelFocused)
		{
			NavigateEquipmentSlot(1);
		}
		else
		{
			NavigateSelection(GRID_COLUMNS);
		}
	}
	bDownWasDown = bDownDown;

	bool bLeftDown = PC->IsInputKeyDown(EKeys::Left);
	if (bLeftDown && !bLeftWasDown)
	{
		if (!bEquipPanelFocused)
		{
			NavigateSelection(-1);
		}
	}
	bLeftWasDown = bLeftDown;

	bool bRightDown = PC->IsInputKeyDown(EKeys::Right);
	if (bRightDown && !bRightWasDown)
	{
		if (!bEquipPanelFocused)
		{
			NavigateSelection(1);
		}
	}
	bRightWasDown = bRightDown;

	// Previous tab - Q (only for inventory grid)
	bool bQDown = PC->IsInputKeyDown(EKeys::Q);
	if (bQDown && !bQWasDown && !bEquipPanelFocused)
	{
		CycleTab(-1);
	}
	bQWasDown = bQDown;

	// Next tab - E (only for inventory grid)
	bool bEDown = PC->IsInputKeyDown(EKeys::E);
	if (bEDown && !bEWasDown && !bEquipPanelFocused)
	{
		CycleTab(1);
	}
	bEWasDown = bEDown;

	// Enter - Open action menu or unequip from equipment panel
	bool bEnterDown = PC->IsInputKeyDown(EKeys::Enter);
	if (bEnterDown && !bEnterWasDown)
	{
		if (bEquipPanelFocused)
		{
			// Unequip selected equipment slot
			if (SelectedEquipSlot != EEquipmentSlot::None && EquipmentComponent)
			{
				if (EquipmentComponent->UnequipSlot(SelectedEquipSlot))
				{
					RefreshAll();
				}
			}
		}
		else if (CurrentTab == EInventoryTab::Equipped)
		{
			// Show action menu for equipped items (uses FilteredEquipSlots)
			if (FilteredEquipSlots.Num() > 0 && FilteredEquipSlots.IsValidIndex(SelectedSlotIndex))
			{
				ShowActionMenu();
			}
		}
		else
		{
			// Show action menu for inventory item
			if (FilteredSlotIndices.Num() > 0 && FilteredSlotIndices.IsValidIndex(SelectedSlotIndex))
			{
				ShowActionMenu();
			}
		}
	}
	bEnterWasDown = bEnterDown;
}

void UInventoryWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::OnInventoryChanged);
	}
	if (EquipmentComponent)
	{
		EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UInventoryWidget::OnEquipmentChanged);
	}
	Super::NativeDestruct();
}

void UInventoryWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MainBackground.Reset();
	TabBar.Reset();
	ItemScrollBox.Reset();
	ItemListContainer.Reset();
	DetailItemIcon.Reset();
	DetailItemName.Reset();
	DetailItemType.Reset();
	DetailItemDesc.Reset();
	DetailItemStats.Reset();
	DetailItemEffect.Reset();

	SlotBorders.Empty();
	SlotIcons.Empty();
	SlotQuantities.Empty();
	SlotEquippedBadges.Empty();
	SlotBrushes.Empty();
	TabBorders.Empty();

	// Equipment panel
	EquipSlotBorders.Empty();
	EquipSlotIcons.Empty();
	EquipSlotBrushes.Empty();

	ActionMenuPanel.Reset();
	ActionMenuContainer.Reset();
	ActionOptionBorders.Empty();
	ActionOptionTexts.Empty();
}

TSharedRef<SWidget> UInventoryWidget::RebuildWidget()
{
	using namespace COTMStyle;

	// Initialize brushes for item slots
	SlotBrushes.Empty();
	for (int32 i = 0; i < GRID_COLUMNS * VISIBLE_ROWS; i++)
	{
		SlotBrushes.Add(FSlateBrush());
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Main layout: full screen overlay with semi-transparent background
	return SNew(SOverlay)
		// Layer 0: Main inventory UI
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(MainBackground, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f))
			.Padding(FMargin(40.0f))
			[
				SNew(SVerticalBox)
				// Title bar
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0, 0, 0, 8.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Inventory")))
						.Font(Fonts::Header())
						.ColorAndOpacity(FSlateColor(Colors::TextPrimary()))
					]
				]
				// Category tabs
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0, 0, 0, 12.0f))
				[
					BuildCategoryTabs()
				]
				// Main content: Item Grid | Details | Stats
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SHorizontalBox)
					// LEFT: Item Grid with scroll
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0, 0, 20.0f, 0))
					[
						BuildItemGrid()
					]
					// CENTER: Item Details
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(FMargin(0, 0, 20.0f, 0))
					[
						BuildDetailsPanel()
					]
					// RIGHT: Character Stats
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						BuildStatsPanel()
					]
				]
				// Controls hint at bottom
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0, 12.0f, 0, 0))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("[Q/E] Tab   [Arrows] Navigate   [Enter] Actions   [X] Drop   [I] Close")))
					.Font(Fonts::Small())
					.ColorAndOpacity(FSlateColor(Colors::TextMuted()))
				]
			]
			]
		]
		// Layer 1: Action Menu Overlay (hidden by default)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			BuildActionMenu()
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildCategoryTabs()
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	TabBorders.Empty();

	TArray<EInventoryTab> Tabs = {
		EInventoryTab::Equipped,
		EInventoryTab::All,
		EInventoryTab::Weapons,
		EInventoryTab::Armor,
		EInventoryTab::Consumables,
		EInventoryTab::Materials,
		EInventoryTab::KeyItems
	};

	TSharedRef<SHorizontalBox> TabContainer = SNew(SHorizontalBox);

	for (int32 i = 0; i < Tabs.Num(); i++)
	{
		TSharedPtr<SBorder> TabBorder;
		FLinearColor TabColor = (Tabs[i] == CurrentTab) ? Colors::AccentAmber() : Colors::BorderIron();

		TabContainer->AddSlot()
			.AutoWidth()
			.Padding(FMargin(0, 0, 4.0f, 0))
			[
				SAssignNew(TabBorder, SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(TabColor)
				.Padding(FMargin(12.0f, 6.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(GetCategoryName(Tabs[i])))
					.Font(Fonts::Small())
					.ColorAndOpacity(FSlateColor(Colors::TextPrimary()))
				]
			];

		TabBorders.Add(TabBorder);
	}

	return TabContainer;
}

TSharedRef<SWidget> UInventoryWidget::BuildEquipmentPanel()
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Initialize brushes for equipment slots
	TArray<EEquipmentSlot> AllSlots = GetEquipmentSlotOrder();
	for (EEquipmentSlot SlotType : AllSlots)
	{
		EquipSlotBrushes.Add(SlotType, FSlateBrush());
	}

	// Create vertical layout for equipment slots
	TSharedRef<SVerticalBox> EquipmentContainer = SNew(SVerticalBox);

	// Section: Armor
	EquipmentContainer->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0, 0, 0, 4.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Armor")))
			.Font(Fonts::Small())
			.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
		];

	EquipmentContainer->AddSlot().AutoHeight()[BuildEquipmentSlot(EEquipmentSlot::Helmet, TEXT("Head"))];
	EquipmentContainer->AddSlot().AutoHeight()[BuildEquipmentSlot(EEquipmentSlot::Chest, TEXT("Chest"))];
	EquipmentContainer->AddSlot().AutoHeight()[BuildEquipmentSlot(EEquipmentSlot::Gloves, TEXT("Arms"))];
	EquipmentContainer->AddSlot().AutoHeight()[BuildEquipmentSlot(EEquipmentSlot::Legs, TEXT("Legs"))];
	EquipmentContainer->AddSlot().AutoHeight()[BuildEquipmentSlot(EEquipmentSlot::Boots, TEXT("Feet"))];

	// Section: Weapons
	EquipmentContainer->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0, 8.0f, 0, 4.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Weapons")))
			.Font(Fonts::Small())
			.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
		];

	EquipmentContainer->AddSlot().AutoHeight()[BuildEquipmentSlot(EEquipmentSlot::PrimaryWeapon, TEXT("Right"))];
	EquipmentContainer->AddSlot().AutoHeight()[BuildEquipmentSlot(EEquipmentSlot::OffHand, TEXT("Left"))];

	// Section: Rings
	EquipmentContainer->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0, 8.0f, 0, 4.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Rings")))
			.Font(Fonts::Small())
			.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
		];

	EquipmentContainer->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Ring1, TEXT(""))]
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Ring2, TEXT(""))]
		];
	EquipmentContainer->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Ring3, TEXT(""))]
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Ring4, TEXT(""))]
		];

	// Section: Trinkets
	EquipmentContainer->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0, 8.0f, 0, 4.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Talismans")))
			.Font(Fonts::Small())
			.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
		];

	EquipmentContainer->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Trinket1, TEXT(""))]
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Trinket2, TEXT(""))]
		];
	EquipmentContainer->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Trinket3, TEXT(""))]
			+ SHorizontalBox::Slot().AutoWidth()[BuildEquipmentSlot(EEquipmentSlot::Trinket4, TEXT(""))]
		];

	// Wrap in a border panel
	return SNew(SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(Colors::BackgroundPanel())
		.Padding(FMargin(8.0f))
		[
			SNew(SBox)
			.WidthOverride(120.0f)
			[
				EquipmentContainer
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildEquipmentSlot(EEquipmentSlot SlotType, const FString& Label)
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const float EquipSlotSize = 40.0f;

	TSharedPtr<SBorder> SlotBorder;
	TSharedPtr<SImage> SlotIcon;

	TSharedRef<SWidget> SlotWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0, 1.0f))
		[
			SNew(SBox)
			.WidthOverride(EquipSlotSize)
			.HeightOverride(EquipSlotSize)
			[
				SNew(SOverlay)
				// Background
				+ SOverlay::Slot()
				[
					SAssignNew(SlotBorder, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(Colors::BorderIron())
					.Padding(FMargin(1.0f))
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(Colors::BackgroundSlot())
					]
				]
				// Icon
				+ SOverlay::Slot()
				.Padding(FMargin(4.0f))
				[
					SAssignNew(SlotIcon, SImage)
					.Image(&EquipSlotBrushes[SlotType])
					.Visibility(EVisibility::Collapsed)
				]
			]
		];

	// Add label if provided
	if (!Label.IsEmpty())
	{
		TSharedRef<SHorizontalBox> SlotRow = StaticCastSharedRef<SHorizontalBox>(SlotWidget);
		SlotRow->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4.0f, 0, 0, 0))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::TextMuted()))
			];
	}

	// Store references
	EquipSlotBorders.Add(SlotType, SlotBorder);
	EquipSlotIcons.Add(SlotType, SlotIcon);

	return SlotWidget;
}

TArray<EEquipmentSlot> UInventoryWidget::GetEquipmentSlotOrder() const
{
	return TArray<EEquipmentSlot>({
		EEquipmentSlot::Helmet,
		EEquipmentSlot::Chest,
		EEquipmentSlot::Gloves,
		EEquipmentSlot::Legs,
		EEquipmentSlot::Boots,
		EEquipmentSlot::PrimaryWeapon,
		EEquipmentSlot::OffHand,
		EEquipmentSlot::Ring1,
		EEquipmentSlot::Ring2,
		EEquipmentSlot::Ring3,
		EEquipmentSlot::Ring4,
		EEquipmentSlot::Trinket1,
		EEquipmentSlot::Trinket2,
		EEquipmentSlot::Trinket3,
		EEquipmentSlot::Trinket4
	});
}

TSharedRef<SWidget> UInventoryWidget::BuildItemGrid()
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	SlotBorders.Empty();
	SlotIcons.Empty();
	SlotQuantities.Empty();

	// Create scrollable item list
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Vertical);

	ItemScrollBox = ScrollBox;

	// Create uniform grid for items
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel)
		.SlotPadding(FMargin(2.0f));

	// Build slots
	for (int32 i = 0; i < GRID_COLUMNS * VISIBLE_ROWS; i++)
	{
		int32 Row = i / GRID_COLUMNS;
		int32 Col = i % GRID_COLUMNS;

		Grid->AddSlot(Col, Row)
			[
				BuildItemSlot(i)
			];
	}

	ScrollBox->AddSlot()
		[
			Grid
		];

	// Wrap in border with header
	return SNew(SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(Colors::BackgroundPanel())
		.Padding(FMargin(8.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 8.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(GetCategoryName(CurrentTab)))
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.WidthOverride((SLOT_SIZE + 4.0f) * GRID_COLUMNS)
				.HeightOverride((SLOT_SIZE + 4.0f) * VISIBLE_ROWS)
				[
					ScrollBox
				]
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildItemSlot(int32 DisplayIndex)
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	TSharedPtr<SBorder> SlotBorder;
	TSharedPtr<SImage> SlotIcon;
	TSharedPtr<STextBlock> QuantityText;
	TSharedPtr<STextBlock> EquippedBadge;

	TSharedRef<SWidget> SlotWidget = SNew(SBox)
		.WidthOverride(SLOT_SIZE)
		.HeightOverride(SLOT_SIZE)
		[
			SNew(SOverlay)
			// Background and border
			+ SOverlay::Slot()
			[
				SAssignNew(SlotBorder, SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(Colors::BorderIron())
				.Padding(FMargin(2.0f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(Colors::BackgroundSlot())
				]
			]
			// Item icon - fills the slot
			+ SOverlay::Slot()
			.Padding(FMargin(4.0f))
			[
				SAssignNew(SlotIcon, SImage)
				.Image(&SlotBrushes[DisplayIndex])
				.Visibility(EVisibility::Collapsed)
			]
			// Equipped badge top-left "E"
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(FMargin(4.0f, 2.0f, 0, 0))
			[
				SAssignNew(EquippedBadge, STextBlock)
				.Text(FText::FromString(TEXT("E")))
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::AccentAmber()))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
				.ShadowColorAndOpacity(FLinearColor::Black)
				.Visibility(EVisibility::Collapsed) // Hidden by default
			]
			// Quantity text bottom-right
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(0, 0, 4.0f, 2.0f))
			[
				SAssignNew(QuantityText, STextBlock)
				.Text(FText::GetEmpty())
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
				.ShadowColorAndOpacity(FLinearColor::Black)
			]
		];

	SlotBorders.Add(SlotBorder);
	SlotIcons.Add(SlotIcon);
	SlotQuantities.Add(QuantityText);
	SlotEquippedBadges.Add(EquippedBadge);

	return SlotWidget;
}

TSharedRef<SWidget> UInventoryWidget::BuildDetailsPanel()
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	return SNew(SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(Colors::BackgroundPanel())
		.Padding(FMargin(16.0f))
		[
			SNew(SVerticalBox)
			// Item name at top
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 4.0f))
			[
				SAssignNew(DetailItemName, STextBlock)
				.Text(FText::FromString(TEXT("Select an item")))
				.Font(Fonts::SubHeader())
				.ColorAndOpacity(FSlateColor(Colors::TextPrimary()))
			]
			// Item type
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 12.0f))
			[
				SAssignNew(DetailItemType, STextBlock)
				.Text(FText::GetEmpty())
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
			]
			// Large item icon
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(FMargin(0, 0, 0, 16.0f))
			[
				SNew(SBox)
				.WidthOverride(128.0f)
				.HeightOverride(128.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(Colors::BorderRust())
					.Padding(FMargin(4.0f))
					[
						SAssignNew(DetailItemIcon, SImage)
						.Image(&DetailIconBrush)
						.Visibility(EVisibility::Collapsed)
					]
				]
			]
			// Item effect / stats
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 8.0f))
			[
				SAssignNew(DetailItemEffect, STextBlock)
				.Text(FText::GetEmpty())
				.Font(Fonts::Body())
				.ColorAndOpacity(FSlateColor(Colors::AccentAmber()))
				.AutoWrapText(true)
			]
			// Item stats
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 12.0f))
			[
				SAssignNew(DetailItemStats, STextBlock)
				.Text(FText::GetEmpty())
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
				.AutoWrapText(true)
			]
			// Description
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(DetailItemDesc, STextBlock)
					.Text(FText::GetEmpty())
					.Font(Fonts::Small())
					.ColorAndOpacity(FSlateColor(Colors::TextMuted()))
					.AutoWrapText(true)
				]
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildStatsPanel()
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	auto BuildStatRow = [&](const FString& Label, TSharedPtr<STextBlock>& ValueText) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(ValueText, STextBlock)
				.Text(FText::FromString(TEXT("0")))
				.Font(Fonts::Small())
				.ColorAndOpacity(FSlateColor(Colors::TextPrimary()))
			];
	};

	return SNew(SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(Colors::BackgroundPanel())
		.Padding(FMargin(12.0f))
		[
			SNew(SBox)
			.WidthOverride(160.0f)
			[
				SNew(SVerticalBox)
				// Header
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0, 0, 0, 12.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Character Status")))
					.Font(Fonts::Small())
					.ColorAndOpacity(FSlateColor(Colors::TextSecondary()))
				]
				// Stats
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 2.0f))[BuildStatRow(TEXT("HP"), StatHealth)]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 2.0f))[BuildStatRow(TEXT("Stamina"), StatStamina)]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 8.0f, 0, 2.0f))[BuildStatRow(TEXT("Attack"), StatDamage)]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 2.0f))[BuildStatRow(TEXT("Defense"), StatDefense)]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 2.0f))[BuildStatRow(TEXT("Poise"), StatPoise)]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 8.0f, 0, 2.0f))[BuildStatRow(TEXT("Equip Load"), StatWeight)]
			]
		];
}

void UInventoryWidget::InitializeInventory(UInventoryComponent* InInventory, UEquipmentComponent* InEquipment)
{
	InventoryComponent = InInventory;
	EquipmentComponent = InEquipment;

	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::OnInventoryChanged);
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->OnEquipmentChanged.AddDynamic(this, &UInventoryWidget::OnEquipmentChanged);
	}

	RefreshAll();
}

void UInventoryWidget::RefreshAll()
{
	UpdateFilteredItems();
	RefreshInventoryGrid();
	RefreshEquipmentDisplay();
	RefreshEquipmentSlotIcons();
	UpdateSelectionHighlight();
	UpdateEquipmentHighlight();
	UpdateItemDetails();
	UpdateTabHighlight();
}

void UInventoryWidget::UpdateFilteredItems()
{
	FilteredSlotIndices.Empty();
	FilteredEquipSlots.Empty();

	// Handle Equipped tab specially - shows equipped items from EquipmentComponent
	if (CurrentTab == EInventoryTab::Equipped)
	{
		if (!EquipmentComponent) return;

		// Get all equipment slots that have items
		TArray<EEquipmentSlot> AllSlots = GetEquipmentSlotOrder();
		for (EEquipmentSlot EquipSlot : AllSlots)
		{
			FName EquippedID = EquipmentComponent->GetEquippedItem(EquipSlot);
			if (!EquippedID.IsNone())
			{
				FilteredEquipSlots.Add(EquipSlot);
			}
		}

		// Reset selection if out of bounds
		if (SelectedSlotIndex >= FilteredEquipSlots.Num())
		{
			SelectedSlotIndex = FMath::Max(0, FilteredEquipSlots.Num() - 1);
		}
		return;
	}

	// Normal inventory filtering
	if (!InventoryComponent) return;

	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();

	for (int32 i = 0; i < AllSlots.Num(); i++)
	{
		if (AllSlots[i].IsEmpty()) continue;

		FItemData ItemData;
		if (InventoryComponent->GetItemData(AllSlots[i].ItemID, ItemData))
		{
			if (ItemMatchesFilter(ItemData))
			{
				FilteredSlotIndices.Add(i);
			}
		}
	}

	// Reset selection if out of bounds
	if (SelectedSlotIndex >= FilteredSlotIndices.Num())
	{
		SelectedSlotIndex = FMath::Max(0, FilteredSlotIndices.Num() - 1);
	}
}

bool UInventoryWidget::ItemMatchesFilter(const FItemData& ItemData) const
{
	switch (CurrentTab)
	{
	case EInventoryTab::All:
		return true;
	case EInventoryTab::Weapons:
		return ItemData.IsWeapon();
	case EInventoryTab::Armor:
		return ItemData.IsEquipment() && !ItemData.IsWeapon();
	case EInventoryTab::Consumables:
		return ItemData.IsConsumable();
	case EInventoryTab::Materials:
		return ItemData.Category == EItemCategory::Material;
	case EInventoryTab::KeyItems:
		return ItemData.Category == EItemCategory::KeyItem || ItemData.Category == EItemCategory::Special;
	default:
		return true;
	}
}

void UInventoryWidget::RefreshInventoryGrid()
{
	using namespace COTMStyle;

	// Handle Equipped tab - show equipped items
	if (CurrentTab == EInventoryTab::Equipped)
	{
		if (!EquipmentComponent) return;

		for (int32 DisplayIdx = 0; DisplayIdx < SlotIcons.Num(); DisplayIdx++)
		{
			if (!SlotIcons.IsValidIndex(DisplayIdx) || !SlotIcons[DisplayIdx].IsValid()) continue;

			if (DisplayIdx < FilteredEquipSlots.Num())
			{
				EEquipmentSlot EquipSlot = FilteredEquipSlots[DisplayIdx];
				FName EquippedID = EquipmentComponent->GetEquippedItem(EquipSlot);

				FItemData ItemData;
				if (EquipmentComponent->GetItemData(EquippedID, ItemData))
				{
					// Load and display icon
					if (!ItemData.Icon.IsNull())
					{
						UTexture2D* IconTexture = ItemData.Icon.LoadSynchronous();
						if (IconTexture)
						{
							SlotBrushes[DisplayIdx].SetResourceObject(IconTexture);
							SlotBrushes[DisplayIdx].ImageSize = FVector2D(IconTexture->GetSizeX(), IconTexture->GetSizeY());
							SlotBrushes[DisplayIdx].DrawAs = ESlateBrushDrawType::Image;
							SlotIcons[DisplayIdx]->SetImage(&SlotBrushes[DisplayIdx]);
							SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Visible);
						}
						else
						{
							const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
							SlotBrushes[DisplayIdx] = *WhiteBrush;
							SlotBrushes[DisplayIdx].TintColor = FSlateColor(GetRarityColor(ItemData.Rarity) * 0.6f);
							SlotBrushes[DisplayIdx].DrawAs = ESlateBrushDrawType::Box;
							SlotIcons[DisplayIdx]->SetImage(&SlotBrushes[DisplayIdx]);
							SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Visible);
						}
					}
					else
					{
						const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
						SlotBrushes[DisplayIdx] = *WhiteBrush;
						SlotBrushes[DisplayIdx].TintColor = FSlateColor(GetRarityColor(ItemData.Rarity) * 0.6f);
						SlotBrushes[DisplayIdx].DrawAs = ESlateBrushDrawType::Box;
						SlotIcons[DisplayIdx]->SetImage(&SlotBrushes[DisplayIdx]);
						SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Visible);
					}

					// No quantity for equipped items
					if (SlotQuantities.IsValidIndex(DisplayIdx) && SlotQuantities[DisplayIdx].IsValid())
					{
						SlotQuantities[DisplayIdx]->SetText(FText::GetEmpty());
					}

					// Border color by rarity
					if (SlotBorders.IsValidIndex(DisplayIdx) && SlotBorders[DisplayIdx].IsValid())
					{
						SlotBorders[DisplayIdx]->SetBorderBackgroundColor(GetRarityColor(ItemData.Rarity));
					}

					// Always show equipped badge on Equipped tab
					if (SlotEquippedBadges.IsValidIndex(DisplayIdx) && SlotEquippedBadges[DisplayIdx].IsValid())
					{
						SlotEquippedBadges[DisplayIdx]->SetVisibility(EVisibility::Visible);
					}
				}
			}
			else
			{
				// Empty display slot
				SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Collapsed);
				if (SlotQuantities.IsValidIndex(DisplayIdx) && SlotQuantities[DisplayIdx].IsValid())
				{
					SlotQuantities[DisplayIdx]->SetText(FText::GetEmpty());
				}
				if (SlotBorders.IsValidIndex(DisplayIdx) && SlotBorders[DisplayIdx].IsValid())
				{
					SlotBorders[DisplayIdx]->SetBorderBackgroundColor(Colors::BorderIron());
				}
				if (SlotEquippedBadges.IsValidIndex(DisplayIdx) && SlotEquippedBadges[DisplayIdx].IsValid())
				{
					SlotEquippedBadges[DisplayIdx]->SetVisibility(EVisibility::Collapsed);
				}
			}
		}
		return;
	}

	// Normal inventory grid
	if (!InventoryComponent) return;

	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();

	for (int32 DisplayIdx = 0; DisplayIdx < SlotIcons.Num(); DisplayIdx++)
	{
		if (!SlotIcons.IsValidIndex(DisplayIdx) || !SlotIcons[DisplayIdx].IsValid()) continue;

		// Check if this display slot has a filtered item
		if (DisplayIdx < FilteredSlotIndices.Num())
		{
			int32 ActualSlotIdx = FilteredSlotIndices[DisplayIdx];
			const FInventorySlot& InvSlot = AllSlots[ActualSlotIdx];

			FItemData ItemData;
			if (InventoryComponent->GetItemData(InvSlot.ItemID, ItemData))
			{
				// Load and display icon - use IsNull() NOT IsValid()
				if (!ItemData.Icon.IsNull())
				{
					UTexture2D* IconTexture = ItemData.Icon.LoadSynchronous();
					if (IconTexture)
					{
						SlotBrushes[DisplayIdx].SetResourceObject(IconTexture);
						SlotBrushes[DisplayIdx].ImageSize = FVector2D(IconTexture->GetSizeX(), IconTexture->GetSizeY());
						SlotBrushes[DisplayIdx].DrawAs = ESlateBrushDrawType::Image;
						SlotIcons[DisplayIdx]->SetImage(&SlotBrushes[DisplayIdx]);
						SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Visible);
					}
					else
					{
						// Failed to load - show placeholder
						const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
						SlotBrushes[DisplayIdx] = *WhiteBrush;
						SlotBrushes[DisplayIdx].TintColor = FSlateColor(FLinearColor::Red * 0.5f); // Red = load failed
						SlotBrushes[DisplayIdx].DrawAs = ESlateBrushDrawType::Box;
						SlotIcons[DisplayIdx]->SetImage(&SlotBrushes[DisplayIdx]);
						SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Visible);
					}
				}
				else
				{
					// No icon path set - show colored placeholder box
					const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
					SlotBrushes[DisplayIdx] = *WhiteBrush;
					SlotBrushes[DisplayIdx].TintColor = FSlateColor(GetRarityColor(ItemData.Rarity) * 0.6f);
					SlotBrushes[DisplayIdx].DrawAs = ESlateBrushDrawType::Box;
					SlotIcons[DisplayIdx]->SetImage(&SlotBrushes[DisplayIdx]);
					SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Visible);
				}

				// Quantity
				if (InvSlot.Quantity > 1 && SlotQuantities.IsValidIndex(DisplayIdx) && SlotQuantities[DisplayIdx].IsValid())
				{
					SlotQuantities[DisplayIdx]->SetText(FText::AsNumber(InvSlot.Quantity));
				}
				else if (SlotQuantities.IsValidIndex(DisplayIdx) && SlotQuantities[DisplayIdx].IsValid())
				{
					SlotQuantities[DisplayIdx]->SetText(FText::GetEmpty());
				}

				// Border color by rarity
				if (SlotBorders.IsValidIndex(DisplayIdx) && SlotBorders[DisplayIdx].IsValid())
				{
					SlotBorders[DisplayIdx]->SetBorderBackgroundColor(GetRarityColor(ItemData.Rarity));
				}

				// Equipped badge - check if this item is equipped in ANY slot
				if (SlotEquippedBadges.IsValidIndex(DisplayIdx) && SlotEquippedBadges[DisplayIdx].IsValid() && EquipmentComponent)
				{
					bool bIsEquipped = false;
					FName ItemIDToCheck = InvSlot.ItemID;

					// Check all equipment slots (armor, weapons, rings, trinkets)
					TArray<EEquipmentSlot> AllEquipmentSlots = {
						EEquipmentSlot::Helmet, EEquipmentSlot::Chest, EEquipmentSlot::Gloves,
						EEquipmentSlot::Legs, EEquipmentSlot::Boots,
						EEquipmentSlot::PrimaryWeapon, EEquipmentSlot::OffHand,
						EEquipmentSlot::Ring1, EEquipmentSlot::Ring2, EEquipmentSlot::Ring3, EEquipmentSlot::Ring4,
						EEquipmentSlot::Trinket1, EEquipmentSlot::Trinket2, EEquipmentSlot::Trinket3, EEquipmentSlot::Trinket4
					};

					for (EEquipmentSlot EquipSlot : AllEquipmentSlots)
					{
						if (EquipmentComponent->GetEquippedItem(EquipSlot) == ItemIDToCheck)
						{
							bIsEquipped = true;
							break;
						}
					}

					SlotEquippedBadges[DisplayIdx]->SetVisibility(bIsEquipped ? EVisibility::Visible : EVisibility::Collapsed);
				}
			}
		}
		else
		{
			// Empty display slot
			SlotIcons[DisplayIdx]->SetVisibility(EVisibility::Collapsed);
			if (SlotQuantities.IsValidIndex(DisplayIdx) && SlotQuantities[DisplayIdx].IsValid())
			{
				SlotQuantities[DisplayIdx]->SetText(FText::GetEmpty());
			}
			if (SlotBorders.IsValidIndex(DisplayIdx) && SlotBorders[DisplayIdx].IsValid())
			{
				SlotBorders[DisplayIdx]->SetBorderBackgroundColor(Colors::BorderIron());
			}
			if (SlotEquippedBadges.IsValidIndex(DisplayIdx) && SlotEquippedBadges[DisplayIdx].IsValid())
			{
				SlotEquippedBadges[DisplayIdx]->SetVisibility(EVisibility::Collapsed);
			}
		}
	}
}

void UInventoryWidget::RefreshEquipmentDisplay()
{
	if (!EquipmentComponent) return;

	// Update stats panel with equipped item totals
	FItemStats TotalStats = EquipmentComponent->GetTotalEquippedStats();

	if (StatHealth.IsValid()) StatHealth->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), 100.0f + TotalStats.Health)));
	if (StatStamina.IsValid()) StatStamina->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), 100.0f + TotalStats.Stamina)));
	if (StatDamage.IsValid()) StatDamage->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), TotalStats.PhysicalDamage)));
	if (StatDefense.IsValid()) StatDefense->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), TotalStats.PhysicalDefense)));
	if (StatPoise.IsValid()) StatPoise->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), TotalStats.Poise)));
	if (StatWeight.IsValid()) StatWeight->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), TotalStats.Weight)));
}

void UInventoryWidget::UpdateSelectionHighlight()
{
	using namespace COTMStyle;

	// Reset all borders
	for (int32 i = 0; i < SlotBorders.Num(); i++)
	{
		if (!SlotBorders[i].IsValid()) continue;

		FLinearColor BorderColor = Colors::BorderIron();

		// Restore rarity color if slot has item
		if (i < FilteredSlotIndices.Num() && InventoryComponent)
		{
			TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
			int32 ActualIdx = FilteredSlotIndices[i];
			if (AllSlots.IsValidIndex(ActualIdx) && !AllSlots[ActualIdx].IsEmpty())
			{
				FItemData ItemData;
				if (InventoryComponent->GetItemData(AllSlots[ActualIdx].ItemID, ItemData))
				{
					BorderColor = GetRarityColor(ItemData.Rarity);
				}
			}
		}

		SlotBorders[i]->SetBorderBackgroundColor(BorderColor);
	}

	// Highlight selected only if inventory grid is focused (not equipment panel)
	if (!bEquipPanelFocused && SlotBorders.IsValidIndex(SelectedSlotIndex) && SlotBorders[SelectedSlotIndex].IsValid())
	{
		SlotBorders[SelectedSlotIndex]->SetBorderBackgroundColor(Colors::AccentAmber());
	}
}

void UInventoryWidget::UpdateItemDetails()
{
	// Handle Equipped tab - show equipped item details
	if (CurrentTab == EInventoryTab::Equipped && FilteredEquipSlots.IsValidIndex(SelectedSlotIndex) && EquipmentComponent)
	{
		EEquipmentSlot EquipSlot = FilteredEquipSlots[SelectedSlotIndex];
		FName EquippedItemID = EquipmentComponent->GetEquippedItem(EquipSlot);
		if (!EquippedItemID.IsNone())
		{
			FItemData ItemData;
			if (EquipmentComponent->GetItemData(EquippedItemID, ItemData))
			{
				// Show equipped item details
				if (DetailItemName.IsValid())
				{
					DetailItemName->SetText(ItemData.DisplayName);
					DetailItemName->SetColorAndOpacity(FSlateColor(GetRarityColor(ItemData.Rarity)));
				}

				if (DetailItemType.IsValid())
				{
					FString TypeStr = TEXT("Equipped");
					switch (ItemData.Category)
					{
					case EItemCategory::Equipment: TypeStr = ItemData.IsWeapon() ? TEXT("Weapon (Equipped)") : TEXT("Armor (Equipped)"); break;
					default: TypeStr = TEXT("Item (Equipped)"); break;
					}
					DetailItemType->SetText(FText::FromString(TypeStr));
				}

				// Icon
				if (DetailItemIcon.IsValid())
				{
					if (!ItemData.Icon.IsNull())
					{
						UTexture2D* IconTexture = ItemData.Icon.LoadSynchronous();
						if (IconTexture)
						{
							DetailIconBrush.SetResourceObject(IconTexture);
							DetailIconBrush.ImageSize = FVector2D(IconTexture->GetSizeX(), IconTexture->GetSizeY());
							DetailIconBrush.DrawAs = ESlateBrushDrawType::Image;
							DetailItemIcon->SetImage(&DetailIconBrush);
							DetailItemIcon->SetVisibility(EVisibility::Visible);
						}
					}
					else
					{
						const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
						DetailIconBrush = *WhiteBrush;
						DetailIconBrush.TintColor = FSlateColor(GetRarityColor(ItemData.Rarity) * 0.5f);
						DetailIconBrush.DrawAs = ESlateBrushDrawType::Box;
						DetailItemIcon->SetImage(&DetailIconBrush);
						DetailItemIcon->SetVisibility(EVisibility::Visible);
					}
				}

				// Stats
				if (DetailItemStats.IsValid())
				{
					FString StatsStr;
					if (ItemData.Stats.PhysicalDamage > 0)
						StatsStr += FString::Printf(TEXT("Attack: %.0f\n"), ItemData.Stats.PhysicalDamage);
					if (ItemData.Stats.PhysicalDefense > 0)
						StatsStr += FString::Printf(TEXT("Defense: %.0f\n"), ItemData.Stats.PhysicalDefense);
					if (ItemData.Stats.Poise > 0)
						StatsStr += FString::Printf(TEXT("Poise: %.0f\n"), ItemData.Stats.Poise);
					if (ItemData.Stats.Weight > 0)
						StatsStr += FString::Printf(TEXT("Weight: %.1f\n"), ItemData.Stats.Weight);
					DetailItemStats->SetText(FText::FromString(StatsStr));
				}

				if (DetailItemEffect.IsValid())
				{
					DetailItemEffect->SetText(FText::FromString(TEXT("[Enter] to Unequip")));
				}

				if (DetailItemDesc.IsValid())
				{
					DetailItemDesc->SetText(ItemData.Description);
				}

				return;
			}
		}

		// Empty equipment slot
		if (DetailItemName.IsValid()) DetailItemName->SetText(FText::FromString(TEXT("Empty Slot")));
		if (DetailItemType.IsValid()) DetailItemType->SetText(FText::GetEmpty());
		if (DetailItemIcon.IsValid()) DetailItemIcon->SetVisibility(EVisibility::Collapsed);
		if (DetailItemEffect.IsValid()) DetailItemEffect->SetText(FText::GetEmpty());
		if (DetailItemStats.IsValid()) DetailItemStats->SetText(FText::GetEmpty());
		if (DetailItemDesc.IsValid()) DetailItemDesc->SetText(FText::GetEmpty());
		return;
	}

	if (!InventoryComponent) return;

	// Clear if no selection
	if (FilteredSlotIndices.Num() == 0 || !FilteredSlotIndices.IsValidIndex(SelectedSlotIndex))
	{
		if (DetailItemName.IsValid()) DetailItemName->SetText(FText::FromString(TEXT("No items")));
		if (DetailItemType.IsValid()) DetailItemType->SetText(FText::GetEmpty());
		if (DetailItemIcon.IsValid()) DetailItemIcon->SetVisibility(EVisibility::Collapsed);
		if (DetailItemEffect.IsValid()) DetailItemEffect->SetText(FText::GetEmpty());
		if (DetailItemStats.IsValid()) DetailItemStats->SetText(FText::GetEmpty());
		if (DetailItemDesc.IsValid()) DetailItemDesc->SetText(FText::GetEmpty());
		return;
	}

	int32 ActualSlotIdx = FilteredSlotIndices[SelectedSlotIndex];
	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();

	if (!AllSlots.IsValidIndex(ActualSlotIdx)) return;

	const FInventorySlot& InvSlot = AllSlots[ActualSlotIdx];
	FItemData ItemData;

	if (!InventoryComponent->GetItemData(InvSlot.ItemID, ItemData)) return;

	// Name with rarity color
	if (DetailItemName.IsValid())
	{
		DetailItemName->SetText(ItemData.DisplayName);
		DetailItemName->SetColorAndOpacity(FSlateColor(GetRarityColor(ItemData.Rarity)));
	}

	// Type
	if (DetailItemType.IsValid())
	{
		FString TypeStr;
		switch (ItemData.Category)
		{
		case EItemCategory::Equipment: TypeStr = ItemData.IsWeapon() ? TEXT("Weapon") : TEXT("Armor"); break;
		case EItemCategory::Consumable: TypeStr = TEXT("Consumable"); break;
		case EItemCategory::Material: TypeStr = TEXT("Material"); break;
		case EItemCategory::KeyItem: TypeStr = TEXT("Key Item"); break;
		case EItemCategory::Special: TypeStr = TEXT("Special"); break;
		default: TypeStr = TEXT("Item"); break;
		}
		if (InvSlot.Quantity > 1)
		{
			TypeStr += FString::Printf(TEXT("  (Held: %d)"), InvSlot.Quantity);
		}
		DetailItemType->SetText(FText::FromString(TypeStr));
	}

	// Large icon - use IsNull() NOT IsValid()
	if (DetailItemIcon.IsValid())
	{
		if (!ItemData.Icon.IsNull())
		{
			UTexture2D* IconTexture = ItemData.Icon.LoadSynchronous();
			if (IconTexture)
			{
				DetailIconBrush.SetResourceObject(IconTexture);
				DetailIconBrush.ImageSize = FVector2D(IconTexture->GetSizeX(), IconTexture->GetSizeY());
				DetailIconBrush.DrawAs = ESlateBrushDrawType::Image;
				DetailItemIcon->SetImage(&DetailIconBrush);
				DetailItemIcon->SetVisibility(EVisibility::Visible);
			}
			else
			{
				// Load failed
				const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
				DetailIconBrush = *WhiteBrush;
				DetailIconBrush.TintColor = FSlateColor(FLinearColor::Red * 0.5f);
				DetailIconBrush.DrawAs = ESlateBrushDrawType::Box;
				DetailItemIcon->SetImage(&DetailIconBrush);
				DetailItemIcon->SetVisibility(EVisibility::Visible);
			}
		}
		else
		{
			// No icon path - show colored placeholder
			const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
			DetailIconBrush = *WhiteBrush;
			DetailIconBrush.TintColor = FSlateColor(GetRarityColor(ItemData.Rarity) * 0.5f);
			DetailIconBrush.DrawAs = ESlateBrushDrawType::Box;
			DetailItemIcon->SetImage(&DetailIconBrush);
			DetailItemIcon->SetVisibility(EVisibility::Visible);
		}
	}

	// Effect (for consumables)
	if (DetailItemEffect.IsValid())
	{
		FString EffectStr;
		if (ItemData.IsConsumable())
		{
			if (ItemData.ConsumableEffect.HealthRestore > 0)
				EffectStr += FString::Printf(TEXT("Restores %.0f HP\n"), ItemData.ConsumableEffect.HealthRestore);
			if (ItemData.ConsumableEffect.StaminaRestore > 0)
				EffectStr += FString::Printf(TEXT("Restores %.0f Stamina\n"), ItemData.ConsumableEffect.StaminaRestore);
		}
		DetailItemEffect->SetText(FText::FromString(EffectStr));
	}

	// Stats
	if (DetailItemStats.IsValid())
	{
		FString StatsStr;
		if (ItemData.Stats.PhysicalDamage > 0)
			StatsStr += FString::Printf(TEXT("Attack: %.0f\n"), ItemData.Stats.PhysicalDamage);
		if (ItemData.Stats.PhysicalDefense > 0)
			StatsStr += FString::Printf(TEXT("Defense: %.0f\n"), ItemData.Stats.PhysicalDefense);
		if (ItemData.Stats.Poise > 0)
			StatsStr += FString::Printf(TEXT("Poise: %.0f\n"), ItemData.Stats.Poise);
		if (ItemData.Stats.Weight > 0)
			StatsStr += FString::Printf(TEXT("Weight: %.1f\n"), ItemData.Stats.Weight);
		DetailItemStats->SetText(FText::FromString(StatsStr));
	}

	// Description
	if (DetailItemDesc.IsValid())
	{
		DetailItemDesc->SetText(ItemData.Description);
	}
}

void UInventoryWidget::UpdateTabHighlight()
{
	using namespace COTMStyle;

	TArray<EInventoryTab> Tabs = {
		EInventoryTab::Equipped,
		EInventoryTab::All,
		EInventoryTab::Weapons,
		EInventoryTab::Armor,
		EInventoryTab::Consumables,
		EInventoryTab::Materials,
		EInventoryTab::KeyItems
	};

	for (int32 i = 0; i < TabBorders.Num() && i < Tabs.Num(); i++)
	{
		if (TabBorders[i].IsValid())
		{
			FLinearColor Color = (Tabs[i] == CurrentTab) ? Colors::AccentAmber() : Colors::BorderIron();
			TabBorders[i]->SetBorderBackgroundColor(Color);
		}
	}
}

void UInventoryWidget::RefreshEquipmentSlotIcons()
{
	using namespace COTMStyle;

	if (!EquipmentComponent) return;

	TArray<EEquipmentSlot> AllSlots = GetEquipmentSlotOrder();

	for (EEquipmentSlot SlotType : AllSlots)
	{
		TSharedPtr<SImage>* IconPtr = EquipSlotIcons.Find(SlotType);
		TSharedPtr<SBorder>* BorderPtr = EquipSlotBorders.Find(SlotType);
		FSlateBrush* BrushPtr = EquipSlotBrushes.Find(SlotType);

		if (!IconPtr || !IconPtr->IsValid() || !BrushPtr) continue;

		FName EquippedItemID = EquipmentComponent->GetEquippedItem(SlotType);

		if (!EquippedItemID.IsNone())
		{
			FItemData ItemData;
			if (EquipmentComponent->GetItemData(EquippedItemID, ItemData))
			{
				// Load icon
				if (!ItemData.Icon.IsNull())
				{
					UTexture2D* IconTexture = ItemData.Icon.LoadSynchronous();
					if (IconTexture)
					{
						BrushPtr->SetResourceObject(IconTexture);
						BrushPtr->ImageSize = FVector2D(IconTexture->GetSizeX(), IconTexture->GetSizeY());
						BrushPtr->DrawAs = ESlateBrushDrawType::Image;
						(*IconPtr)->SetImage(BrushPtr);
						(*IconPtr)->SetVisibility(EVisibility::Visible);
					}
					else
					{
						(*IconPtr)->SetVisibility(EVisibility::Collapsed);
					}
				}
				else
				{
					// No icon - show colored placeholder
					const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
					*BrushPtr = *WhiteBrush;
					BrushPtr->TintColor = FSlateColor(GetRarityColor(ItemData.Rarity) * 0.6f);
					BrushPtr->DrawAs = ESlateBrushDrawType::Box;
					(*IconPtr)->SetImage(BrushPtr);
					(*IconPtr)->SetVisibility(EVisibility::Visible);
				}

				// Set border color by rarity
				if (BorderPtr && BorderPtr->IsValid())
				{
					(*BorderPtr)->SetBorderBackgroundColor(GetRarityColor(ItemData.Rarity));
				}
			}
		}
		else
		{
			// Empty slot
			(*IconPtr)->SetVisibility(EVisibility::Collapsed);
			if (BorderPtr && BorderPtr->IsValid())
			{
				(*BorderPtr)->SetBorderBackgroundColor(Colors::BorderIron());
			}
		}
	}

	// Update selection highlight
	UpdateEquipmentHighlight();
}

void UInventoryWidget::UpdateEquipmentHighlight()
{
	using namespace COTMStyle;

	TArray<EEquipmentSlot> AllSlots = GetEquipmentSlotOrder();

	for (EEquipmentSlot SlotType : AllSlots)
	{
		TSharedPtr<SBorder>* BorderPtr = EquipSlotBorders.Find(SlotType);
		if (!BorderPtr || !BorderPtr->IsValid()) continue;

		// If focused on equipment panel and this is selected slot, highlight amber
		if (bEquipPanelFocused && SlotType == SelectedEquipSlot)
		{
			(*BorderPtr)->SetBorderBackgroundColor(Colors::AccentAmber());
		}
		else
		{
			// Otherwise use rarity color or default
			FName EquippedItemID = EquipmentComponent ? EquipmentComponent->GetEquippedItem(SlotType) : NAME_None;
			if (!EquippedItemID.IsNone())
			{
				FItemData ItemData;
				if (EquipmentComponent->GetItemData(EquippedItemID, ItemData))
				{
					(*BorderPtr)->SetBorderBackgroundColor(GetRarityColor(ItemData.Rarity));
				}
				else
				{
					(*BorderPtr)->SetBorderBackgroundColor(Colors::BorderIron());
				}
			}
			else
			{
				(*BorderPtr)->SetBorderBackgroundColor(Colors::BorderIron());
			}
		}
	}
}

void UInventoryWidget::NavigateEquipmentSlot(int32 Delta)
{
	TArray<EEquipmentSlot> SlotOrder = GetEquipmentSlotOrder();
	if (SlotOrder.Num() == 0) return;

	int32 CurrentIndex = SlotOrder.IndexOfByKey(SelectedEquipSlot);
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = 0;
	}

	int32 NewIndex = FMath::Clamp(CurrentIndex + Delta, 0, SlotOrder.Num() - 1);
	SelectedEquipSlot = SlotOrder[NewIndex];

	UpdateEquipmentHighlight();
	UpdateItemDetails(); // Show details of equipped item
}

void UInventoryWidget::SwitchFocusPanel()
{
	bEquipPanelFocused = !bEquipPanelFocused;

	if (bEquipPanelFocused)
	{
		// Switching to equipment panel - select first slot if none selected
		if (SelectedEquipSlot == EEquipmentSlot::None)
		{
			TArray<EEquipmentSlot> SlotOrder = GetEquipmentSlotOrder();
			if (SlotOrder.Num() > 0)
			{
				SelectedEquipSlot = SlotOrder[0];
			}
		}
	}

	UpdateEquipmentHighlight();
	UpdateSelectionHighlight();
}

FReply UInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FKey Key = InKeyEvent.GetKey();

	// Close
	if (Key == EKeys::I || Key == EKeys::Escape)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return FReply::Handled();
	}

	// Navigate up (arrow keys only)
	if (Key == EKeys::Up)
	{
		NavigateSelection(-GRID_COLUMNS);
		return FReply::Handled();
	}

	// Navigate down
	if (Key == EKeys::Down)
	{
		NavigateSelection(GRID_COLUMNS);
		return FReply::Handled();
	}

	// Navigate left
	if (Key == EKeys::Left)
	{
		NavigateSelection(-1);
		return FReply::Handled();
	}

	// Navigate right
	if (Key == EKeys::Right)
	{
		NavigateSelection(1);
		return FReply::Handled();
	}

	// Previous tab
	if (Key == EKeys::Q)
	{
		CycleTab(-1);
		return FReply::Handled();
	}

	// Next tab
	if (Key == EKeys::E)
	{
		CycleTab(1);
		return FReply::Handled();
	}

	// Equip/Use
	if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		TryEquipSelected();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void UInventoryWidget::NavigateSelection(int32 Delta)
{
	// Get the correct item count based on current tab
	int32 ItemCount = (CurrentTab == EInventoryTab::Equipped) ? FilteredEquipSlots.Num() : FilteredSlotIndices.Num();
	if (ItemCount == 0) return;

	int32 NewSelection = SelectedSlotIndex + Delta;

	// Handle row wrapping
	int32 CurrentRow = SelectedSlotIndex / GRID_COLUMNS;
	int32 NewRow = NewSelection / GRID_COLUMNS;
	int32 CurrentCol = SelectedSlotIndex % GRID_COLUMNS;
	int32 NewCol = NewSelection % GRID_COLUMNS;

	// Clamp to valid range
	NewSelection = FMath::Clamp(NewSelection, 0, FMath::Max(0, ItemCount - 1));

	if (NewSelection != SelectedSlotIndex)
	{
		SelectedSlotIndex = NewSelection;
		UpdateSelectionHighlight();
		UpdateItemDetails();
	}
}

void UInventoryWidget::CycleTab(int32 Direction)
{
	TArray<EInventoryTab> Tabs = {
		EInventoryTab::Equipped,
		EInventoryTab::All,
		EInventoryTab::Weapons,
		EInventoryTab::Armor,
		EInventoryTab::Consumables,
		EInventoryTab::Materials,
		EInventoryTab::KeyItems
	};

	int32 CurrentIdx = Tabs.IndexOfByKey(CurrentTab);
	int32 NewIdx = (CurrentIdx + Direction + Tabs.Num()) % Tabs.Num();

	CurrentTab = Tabs[NewIdx];
	SelectedSlotIndex = 0;

	RefreshAll();
}

void UInventoryWidget::TryEquipSelected()
{
	if (!InventoryComponent || !EquipmentComponent)
	{
		return;
	}
	if (!FilteredSlotIndices.IsValidIndex(SelectedSlotIndex))
	{
		return;
	}

	int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();

	if (!AllSlots.IsValidIndex(ActualIdx))
	{
		return;
	}

	FName ItemID = AllSlots[ActualIdx].ItemID;
	FItemData ItemData;
	if (!InventoryComponent->GetItemData(ItemID, ItemData))
	{
		return;
	}

	// Equipment - equip it
	if (ItemData.IsEquipment())
	{
		if (EquipmentComponent->EquipItem(ItemID))
		{
			RefreshAll();
		}
	}
	// Consumable - assign to hotbar
	else if (ItemData.IsConsumable())
	{
		EquipmentComponent->AssignToHotbar(ItemID, EHotbarSlot::Consumable);
	}
	// Key/Special - assign to special slot
	else if (ItemData.Category == EItemCategory::KeyItem || ItemData.Category == EItemCategory::Special)
	{
		EquipmentComponent->AssignToHotbar(ItemID, EHotbarSlot::Special);
	}
}

void UInventoryWidget::TryUseSelected()
{
	// For consumables - same as equip for now
	TryEquipSelected();
}

void UInventoryWidget::OnInventoryChanged()
{
	RefreshAll();
}

void UInventoryWidget::OnEquipmentChanged(EEquipmentSlot SlotType, FName NewItemID)
{
	RefreshAll();
}

FLinearColor UInventoryWidget::GetRarityColor(EItemRarity Rarity)
{
	using namespace COTMStyle;

	switch (Rarity)
	{
	case EItemRarity::Common:    return Colors::RarityCommon();
	case EItemRarity::Uncommon:  return Colors::RarityUncommon();
	case EItemRarity::Rare:      return Colors::RarityRare();
	case EItemRarity::Epic:      return Colors::RarityEpic();
	case EItemRarity::Legendary: return Colors::RarityLegendary();
	default:                     return FLinearColor::White;
	}
}

FString UInventoryWidget::GetCategoryName(EInventoryTab Tab)
{
	switch (Tab)
	{
	case EInventoryTab::Equipped:    return TEXT("Equipped");
	case EInventoryTab::All:         return TEXT("All");
	case EInventoryTab::Weapons:     return TEXT("Weapons");
	case EInventoryTab::Armor:       return TEXT("Armor");
	case EInventoryTab::Consumables: return TEXT("Consumables");
	case EInventoryTab::Materials:   return TEXT("Materials");
	case EInventoryTab::KeyItems:    return TEXT("Key Items");
	default:                         return TEXT("Items");
	}
}

// ============================================================================
// ACTION MENU FUNCTIONS
// ============================================================================

TSharedRef<SWidget> UInventoryWidget::BuildActionMenu()
{
	using namespace COTMStyle;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	ActionOptionBorders.Empty();
	ActionOptionTexts.Empty();

	// Build the container for action options (will be populated dynamically)
	TSharedRef<SVerticalBox> OptionsContainer = SNew(SVerticalBox);
	ActionMenuContainer = OptionsContainer;

	// Create 6 option slots (max: Equip/Unequip, Use, Drop, Drop All, Split, Cancel)
	for (int32 i = 0; i < 6; i++)
	{
		TSharedPtr<SBorder> OptionBorder;
		TSharedPtr<STextBlock> OptionText;

		OptionsContainer->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0, 2.0f))
			[
				SAssignNew(OptionBorder, SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(Colors::BorderIron())
				.Padding(FMargin(16.0f, 8.0f))
				.Visibility(EVisibility::Collapsed) // Hidden by default
				[
					SAssignNew(OptionText, STextBlock)
					.Text(FText::GetEmpty())
					.Font(Fonts::Body())
					.ColorAndOpacity(FSlateColor(Colors::TextPrimary()))
				]
			];

		ActionOptionBorders.Add(OptionBorder);
		ActionOptionTexts.Add(OptionText);
	}

	// The popup panel
	return SAssignNew(ActionMenuPanel, SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.95f))
		.Padding(FMargin(4.0f))
		.Visibility(EVisibility::Collapsed) // Hidden by default
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(Colors::AccentAmber())
			.Padding(FMargin(2.0f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 1.0f))
				.Padding(FMargin(8.0f))
				[
					SNew(SBox)
					.MinDesiredWidth(150.0f)
					[
						OptionsContainer
					]
				]
			]
		];
}

void UInventoryWidget::ShowActionMenu()
{
	// Check correct list based on current tab
	if (CurrentTab == EInventoryTab::Equipped)
	{
		if (FilteredEquipSlots.Num() == 0 || !FilteredEquipSlots.IsValidIndex(SelectedSlotIndex))
		{
			return;
		}
	}
	else
	{
		if (FilteredSlotIndices.Num() == 0 || !FilteredSlotIndices.IsValidIndex(SelectedSlotIndex))
		{
			return;
		}
	}

	// Populate options based on selected item
	PopulateActionOptions();

	if (CurrentActionOptions.Num() == 0)
	{
		return;
	}

	bActionMenuOpen = true;
	ActionMenuSelection = 0;

	// Show the panel
	if (ActionMenuPanel.IsValid())
	{
		ActionMenuPanel->SetVisibility(EVisibility::Visible);
	}

	UpdateActionMenuHighlight();
}

void UInventoryWidget::HideActionMenu()
{
	bActionMenuOpen = false;
	ActionMenuSelection = 0;

	if (ActionMenuPanel.IsValid())
	{
		ActionMenuPanel->SetVisibility(EVisibility::Collapsed);
	}

	// Hide all options
	for (auto& Border : ActionOptionBorders)
	{
		if (Border.IsValid())
		{
			Border->SetVisibility(EVisibility::Collapsed);
		}
	}
}

void UInventoryWidget::PopulateActionOptions()
{
	CurrentActionOptions.Empty();

	// Handle Equipped tab - simple unequip only
	if (CurrentTab == EInventoryTab::Equipped)
	{
		if (!EquipmentComponent) return;
		if (!FilteredEquipSlots.IsValidIndex(SelectedSlotIndex)) return;

		CurrentActionOptions.Add(TEXT("Unequip"));
		CurrentActionOptions.Add(TEXT("Cancel"));
	}
	else
	{
		// Normal inventory tabs
		if (!InventoryComponent) return;
		if (!FilteredSlotIndices.IsValidIndex(SelectedSlotIndex)) return;

		int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
		TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
		if (!AllSlots.IsValidIndex(ActualIdx)) return;

		const FInventorySlot& InvSlot = AllSlots[ActualIdx];
		FItemData ItemData;
		if (!InventoryComponent->GetItemData(InvSlot.ItemID, ItemData)) return;

		// Build options based on item type
		if (ItemData.IsEquipment())
		{
			if (IsSelectedItemEquipped())
			{
				CurrentActionOptions.Add(TEXT("Unequip"));
			}
			else
			{
				CurrentActionOptions.Add(TEXT("Equip"));
			}
		}

		if (ItemData.IsConsumable())
		{
			CurrentActionOptions.Add(TEXT("Use"));
			CurrentActionOptions.Add(TEXT("Assign to Hotbar"));
		}

		// Drop options
		CurrentActionOptions.Add(TEXT("Drop"));
		if (InvSlot.Quantity > 1)
		{
			CurrentActionOptions.Add(TEXT("Drop All"));
			CurrentActionOptions.Add(TEXT("Split Stack"));
		}

		CurrentActionOptions.Add(TEXT("Cancel"));
	}

	// Update the UI
	for (int32 i = 0; i < ActionOptionBorders.Num(); i++)
	{
		if (i < CurrentActionOptions.Num())
		{
			if (ActionOptionBorders[i].IsValid())
			{
				ActionOptionBorders[i]->SetVisibility(EVisibility::Visible);
			}
			if (ActionOptionTexts[i].IsValid())
			{
				ActionOptionTexts[i]->SetText(FText::FromString(CurrentActionOptions[i]));
			}
		}
		else
		{
			if (ActionOptionBorders[i].IsValid())
			{
				ActionOptionBorders[i]->SetVisibility(EVisibility::Collapsed);
			}
		}
	}
}

void UInventoryWidget::NavigateActionMenu(int32 Delta)
{
	if (CurrentActionOptions.Num() == 0) return;

	ActionMenuSelection = (ActionMenuSelection + Delta + CurrentActionOptions.Num()) % CurrentActionOptions.Num();
	UpdateActionMenuHighlight();
}

void UInventoryWidget::UpdateActionMenuHighlight()
{
	using namespace COTMStyle;

	for (int32 i = 0; i < ActionOptionBorders.Num(); i++)
	{
		if (!ActionOptionBorders[i].IsValid()) continue;

		if (i == ActionMenuSelection)
		{
			ActionOptionBorders[i]->SetBorderBackgroundColor(Colors::AccentAmber());
		}
		else
		{
			ActionOptionBorders[i]->SetBorderBackgroundColor(Colors::BorderIron());
		}
	}
}

void UInventoryWidget::ExecuteSelectedAction()
{
	if (!CurrentActionOptions.IsValidIndex(ActionMenuSelection))
	{
		HideActionMenu();
		return;
	}

	FString Action = CurrentActionOptions[ActionMenuSelection];

	if (Action == TEXT("Equip"))
	{
		TryEquipSelected();
	}
	else if (Action == TEXT("Unequip"))
	{
		TryUnequipSelected();
	}
	else if (Action == TEXT("Use"))
	{
		TryUseSelected();
	}
	else if (Action == TEXT("Assign to Hotbar"))
	{
		TryEquipSelected(); // Uses same hotbar assignment logic
	}
	else if (Action == TEXT("Drop"))
	{
		TryDropSelected(1);
	}
	else if (Action == TEXT("Drop All"))
	{
		if (InventoryComponent && FilteredSlotIndices.IsValidIndex(SelectedSlotIndex))
		{
			int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
			TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
			if (AllSlots.IsValidIndex(ActualIdx))
			{
				TryDropSelected(AllSlots[ActualIdx].Quantity);
			}
		}
	}
	else if (Action == TEXT("Split Stack"))
	{
		TrySplitSelected();
	}
	// Cancel or unknown - just close

	HideActionMenu();
}

bool UInventoryWidget::IsSelectedItemEquipped() const
{
	if (!EquipmentComponent || !InventoryComponent) return false;
	if (!FilteredSlotIndices.IsValidIndex(SelectedSlotIndex)) return false;

	int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
	if (!AllSlots.IsValidIndex(ActualIdx)) return false;

	FName ItemID = AllSlots[ActualIdx].ItemID;

	// Check ALL equipment slots (armor, weapons, rings, trinkets)
	TArray<EEquipmentSlot> AllEquipmentSlots = {
		EEquipmentSlot::Helmet, EEquipmentSlot::Chest, EEquipmentSlot::Gloves,
		EEquipmentSlot::Legs, EEquipmentSlot::Boots,
		EEquipmentSlot::PrimaryWeapon, EEquipmentSlot::OffHand,
		EEquipmentSlot::Ring1, EEquipmentSlot::Ring2, EEquipmentSlot::Ring3, EEquipmentSlot::Ring4,
		EEquipmentSlot::Trinket1, EEquipmentSlot::Trinket2, EEquipmentSlot::Trinket3, EEquipmentSlot::Trinket4
	};

	for (EEquipmentSlot EquipSlot : AllEquipmentSlots)
	{
		if (EquipmentComponent->GetEquippedItem(EquipSlot) == ItemID)
		{
			return true;
		}
	}

	return false;
}

bool UInventoryWidget::IsShiftHeld() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return false;

	return PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
}

void UInventoryWidget::TryUnequipSelected()
{
	if (!EquipmentComponent) return;

	// Handle Equipped tab - directly unequip from selected slot
	if (CurrentTab == EInventoryTab::Equipped)
	{
		if (!FilteredEquipSlots.IsValidIndex(SelectedSlotIndex)) return;

		EEquipmentSlot EquipSlot = FilteredEquipSlots[SelectedSlotIndex];
		if (EquipmentComponent->UnequipSlot(EquipSlot))
		{
			RefreshAll();
		}
		return;
	}

	// Normal inventory - find equipped item
	if (!InventoryComponent) return;
	if (!FilteredSlotIndices.IsValidIndex(SelectedSlotIndex)) return;

	int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
	if (!AllSlots.IsValidIndex(ActualIdx)) return;

	FName ItemID = AllSlots[ActualIdx].ItemID;
	FItemData ItemData;
	if (!InventoryComponent->GetItemData(ItemID, ItemData)) return;

	// Find which slot it's equipped in and unequip (check ALL slots)
	TArray<EEquipmentSlot> SlotsToCheck = {
		EEquipmentSlot::Helmet, EEquipmentSlot::Chest, EEquipmentSlot::Gloves,
		EEquipmentSlot::Legs, EEquipmentSlot::Boots,
		EEquipmentSlot::PrimaryWeapon, EEquipmentSlot::OffHand,
		EEquipmentSlot::Ring1, EEquipmentSlot::Ring2, EEquipmentSlot::Ring3, EEquipmentSlot::Ring4,
		EEquipmentSlot::Trinket1, EEquipmentSlot::Trinket2, EEquipmentSlot::Trinket3, EEquipmentSlot::Trinket4
	};

	for (EEquipmentSlot EquipSlot : SlotsToCheck)
	{
		if (EquipmentComponent->GetEquippedItem(EquipSlot) == ItemID)
		{
			EquipmentComponent->UnequipSlot(EquipSlot);
			RefreshAll();
			return;
		}
	}
}

void UInventoryWidget::TryDropSelected(int32 Quantity)
{
	if (!InventoryComponent) return;
	if (!FilteredSlotIndices.IsValidIndex(SelectedSlotIndex)) return;

	int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
	if (!AllSlots.IsValidIndex(ActualIdx)) return;

	const FInventorySlot& InvSlot = AllSlots[ActualIdx];
	FName ItemID = InvSlot.ItemID;
	int32 ActualQty = FMath::Min(Quantity, InvSlot.Quantity);

	// If item is equipped, unequip first
	if (IsSelectedItemEquipped())
	{
		TryUnequipSelected();
	}

	// Drop the item (this spawns a pickup in the world)
	if (InventoryComponent->DropItem(ItemID, ActualQty))
	{
		RefreshAll();
	}
}

void UInventoryWidget::TrySplitSelected()
{
	if (!InventoryComponent) return;
	if (!FilteredSlotIndices.IsValidIndex(SelectedSlotIndex)) return;

	int32 ActualIdx = FilteredSlotIndices[SelectedSlotIndex];
	TArray<FInventorySlot> AllSlots = InventoryComponent->GetAllSlots();
	if (!AllSlots.IsValidIndex(ActualIdx)) return;

	const FInventorySlot& InvSlot = AllSlots[ActualIdx];

	if (InvSlot.Quantity <= 1)
	{
		return;
	}

	// Split half the stack into a new slot
	int32 SplitAmount = InvSlot.Quantity / 2;
	FName SplitItemID = InvSlot.ItemID;

	// Find an empty slot
	int32 EmptySlotIdx = -1;
	for (int32 i = 0; i < AllSlots.Num(); i++)
	{
		if (AllSlots[i].IsEmpty())
		{
			EmptySlotIdx = i;
			break;
		}
	}

	if (EmptySlotIdx == -1)
	{
		return;
	}

	// Remove from current stack and add to new slot
	if (InventoryComponent->RemoveItemAtSlot(ActualIdx, SplitAmount))
	{
		InventoryComponent->AddItem(SplitItemID, SplitAmount);
		RefreshAll();
	}
}
