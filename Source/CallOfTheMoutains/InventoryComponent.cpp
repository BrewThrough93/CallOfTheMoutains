// CallOfTheMoutains - Inventory Component Implementation

#include "InventoryComponent.h"
#include "ItemPickup.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// NOTE: Do NOT use ConstructorHelpers here - causes circular dependency crash
	// ItemDataTable will be loaded in BeginPlay
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize inventory slots
	InventorySlots.SetNum(MaxSlots);

	// Load ItemDataTable at runtime if not set
	if (!ItemDataTable)
	{
		ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/BluePrints/Data/ItemData"));
	}

	// Check DataTable status
	if (!ItemDataTable)
	{
		if (bDebugMode)
		{
			CreateDebugDataTable();
			AddDebugItems();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: ItemDataTable is not set and could not be loaded. Inventory features may not work correctly."));
		}
	}
}

void UInventoryComponent::CreateDebugDataTable()
{
	// Create runtime DataTable
	ItemDataTable = NewObject<UDataTable>(this, TEXT("DebugItemDataTable"));
	ItemDataTable->RowStruct = FItemData::StaticStruct();

	// Add test items to the DataTable
	auto AddTestItem = [this](const FName& RowName, const FItemData& Data) {
		ItemDataTable->AddRow(RowName, Data);
	};

	// === Test Sword ===
	FItemData Sword;
	Sword.ItemID = FName("TestSword");
	Sword.DisplayName = FText::FromString("Iron Longsword");
	Sword.Description = FText::FromString("A reliable longsword forged from iron. Standard issue for kingdom soldiers.");
	Sword.Category = EItemCategory::Equipment;
	Sword.EquipmentSlot = EEquipmentSlot::PrimaryWeapon;
	Sword.WeaponType = EWeaponType::Sword;
	Sword.Rarity = EItemRarity::Common;
	Sword.Stats.PhysicalDamage = 25.0f;
	Sword.Stats.Weight = 4.0f;
	Sword.MaxStackSize = 1;
	AddTestItem(FName("TestSword"), Sword);

	// === Test Shield ===
	FItemData Shield;
	Shield.ItemID = FName("TestShield");
	Shield.DisplayName = FText::FromString("Wooden Shield");
	Shield.Description = FText::FromString("A basic wooden shield. Better than nothing.");
	Shield.Category = EItemCategory::Equipment;
	Shield.EquipmentSlot = EEquipmentSlot::OffHand;
	Shield.WeaponType = EWeaponType::Shield;
	Shield.Rarity = EItemRarity::Common;
	Shield.Stats.PhysicalDefense = 15.0f;
	Shield.Stats.Poise = 10.0f;
	Shield.Stats.Weight = 3.0f;
	Shield.MaxStackSize = 1;
	AddTestItem(FName("TestShield"), Shield);

	// === Test Helmet ===
	FItemData Helmet;
	Helmet.ItemID = FName("TestHelmet");
	Helmet.DisplayName = FText::FromString("Iron Helm");
	Helmet.Description = FText::FromString("An iron helmet offering basic head protection.");
	Helmet.Category = EItemCategory::Equipment;
	Helmet.EquipmentSlot = EEquipmentSlot::Helmet;
	Helmet.Rarity = EItemRarity::Common;
	Helmet.Stats.PhysicalDefense = 8.0f;
	Helmet.Stats.Poise = 5.0f;
	Helmet.Stats.Weight = 2.0f;
	Helmet.MaxStackSize = 1;
	AddTestItem(FName("TestHelmet"), Helmet);

	// === Health Potion ===
	FItemData HealthPotion;
	HealthPotion.ItemID = FName("HealthPotion");
	HealthPotion.DisplayName = FText::FromString("Health Potion");
	HealthPotion.Description = FText::FromString("A warm golden flask. Restores health when consumed.");
	HealthPotion.Category = EItemCategory::Consumable;
	HealthPotion.Rarity = EItemRarity::Rare;
	HealthPotion.ConsumableEffect.HealthRestore = 50.0f;
	HealthPotion.ConsumableEffect.bIsInstant = true;
	HealthPotion.MaxStackSize = 10;
	AddTestItem(FName("HealthPotion"), HealthPotion);

	// === Stamina Herb ===
	FItemData StaminaHerb;
	StaminaHerb.ItemID = FName("StaminaHerb");
	StaminaHerb.DisplayName = FText::FromString("Green Blossom");
	StaminaHerb.Description = FText::FromString("A fragrant herb. Temporarily boosts stamina recovery.");
	StaminaHerb.Category = EItemCategory::Consumable;
	StaminaHerb.Rarity = EItemRarity::Uncommon;
	StaminaHerb.ConsumableEffect.StaminaRestore = 30.0f;
	StaminaHerb.ConsumableEffect.Duration = 60.0f;
	StaminaHerb.ConsumableEffect.bIsInstant = false;
	StaminaHerb.MaxStackSize = 20;
	AddTestItem(FName("StaminaHerb"), StaminaHerb);

	// === Key Item ===
	FItemData RustyKey;
	RustyKey.ItemID = FName("RustyKey");
	RustyKey.DisplayName = FText::FromString("Rusty Key");
	RustyKey.Description = FText::FromString("An old rusty key. Might open something important.");
	RustyKey.Category = EItemCategory::KeyItem;
	RustyKey.Rarity = EItemRarity::Rare;
	RustyKey.bIsKeyItem = true;
	RustyKey.bCanDrop = false;
	RustyKey.MaxStackSize = 1;
	AddTestItem(FName("RustyKey"), RustyKey);

	// === Epic Greatsword ===
	FItemData Greatsword;
	Greatsword.ItemID = FName("FlameGreatsword");
	Greatsword.DisplayName = FText::FromString("Flamberge");
	Greatsword.Description = FText::FromString("A massive greatsword wreathed in flame. Requires great strength to wield.");
	Greatsword.Category = EItemCategory::Equipment;
	Greatsword.EquipmentSlot = EEquipmentSlot::PrimaryWeapon;
	Greatsword.WeaponType = EWeaponType::Greatsword;
	Greatsword.Rarity = EItemRarity::Epic;
	Greatsword.Stats.PhysicalDamage = 55.0f;
	Greatsword.Stats.Weight = 12.0f;
	Greatsword.MaxStackSize = 1;
	AddTestItem(FName("FlameGreatsword"), Greatsword);
}

void UInventoryComponent::AddDebugItems()
{
	// Add some items to start with
	AddItem(FName("HealthPotion"), 5);
	AddItem(FName("StaminaHerb"), 3);
	AddItem(FName("TestSword"), 1);
	AddItem(FName("TestShield"), 1);
	AddItem(FName("TestHelmet"), 1);
}

int32 UInventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	FItemData ItemData;
	if (!GetItemData(ItemID, ItemData))
	{
		return 0;
	}

	int32 RemainingToAdd = Quantity;
	int32 TotalAdded = 0;

	// First, try to stack with existing items
	if (ItemData.IsStackable())
	{
		while (RemainingToAdd > 0)
		{
			int32 StackableSlot = FindStackableSlot(ItemID, ItemData);
			if (StackableSlot == INDEX_NONE)
			{
				break;
			}

			int32 SpaceInSlot = ItemData.MaxStackSize - InventorySlots[StackableSlot].Quantity;
			int32 ToAdd = FMath::Min(RemainingToAdd, SpaceInSlot);

			InventorySlots[StackableSlot].Quantity += ToAdd;
			RemainingToAdd -= ToAdd;
			TotalAdded += ToAdd;
		}
	}

	// Then, add to empty slots
	while (RemainingToAdd > 0)
	{
		int32 EmptySlot = FindEmptySlot();
		if (EmptySlot == INDEX_NONE)
		{
			break; // Inventory full
		}

		int32 ToAdd = FMath::Min(RemainingToAdd, ItemData.MaxStackSize);

		InventorySlots[EmptySlot].ItemID = ItemID;
		InventorySlots[EmptySlot].Quantity = ToAdd;
		RemainingToAdd -= ToAdd;
		TotalAdded += ToAdd;
	}

	if (TotalAdded > 0)
	{
		OnItemAdded.Broadcast(ItemID, TotalAdded);
		OnInventoryChanged.Broadcast();
	}

	return TotalAdded;
}

int32 UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	int32 RemainingToRemove = Quantity;
	int32 TotalRemoved = 0;

	// Remove from all slots containing this item
	for (int32 i = 0; i < InventorySlots.Num() && RemainingToRemove > 0; ++i)
	{
		if (InventorySlots[i].ItemID == ItemID)
		{
			int32 ToRemove = FMath::Min(RemainingToRemove, InventorySlots[i].Quantity);

			InventorySlots[i].Quantity -= ToRemove;
			RemainingToRemove -= ToRemove;
			TotalRemoved += ToRemove;

			if (InventorySlots[i].Quantity <= 0)
			{
				InventorySlots[i].Clear();
			}
		}
	}

	if (TotalRemoved > 0)
	{
		OnItemRemoved.Broadcast(ItemID, TotalRemoved);
		OnInventoryChanged.Broadcast();
	}

	return TotalRemoved;
}

bool UInventoryComponent::RemoveItemAtSlot(int32 SlotIndex, int32 Quantity)
{
	if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty())
	{
		return false;
	}

	FName ItemID = InventorySlots[SlotIndex].ItemID;
	int32 ToRemove = FMath::Min(Quantity, InventorySlots[SlotIndex].Quantity);

	InventorySlots[SlotIndex].Quantity -= ToRemove;

	if (InventorySlots[SlotIndex].Quantity <= 0)
	{
		InventorySlots[SlotIndex].Clear();
	}

	OnItemRemoved.Broadcast(ItemID, ToRemove);
	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	return GetItemCount(ItemID) >= Quantity;
}

int32 UInventoryComponent::GetItemCount(FName ItemID) const
{
	int32 Total = 0;

	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.ItemID == ItemID)
		{
			Total += Slot.Quantity;
		}
	}

	return Total;
}

bool UInventoryComponent::GetItemData(FName ItemID, FItemData& OutItemData) const
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return false;
	}

	FItemData* FoundData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("GetItemData"));
	if (FoundData)
	{
		OutItemData = *FoundData;
		return true;
	}

	return false;
}

FInventorySlot UInventoryComponent::GetSlotAtIndex(int32 Index) const
{
	if (InventorySlots.IsValidIndex(Index))
	{
		return InventorySlots[Index];
	}
	return FInventorySlot();
}

TArray<FInventorySlot> UInventoryComponent::GetSlotsByCategory(EItemCategory Category) const
{
	TArray<FInventorySlot> Result;

	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (!Slot.IsEmpty())
		{
			FItemData ItemData;
			if (GetItemData(Slot.ItemID, ItemData) && ItemData.Category == Category)
			{
				Result.Add(Slot);
			}
		}
	}

	return Result;
}

TArray<FInventorySlot> UInventoryComponent::GetSlotsByEquipmentSlot(EEquipmentSlot EquipSlot) const
{
	TArray<FInventorySlot> Result;

	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (!Slot.IsEmpty())
		{
			FItemData ItemData;
			if (GetItemData(Slot.ItemID, ItemData) && ItemData.EquipmentSlot == EquipSlot)
			{
				Result.Add(Slot);
			}
		}
	}

	return Result;
}

bool UInventoryComponent::IsFull() const
{
	return FindEmptySlot() == INDEX_NONE;
}

int32 UInventoryComponent::GetUsedSlotCount() const
{
	int32 Count = 0;
	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (!Slot.IsEmpty())
		{
			++Count;
		}
	}
	return Count;
}

bool UInventoryComponent::SwapSlots(int32 IndexA, int32 IndexB)
{
	if (!InventorySlots.IsValidIndex(IndexA) || !InventorySlots.IsValidIndex(IndexB))
	{
		return false;
	}

	FInventorySlot Temp = InventorySlots[IndexA];
	InventorySlots[IndexA] = InventorySlots[IndexB];
	InventorySlots[IndexB] = Temp;

	OnInventoryChanged.Broadcast();
	return true;
}

void UInventoryComponent::SortInventory()
{
	// Sort by category, then by name
	InventorySlots.Sort([this](const FInventorySlot& A, const FInventorySlot& B)
	{
		// Empty slots go to the end
		if (A.IsEmpty() && !B.IsEmpty()) return false;
		if (!A.IsEmpty() && B.IsEmpty()) return true;
		if (A.IsEmpty() && B.IsEmpty()) return false;

		FItemData DataA, DataB;
		GetItemData(A.ItemID, DataA);
		GetItemData(B.ItemID, DataB);

		// Sort by category first
		if (DataA.Category != DataB.Category)
		{
			return static_cast<uint8>(DataA.Category) < static_cast<uint8>(DataB.Category);
		}

		// Then by name
		return DataA.DisplayName.ToString() < DataB.DisplayName.ToString();
	});

	OnInventoryChanged.Broadcast();
}

int32 UInventoryComponent::FindSlotWithItem(FName ItemID) const
{
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		if (InventorySlots[i].ItemID == ItemID)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UInventoryComponent::FindEmptySlot() const
{
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		if (InventorySlots[i].IsEmpty())
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UInventoryComponent::FindStackableSlot(FName ItemID, const FItemData& ItemData) const
{
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		if (InventorySlots[i].ItemID == ItemID &&
			InventorySlots[i].Quantity < ItemData.MaxStackSize)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

AItemPickup* UInventoryComponent::DropItem(FName ItemID, int32 Quantity, FVector DropOffset)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return nullptr;
	}

	// Check if we have enough
	int32 CurrentCount = GetItemCount(ItemID);
	if (CurrentCount <= 0)
	{
		return nullptr;
	}

	// Clamp to what we have
	int32 ActualDrop = FMath::Min(Quantity, CurrentCount);

	// Get item data for validation
	FItemData ItemData;
	if (!GetItemData(ItemID, ItemData))
	{
		return nullptr;
	}

	// Check if item can be dropped
	if (!ItemData.bCanDrop)
	{
		return nullptr;
	}

	// Remove from inventory first
	int32 Removed = RemoveItem(ItemID, ActualDrop);
	if (Removed <= 0)
	{
		return nullptr;
	}

	// Calculate spawn location
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		// Put items back if we can't spawn
		AddItem(ItemID, Removed);
		return nullptr;
	}

	FVector SpawnLocation = Owner->GetActorLocation();
	FRotator SpawnRotation = Owner->GetActorRotation();

	// Apply offset relative to owner's forward direction
	FVector ForwardOffset = Owner->GetActorForwardVector() * DropOffset.X;
	FVector RightOffset = Owner->GetActorRightVector() * DropOffset.Y;
	FVector UpOffset = FVector(0.0f, 0.0f, DropOffset.Z);
	SpawnLocation += ForwardOffset + RightOffset + UpOffset;

	// Spawn the pickup
	UWorld* World = GetWorld();
	if (!World)
	{
		AddItem(ItemID, Removed);
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Use custom pickup class if specified, otherwise default
	TSubclassOf<AItemPickup> PickupClassToSpawn = ItemData.PickupClass;
	if (!PickupClassToSpawn)
	{
		PickupClassToSpawn = AItemPickup::StaticClass();
	}

	AItemPickup* Pickup = World->SpawnActor<AItemPickup>(PickupClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	if (Pickup)
	{
		Pickup->SetItem(ItemID, Removed);
		Pickup->ItemDataTable = ItemDataTable;

		// Set the visual mesh from item data
		if (ItemData.WorldMesh.IsValid())
		{
			UStaticMesh* Mesh = ItemData.WorldMesh.LoadSynchronous();
			if (Mesh && Pickup->ItemMesh)
			{
				Pickup->ItemMesh->SetStaticMesh(Mesh);
			}
		}
	}
	else
	{
		// Failed to spawn, return items
		AddItem(ItemID, Removed);
	}

	return Pickup;
}

AItemPickup* UInventoryComponent::DropItemAtSlot(int32 SlotIndex, int32 Quantity, FVector DropOffset)
{
	if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty())
	{
		return nullptr;
	}

	FName ItemID = InventorySlots[SlotIndex].ItemID;
	int32 ActualDrop = FMath::Min(Quantity, InventorySlots[SlotIndex].Quantity);

	return DropItem(ItemID, ActualDrop, DropOffset);
}

void UInventoryComponent::ClearInventory()
{
	for (FInventorySlot& Slot : InventorySlots)
	{
		Slot.Clear();
	}
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::SetInventorySlots(const TArray<FInventorySlot>& NewSlots)
{
	// Ensure we have the right number of slots
	InventorySlots.SetNum(MaxSlots);

	// Copy data from saved slots
	for (int32 i = 0; i < NewSlots.Num() && i < MaxSlots; ++i)
	{
		InventorySlots[i] = NewSlots[i];
	}

	// Clear any remaining slots
	for (int32 i = NewSlots.Num(); i < MaxSlots; ++i)
	{
		InventorySlots[i].Clear();
	}

	OnInventoryChanged.Broadcast();
}
