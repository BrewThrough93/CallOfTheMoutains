// CallOfTheMoutains - Equipment Component Implementation

#include "EquipmentComponent.h"
#include "InventoryComponent.h"
#include "HealthComponent.h"
#include "LampActor.h"
#include "Engine/DataTable.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "LockOnComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Only tick when needed (during attacks)

	// NOTE: Do NOT use ConstructorHelpers here - causes circular dependency crash
	// ItemDataTable will be loaded in BeginPlay
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// Load ItemDataTable at runtime if not set
	if (!ItemDataTable)
	{
		ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/BluePrints/Data/ItemData"));
		if (!ItemDataTable)
		{
			UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: Failed to load ItemDataTable. Equipment and inventory features may not work correctly."));
		}
	}

	// Auto-find inventory component if not set
	if (!InventoryComponent)
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UInventoryComponent>();
	}

	// Auto-find health component if not set
	if (!HealthComponent)
	{
		HealthComponent = GetOwner()->FindComponentByClass<UHealthComponent>();
	}

	// Share DataTable between EquipmentComponent and InventoryComponent
	if (InventoryComponent)
	{
		// If we have DataTable but inventory doesn't, share ours
		if (ItemDataTable && !InventoryComponent->ItemDataTable)
		{
			InventoryComponent->ItemDataTable = ItemDataTable;
		}
		// If inventory has DataTable but we don't, use theirs
		else if (!ItemDataTable && InventoryComponent->ItemDataTable)
		{
			ItemDataTable = InventoryComponent->ItemDataTable;
		}
	}

	// Initialize hotbar slots
	HotbarSlots.Add(EHotbarSlot::Consumable, FHotbarSlotData());
	HotbarSlots.Add(EHotbarSlot::PrimaryWeapon, FHotbarSlotData());
	HotbarSlots.Add(EHotbarSlot::OffHand, FHotbarSlotData());
	HotbarSlots.Add(EHotbarSlot::Special, FHotbarSlotData());

	// Initialize all equipment slots
	// Armor slots
	EquippedItems.Add(EEquipmentSlot::Helmet, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Chest, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Gloves, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Legs, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Boots, NAME_None);
	// Weapon slots
	EquippedItems.Add(EEquipmentSlot::PrimaryWeapon, NAME_None);
	EquippedItems.Add(EEquipmentSlot::OffHand, NAME_None);
	// Ring slots (4)
	EquippedItems.Add(EEquipmentSlot::Ring1, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Ring2, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Ring3, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Ring4, NAME_None);
	// Trinket slots (4)
	EquippedItems.Add(EEquipmentSlot::Trinket1, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Trinket2, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Trinket3, NAME_None);
	EquippedItems.Add(EEquipmentSlot::Trinket4, NAME_None);

	// Auto-assign debug items to hotbar if inventory is in debug mode
	if (InventoryComponent && InventoryComponent->bDebugMode && ItemDataTable)
	{
		// Assign test items to hotbar
		AssignToHotbar(FName("HealthPotion"), EHotbarSlot::Consumable);
		AssignToHotbar(FName("StaminaHerb"), EHotbarSlot::Consumable);
		AssignToHotbar(FName("TestSword"), EHotbarSlot::PrimaryWeapon);
		AssignToHotbar(FName("FlameGreatsword"), EHotbarSlot::PrimaryWeapon);
		AssignToHotbar(FName("TestShield"), EHotbarSlot::OffHand);
		AssignToHotbar(FName("RustyKey"), EHotbarSlot::Special);
	}
}

void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update attack progress for dodge canceling and input buffering
	if (bIsAttacking)
	{
		UpdateAttackProgress();
	}
	else
	{
		// Disable tick when not attacking to save performance
		SetComponentTickEnabled(false);
	}
}

bool UEquipmentComponent::EquipItem(FName ItemID)
{
	FItemData ItemData;
	if (!GetItemData(ItemID, ItemData))
	{
		return false;
	}

	if (!ItemData.IsEquipment())
	{
		return false;
	}

	return EquipItemToSlot(ItemID, ItemData.EquipmentSlot);
}

bool UEquipmentComponent::EquipItemToSlot(FName ItemID, EEquipmentSlot Slot, bool bFromSaveLoad)
{
	if (Slot == EEquipmentSlot::None)
	{
		return false;
	}

	FItemData ItemData;
	if (!GetItemData(ItemID, ItemData))
	{
		return false;
	}

	// Check if item can go in this slot
	if (ItemData.EquipmentSlot != Slot)
	{
		return false;
	}

	// Unequip current item in slot first (returns to inventory)
	// Skip this when loading from save to avoid duplication
	if (!bFromSaveLoad)
	{
		UnequipSlot(Slot);
	}

	// Remove from inventory if we have inventory component
	// Skip inventory check/removal when loading from save (item is already equipped, not in inventory)
	if (InventoryComponent && !bFromSaveLoad)
	{
		if (!InventoryComponent->HasItem(ItemID))
		{
			return false;
		}
		InventoryComponent->RemoveItem(ItemID, 1);
	}

	// Equip item
	EquippedItems[Slot] = ItemID;

	// Attach visual mesh
	if (ItemData.IsWeapon())
	{
		AttachWeaponMesh(Slot, ItemData);

		// Play equip montage for the weapon
		PlayWeaponMontage(ItemData, true);

		// Auto-add weapons to hotbar for quick swapping
		EHotbarSlot TargetHotbar = EHotbarSlot::PrimaryWeapon;
		if (Slot == EEquipmentSlot::OffHand)
		{
			TargetHotbar = EHotbarSlot::OffHand;
		}

		// Add to hotbar if not already there
		FHotbarSlotData& HotbarData = HotbarSlots[TargetHotbar];
		if (!HotbarData.AssignedItems.Contains(ItemID))
		{
			if (HotbarData.AssignedItems.Num() < MaxHotbarItemsPerSlot)
			{
				HotbarData.AssignedItems.Add(ItemID);
				// Set as current if it's the only one
				if (HotbarData.AssignedItems.Num() == 1)
				{
					HotbarData.CurrentIndex = 0;
				}
				OnHotbarChanged.Broadcast(TargetHotbar);
			}
		}
	}
	else if (ItemData.IsEquipment())
	{
		AttachArmorMesh(Slot, ItemData);
	}

	// Update weapon type tracking
	UpdateWeaponTypes();

	OnEquipmentChanged.Broadcast(Slot, ItemID);
	UpdateStats();

	return true;
}

bool UEquipmentComponent::UnequipSlot(EEquipmentSlot Slot)
{
	if (!EquippedItems.Contains(Slot))
	{
		return false;
	}

	FName CurrentItem = EquippedItems[Slot];
	if (CurrentItem.IsNone())
	{
		return false;
	}

	// Play unequip montage if this is a weapon
	FItemData ItemData;
	if (GetItemData(CurrentItem, ItemData) && ItemData.IsWeapon())
	{
		PlayWeaponMontage(ItemData, false);
	}

	// Remove visual mesh
	RemoveWeaponMesh(Slot);
	RemoveArmorMesh(Slot);

	// Return to inventory
	if (InventoryComponent)
	{
		InventoryComponent->AddItem(CurrentItem, 1);
	}

	EquippedItems[Slot] = NAME_None;

	// Update weapon type tracking
	UpdateWeaponTypes();

	OnEquipmentChanged.Broadcast(Slot, NAME_None);
	UpdateStats();

	return true;
}

FName UEquipmentComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	if (const FName* Item = EquippedItems.Find(Slot))
	{
		return *Item;
	}
	return NAME_None;
}

bool UEquipmentComponent::GetEquippedItemData(EEquipmentSlot Slot, FItemData& OutItemData) const
{
	FName ItemID = GetEquippedItem(Slot);
	if (ItemID.IsNone())
	{
		return false;
	}
	return GetItemData(ItemID, OutItemData);
}

bool UEquipmentComponent::IsSlotEquipped(EEquipmentSlot Slot) const
{
	return !GetEquippedItem(Slot).IsNone();
}

FItemStats UEquipmentComponent::GetTotalEquippedStats() const
{
	FItemStats TotalStats;

	for (const auto& Pair : EquippedItems)
	{
		if (!Pair.Value.IsNone())
		{
			FItemData ItemData;
			if (GetItemData(Pair.Value, ItemData))
			{
				TotalStats = TotalStats + ItemData.Stats;
			}
		}
	}

	return TotalStats;
}

bool UEquipmentComponent::AssignToHotbar(FName ItemID, EHotbarSlot HotbarSlot)
{
	if (ItemID.IsNone())
	{
		return false;
	}

	FItemData ItemData;
	if (!GetItemData(ItemID, ItemData))
	{
		return false;
	}

	// Validate item type matches hotbar slot
	bool bValid = false;
	switch (HotbarSlot)
	{
		case EHotbarSlot::Consumable:
			bValid = ItemData.IsConsumable();
			break;
		case EHotbarSlot::PrimaryWeapon:
			bValid = ItemData.EquipmentSlot == EEquipmentSlot::PrimaryWeapon;
			break;
		case EHotbarSlot::OffHand:
			bValid = ItemData.EquipmentSlot == EEquipmentSlot::OffHand;
			break;
		case EHotbarSlot::Special:
			bValid = ItemData.Category == EItemCategory::Special || ItemData.Category == EItemCategory::KeyItem;
			break;
	}

	if (!bValid)
	{
		return false;
	}

	FHotbarSlotData& SlotData = HotbarSlots[HotbarSlot];

	// Check if already in slot
	if (SlotData.AssignedItems.Contains(ItemID))
	{
		return true;
	}

	// Check max items
	if (SlotData.AssignedItems.Num() >= MaxHotbarItemsPerSlot)
	{
		return false;
	}

	SlotData.AssignedItems.Add(ItemID);
	OnHotbarChanged.Broadcast(HotbarSlot);

	// Auto-equip if this is a weapon slot and nothing is equipped
	if (HotbarSlot == EHotbarSlot::PrimaryWeapon || HotbarSlot == EHotbarSlot::OffHand)
	{
		EquipFromHotbar(HotbarSlot);
	}

	return true;
}

bool UEquipmentComponent::RemoveFromHotbar(FName ItemID, EHotbarSlot HotbarSlot)
{
	if (!HotbarSlots.Contains(HotbarSlot))
	{
		return false;
	}

	FHotbarSlotData& SlotData = HotbarSlots[HotbarSlot];
	int32 RemovedCount = SlotData.AssignedItems.Remove(ItemID);

	if (RemovedCount > 0)
	{
		// Adjust current index if needed
		if (SlotData.CurrentIndex >= SlotData.AssignedItems.Num())
		{
			SlotData.CurrentIndex = FMath::Max(0, SlotData.AssignedItems.Num() - 1);
		}

		OnHotbarChanged.Broadcast(HotbarSlot);
		return true;
	}

	return false;
}

void UEquipmentComponent::ClearHotbarSlot(EHotbarSlot HotbarSlot)
{
	if (HotbarSlots.Contains(HotbarSlot))
	{
		HotbarSlots[HotbarSlot].AssignedItems.Empty();
		HotbarSlots[HotbarSlot].CurrentIndex = 0;
		OnHotbarChanged.Broadcast(HotbarSlot);
	}
}

void UEquipmentComponent::CycleHotbarNext(EHotbarSlot HotbarSlot)
{
	if (HotbarSlots.Contains(HotbarSlot))
	{
		HotbarSlots[HotbarSlot].CycleNext();
		OnHotbarChanged.Broadcast(HotbarSlot);

		// Auto-equip for weapon slots
		if (HotbarSlot == EHotbarSlot::PrimaryWeapon || HotbarSlot == EHotbarSlot::OffHand)
		{
			EquipFromHotbar(HotbarSlot);
		}
	}
}

void UEquipmentComponent::CycleHotbarPrevious(EHotbarSlot HotbarSlot)
{
	if (HotbarSlots.Contains(HotbarSlot))
	{
		HotbarSlots[HotbarSlot].CyclePrevious();
		OnHotbarChanged.Broadcast(HotbarSlot);

		// Auto-equip for weapon slots
		if (HotbarSlot == EHotbarSlot::PrimaryWeapon || HotbarSlot == EHotbarSlot::OffHand)
		{
			EquipFromHotbar(HotbarSlot);
		}
	}
}

FName UEquipmentComponent::GetCurrentHotbarItem(EHotbarSlot HotbarSlot) const
{
	if (const FHotbarSlotData* SlotData = HotbarSlots.Find(HotbarSlot))
	{
		return SlotData->GetCurrentItem();
	}
	return NAME_None;
}

FHotbarSlotData UEquipmentComponent::GetHotbarSlotData(EHotbarSlot HotbarSlot) const
{
	if (const FHotbarSlotData* SlotData = HotbarSlots.Find(HotbarSlot))
	{
		return *SlotData;
	}
	return FHotbarSlotData();
}

void UEquipmentComponent::SetHotbarCurrentIndex(EHotbarSlot HotbarSlot, int32 Index)
{
	if (FHotbarSlotData* SlotData = HotbarSlots.Find(HotbarSlot))
	{
		if (SlotData->AssignedItems.Num() > 0)
		{
			SlotData->CurrentIndex = FMath::Clamp(Index, 0, SlotData->AssignedItems.Num() - 1);
			OnHotbarChanged.Broadcast(HotbarSlot);
		}
	}
}

bool UEquipmentComponent::UseConsumable()
{
	FName ItemID = GetCurrentHotbarItem(EHotbarSlot::Consumable);
	if (ItemID.IsNone())
	{
		return false;
	}

	// Check if we have it in inventory
	if (InventoryComponent && !InventoryComponent->HasItem(ItemID))
	{
		// Remove from hotbar if we don't have it anymore
		RemoveFromHotbar(ItemID, EHotbarSlot::Consumable);
		return false;
	}

	FItemData ItemData;
	if (!GetItemData(ItemID, ItemData))
	{
		return false;
	}

	// Handle toggle items (infinite use, no consumption)
	if (ItemData.IsToggleItem())
	{
		// Check if toggle actor already exists
		if (AActor** ExistingActor = SpawnedToggleActors.Find(ItemID))
		{
			if (*ExistingActor)
			{
				// Toggle the existing actor (e.g., turn lamp on/off)
				ALampActor* Lamp = Cast<ALampActor>(*ExistingActor);
				if (Lamp)
				{
					Lamp->ToggleLamp();
				}
				return true;
			}
		}

		// Spawn new toggle actor if class is specified
		if (ItemData.ToggleActorClass)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = GetOwner();
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AActor* NewActor = World->SpawnActor<AActor>(ItemData.ToggleActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				if (NewActor)
				{
					// Attach to player character
					ACharacter* Character = nullptr;
					APawn* OwnerPawn = Cast<APawn>(GetOwner());
					if (OwnerPawn)
					{
						Character = Cast<ACharacter>(OwnerPawn);
					}
					else
					{
						APlayerController* PC = Cast<APlayerController>(GetOwner());
						if (PC)
						{
							Character = Cast<ACharacter>(PC->GetPawn());
						}
					}

					// Try to attach lamp to character
					ALampActor* Lamp = Cast<ALampActor>(NewActor);
					if (Lamp && Character)
					{
						// Set the lamp mesh from item's WorldMesh
						if (!ItemData.WorldMesh.IsNull())
						{
							UStaticMesh* LampMesh = ItemData.WorldMesh.LoadSynchronous();
							if (LampMesh)
							{
								Lamp->SetLampMesh(LampMesh);
							}
						}

						// Use item's AttachSocket if specified, otherwise lamp's default
						if (!ItemData.AttachSocket.IsNone())
						{
							Lamp->AttachToCharacterAtSocket(Character, ItemData.AttachSocket);
						}
						else
						{
							Lamp->AttachToCharacter(Character);
						}
						Lamp->TurnOn();
					}

					// Store reference
					SpawnedToggleActors.Add(ItemID, NewActor);
					return true;
				}
			}
		}

		// Toggle item used but no actor class - just return success (item not consumed)
		return true;
	}

	// Regular consumable - apply effect and consume
	// Apply consumable effects to character via HealthComponent
	if (HealthComponent)
	{
		const FConsumableEffect& Effect = ItemData.ConsumableEffect;

		// Apply instant effects
		if (Effect.bIsInstant)
		{
			if (Effect.HealthRestore > 0.0f)
			{
				HealthComponent->Heal(Effect.HealthRestore);
			}
			if (Effect.StaminaRestore > 0.0f)
			{
				HealthComponent->RestoreStamina(Effect.StaminaRestore);
			}
		}
		else
		{
			// For over-time effects, apply immediate boost and let natural regen handle the rest
			// A more complete implementation would use a buff system
			if (Effect.HealthRestore > 0.0f)
			{
				HealthComponent->Heal(Effect.HealthRestore);
			}
			if (Effect.StaminaRestore > 0.0f)
			{
				HealthComponent->RestoreStamina(Effect.StaminaRestore);
			}
		}
	}

	// Remove from inventory
	if (InventoryComponent)
	{
		InventoryComponent->RemoveItem(ItemID, 1);

		// If we ran out, remove from hotbar
		if (!InventoryComponent->HasItem(ItemID))
		{
			RemoveFromHotbar(ItemID, EHotbarSlot::Consumable);
		}
	}

	return true;
}

bool UEquipmentComponent::UseSpecialItem()
{
	FName ItemID = GetCurrentHotbarItem(EHotbarSlot::Special);
	if (ItemID.IsNone())
	{
		return false;
	}

	FItemData ItemData;
	if (!GetItemData(ItemID, ItemData))
	{
		return false;
	}

	// Handle toggle items (like lanterns, torches)
	if (ItemData.IsToggleItem())
	{
		// Check if toggle actor already exists
		if (AActor** ExistingActor = SpawnedToggleActors.Find(ItemID))
		{
			if (*ExistingActor)
			{
				ALampActor* Lamp = Cast<ALampActor>(*ExistingActor);
				if (Lamp)
				{
					Lamp->ToggleLamp();
				}
				return true;
			}
		}

		// Spawn new toggle actor if class is specified
		if (ItemData.ToggleActorClass)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = GetOwner();
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AActor* NewActor = World->SpawnActor<AActor>(ItemData.ToggleActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				if (NewActor)
				{
					ACharacter* Character = nullptr;
					APawn* OwnerPawn = Cast<APawn>(GetOwner());
					if (OwnerPawn)
					{
						Character = Cast<ACharacter>(OwnerPawn);
					}
					else
					{
						APlayerController* PC = Cast<APlayerController>(GetOwner());
						if (PC)
						{
							Character = Cast<ACharacter>(PC->GetPawn());
						}
					}

					ALampActor* Lamp = Cast<ALampActor>(NewActor);
					if (Lamp && Character)
					{
						if (!ItemData.WorldMesh.IsNull())
						{
							UStaticMesh* LampMesh = ItemData.WorldMesh.LoadSynchronous();
							if (LampMesh)
							{
								Lamp->SetLampMesh(LampMesh);
							}
						}

						if (!ItemData.AttachSocket.IsNone())
						{
							Lamp->AttachToCharacterAtSocket(Character, ItemData.AttachSocket);
						}
						else
						{
							Lamp->AttachToCharacter(Character);
						}
						Lamp->TurnOn();
					}

					SpawnedToggleActors.Add(ItemID, NewActor);
					return true;
				}
			}
		}
		return true;
	}

	// Key items typically don't have direct use effects
	// They're used through interaction systems (doors, NPCs, etc.)
	if (ItemData.bIsKeyItem)
	{
		// Key items are valid but don't have a direct "use" effect
		// The game should handle these through interaction/quest systems
		return false;
	}

	// Special items with consumable effects (rare spells, etc.)
	if (ItemData.ConsumableEffect.HealthRestore > 0.0f || ItemData.ConsumableEffect.StaminaRestore > 0.0f)
	{
		if (HealthComponent)
		{
			if (ItemData.ConsumableEffect.HealthRestore > 0.0f)
			{
				HealthComponent->Heal(ItemData.ConsumableEffect.HealthRestore);
			}
			if (ItemData.ConsumableEffect.StaminaRestore > 0.0f)
			{
				HealthComponent->RestoreStamina(ItemData.ConsumableEffect.StaminaRestore);
			}
		}

		// Consume the item if it's not infinite use
		if (!ItemData.bIsToggleItem && InventoryComponent)
		{
			InventoryComponent->RemoveItem(ItemID, 1);
			if (!InventoryComponent->HasItem(ItemID))
			{
				RemoveFromHotbar(ItemID, EHotbarSlot::Special);
			}
		}
		return true;
	}

	return false;
}

bool UEquipmentComponent::IsToggleItemActive(FName ItemID) const
{
	if (const AActor* const* Actor = SpawnedToggleActors.Find(ItemID))
	{
		if (*Actor)
		{
			// Check if it's a lamp and if it's on
			const ALampActor* Lamp = Cast<ALampActor>(*Actor);
			if (Lamp)
			{
				return Lamp->IsLampOn();
			}
			// For other toggle actors, just return true if spawned
			return true;
		}
	}
	return false;
}

AActor* UEquipmentComponent::GetToggleItemActor(FName ItemID) const
{
	if (const AActor* const* FoundActor = SpawnedToggleActors.Find(ItemID))
	{
		return const_cast<AActor*>(*FoundActor);
	}
	return nullptr;
}

FName UEquipmentComponent::GetPrimaryWeapon() const
{
	return GetEquippedItem(EEquipmentSlot::PrimaryWeapon);
}

FName UEquipmentComponent::GetOffHandItem() const
{
	return GetEquippedItem(EEquipmentSlot::OffHand);
}

void UEquipmentComponent::CyclePrimaryWeapon()
{
	CycleHotbarNext(EHotbarSlot::PrimaryWeapon);
}

void UEquipmentComponent::CycleOffHand()
{
	CycleHotbarNext(EHotbarSlot::OffHand);
}

bool UEquipmentComponent::GetItemData(FName ItemID, FItemData& OutItemData) const
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

void UEquipmentComponent::UpdateStats()
{
	UpdateWeight();
	OnStatsChanged.Broadcast();
}

void UEquipmentComponent::UpdateWeight()
{
	float OldWeight = CurrentEquippedWeight;
	bool bWasOverEncumbered = bIsOverEncumbered;

	// Calculate total weight from all equipped items
	CurrentEquippedWeight = 0.0f;
	for (const auto& Pair : EquippedItems)
	{
		if (!Pair.Value.IsNone())
		{
			FItemData ItemData;
			if (GetItemData(Pair.Value, ItemData))
			{
				CurrentEquippedWeight += ItemData.Stats.Weight;
			}
		}
	}

	// Check encumbrance
	bIsOverEncumbered = CurrentEquippedWeight > MaxCarryWeight;

	// Broadcast if encumbrance state changed
	if (bIsOverEncumbered != bWasOverEncumbered)
	{
		OnEncumbranceChanged.Broadcast(bIsOverEncumbered);
	}
}

void UEquipmentComponent::EquipFromHotbar(EHotbarSlot HotbarSlot)
{
	FName ItemID = GetCurrentHotbarItem(HotbarSlot);
	if (ItemID.IsNone())
	{
		return;
	}

	EEquipmentSlot TargetSlot = EEquipmentSlot::None;
	if (HotbarSlot == EHotbarSlot::PrimaryWeapon)
	{
		TargetSlot = EEquipmentSlot::PrimaryWeapon;
	}
	else if (HotbarSlot == EHotbarSlot::OffHand)
	{
		TargetSlot = EEquipmentSlot::OffHand;
	}

	if (TargetSlot != EEquipmentSlot::None)
	{
		// Don't use EquipItemToSlot since hotbar items don't come from inventory
		// Just directly set the equipped item
		FName CurrentEquipped = GetEquippedItem(TargetSlot);
		if (CurrentEquipped != ItemID)
		{
			// Remove old mesh first
			RemoveWeaponMesh(TargetSlot);

			EquippedItems[TargetSlot] = ItemID;

			// Attach new weapon mesh
			FItemData ItemData;
			if (GetItemData(ItemID, ItemData))
			{
				AttachWeaponMesh(TargetSlot, ItemData);
			}

			// Update weapon type
			UpdateWeaponTypes();

			OnEquipmentChanged.Broadcast(TargetSlot, ItemID);
			UpdateStats();
		}
	}
}

USkeletalMeshComponent* UEquipmentComponent::GetOwnerMesh() const
{
	// First try: Owner is a Character directly
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		return Character->GetMesh();
	}

	// Second try: Owner is a PlayerController - get the controlled Pawn
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (PC)
	{
		APawn* ControlledPawn = PC->GetPawn();
		if (ControlledPawn)
		{
			ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn);
			if (ControlledCharacter)
			{
				return ControlledCharacter->GetMesh();
			}
		}
	}

	return nullptr;
}

void UEquipmentComponent::AttachWeaponMesh(EEquipmentSlot Slot, const FItemData& ItemData)
{
	// Only weapons go to sockets
	if (Slot != EEquipmentSlot::PrimaryWeapon && Slot != EEquipmentSlot::OffHand)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	// Use item's socket name if specified, otherwise use default
	FName SocketName = ItemData.AttachSocket;
	if (SocketName.IsNone())
	{
		SocketName = (Slot == EEquipmentSlot::PrimaryWeapon) ? PrimaryWeaponSocket : OffHandSocket;
	}

	FAttachmentTransformRules AttachRules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;

	// Use skeletal mesh for weapons - use IsNull() NOT IsValid()!
	if (!ItemData.SkeletalMesh.IsNull())
	{
		USkeletalMesh* WeaponMesh = ItemData.SkeletalMesh.LoadSynchronous();
		if (WeaponMesh)
		{
			// Create new skeletal mesh component
			USkeletalMeshComponent* NewMeshComp = NewObject<USkeletalMeshComponent>(OwnerMesh->GetOwner());
			NewMeshComp->SetSkeletalMesh(WeaponMesh);
			NewMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NewMeshComp->RegisterComponent();
			NewMeshComp->AttachToComponent(OwnerMesh, AttachRules, SocketName);

			// Apply item's mesh scale (set in DataTable)
			NewMeshComp->SetRelativeScale3D(ItemData.MeshScale);

			// Store reference
			WeaponMeshComponents.Add(Slot, NewMeshComp);
		}
	}
	// Fallback to static mesh (WorldMesh) if no skeletal mesh - use IsNull() NOT IsValid()!
	else if (!ItemData.WorldMesh.IsNull())
	{
		UStaticMesh* WeaponStaticMesh = ItemData.WorldMesh.LoadSynchronous();
		if (WeaponStaticMesh)
		{
			// For static mesh weapons, we create a simple attachment
			UStaticMeshComponent* StaticComp = NewObject<UStaticMeshComponent>(OwnerMesh->GetOwner());
			StaticComp->SetStaticMesh(WeaponStaticMesh);
			StaticComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			StaticComp->RegisterComponent();
			StaticComp->AttachToComponent(OwnerMesh, AttachRules, SocketName);

			// Apply item's mesh scale (set in DataTable)
			StaticComp->SetRelativeScale3D(ItemData.MeshScale);

			// Store reference so we can remove/stow it later
			WeaponStaticMeshComponents.Add(Slot, StaticComp);
		}
	}
}

void UEquipmentComponent::RemoveWeaponMesh(EEquipmentSlot Slot)
{
	// Remove skeletal mesh component if exists
	if (USkeletalMeshComponent** MeshComp = WeaponMeshComponents.Find(Slot))
	{
		if (*MeshComp)
		{
			(*MeshComp)->DestroyComponent();
		}
		WeaponMeshComponents.Remove(Slot);
	}

	// Also remove static mesh component if exists (fallback weapons use static mesh)
	if (UStaticMeshComponent** StaticMeshComp = WeaponStaticMeshComponents.Find(Slot))
	{
		if (*StaticMeshComp)
		{
			(*StaticMeshComp)->DestroyComponent();
		}
		WeaponStaticMeshComponents.Remove(Slot);
	}
}

void UEquipmentComponent::AttachArmorMesh(EEquipmentSlot Slot, const FItemData& ItemData)
{
	// Only armor slots get visual meshes attached (Helmet, Chest, Gloves, Legs, Boots)
	// Weapons use AttachWeaponMesh; rings/trinkets have no visual
	if (Slot == EEquipmentSlot::PrimaryWeapon || Slot == EEquipmentSlot::OffHand ||
		Slot == EEquipmentSlot::Ring1 || Slot == EEquipmentSlot::Ring2 ||
		Slot == EEquipmentSlot::Ring3 || Slot == EEquipmentSlot::Ring4 ||
		Slot == EEquipmentSlot::Trinket1 || Slot == EEquipmentSlot::Trinket2 ||
		Slot == EEquipmentSlot::Trinket3 || Slot == EEquipmentSlot::Trinket4 ||
		Slot == EEquipmentSlot::None)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	// Use skeletal mesh with leader pose - use IsNull() NOT IsValid() for soft pointers!
	if (!ItemData.SkeletalMesh.IsNull())
	{
		USkeletalMesh* ArmorMesh = ItemData.SkeletalMesh.LoadSynchronous();
		if (ArmorMesh)
		{
			// Create new skeletal mesh component
			USkeletalMeshComponent* NewMeshComp = NewObject<USkeletalMeshComponent>(GetOwner());
			NewMeshComp->SetSkeletalMesh(ArmorMesh);
			NewMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NewMeshComp->RegisterComponent();
			NewMeshComp->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			// Set leader pose - follow owner's skeletal mesh
			NewMeshComp->SetLeaderPoseComponent(OwnerMesh);

			// Store reference
			ArmorMeshComponents.Add(Slot, NewMeshComp);
		}
	}
}

void UEquipmentComponent::RemoveArmorMesh(EEquipmentSlot Slot)
{
	if (USkeletalMeshComponent** MeshComp = ArmorMeshComponents.Find(Slot))
	{
		if (*MeshComp)
		{
			(*MeshComp)->DestroyComponent();
		}
		ArmorMeshComponents.Remove(Slot);
	}
}

void UEquipmentComponent::UpdateWeaponTypes()
{
	EWeaponType OldAnimationType = GetCurrentWeaponType();

	// Update primary weapon type
	FItemData PrimaryData;
	if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryData))
	{
		CurrentPrimaryWeaponType = PrimaryData.WeaponType;
	}
	else
	{
		CurrentPrimaryWeaponType = EWeaponType::None;
	}

	// Update off-hand weapon type
	FItemData OffHandData;
	if (GetEquippedItemData(EEquipmentSlot::OffHand, OffHandData))
	{
		CurrentOffHandWeaponType = OffHandData.WeaponType;
	}
	else
	{
		CurrentOffHandWeaponType = EWeaponType::None;
	}

	// Determine animation weapon type:
	// Shield in off-hand overrides primary for animation selection
	// This allows "Sword and Shield" to use Shield weapon type for animations
	EWeaponType NewAnimationType = GetCurrentWeaponType();

	// Broadcast if animation weapon type changed
	if (NewAnimationType != OldAnimationType)
	{
		SetWeaponType(NewAnimationType);
	}
}

void UEquipmentComponent::SetWeaponType(EWeaponType NewType)
{
	OnWeaponTypeChanged.Broadcast(NewType);
}

void UEquipmentComponent::PlayWeaponMontage(const FItemData& ItemData, bool bEquipping)
{
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	UAnimMontage* Montage = nullptr;

	if (bEquipping)
	{
		if (!ItemData.EquipMontage.IsNull())
		{
			Montage = ItemData.EquipMontage.LoadSynchronous();
		}
	}
	else
	{
		if (!ItemData.UnequipMontage.IsNull())
		{
			Montage = ItemData.UnequipMontage.LoadSynchronous();
		}
	}

	if (Montage)
	{
		UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(Montage);
		}
	}
}

void UEquipmentComponent::StowWeapons()
{
	if (bWeaponsStowed)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	// Play unequip montage for primary weapon before stowing
	FItemData PrimaryItemData;
	if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryItemData))
	{
		PlayWeaponMontage(PrimaryItemData, false); // false = unequip/stow
	}

	// Move primary weapon to stow socket (skeletal mesh)
	if (USkeletalMeshComponent** PrimaryMesh = WeaponMeshComponents.Find(EEquipmentSlot::PrimaryWeapon))
	{
		if (*PrimaryMesh)
		{
			FVector CurrentScale = (*PrimaryMesh)->GetRelativeScale3D();
			FItemData ItemData;
			FName SocketName = PrimaryWeaponStowSocket;
			if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, ItemData) && !ItemData.StowSocket.IsNone())
			{
				SocketName = ItemData.StowSocket;
			}
			if (OwnerMesh->DoesSocketExist(SocketName))
			{
				(*PrimaryMesh)->AttachToComponent(OwnerMesh,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
				(*PrimaryMesh)->SetRelativeScale3D(CurrentScale);
			}
		}
	}
	// Move primary weapon to stow socket (static mesh fallback)
	else if (UStaticMeshComponent** PrimaryStaticMesh = WeaponStaticMeshComponents.Find(EEquipmentSlot::PrimaryWeapon))
	{
		if (*PrimaryStaticMesh)
		{
			FVector CurrentScale = (*PrimaryStaticMesh)->GetRelativeScale3D();
			FItemData ItemData;
			FName SocketName = PrimaryWeaponStowSocket;
			if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, ItemData) && !ItemData.StowSocket.IsNone())
			{
				SocketName = ItemData.StowSocket;
			}
			if (OwnerMesh->DoesSocketExist(SocketName))
			{
				(*PrimaryStaticMesh)->AttachToComponent(OwnerMesh,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
				(*PrimaryStaticMesh)->SetRelativeScale3D(CurrentScale);
			}
		}
	}

	// Move off-hand to stow socket (skeletal mesh)
	if (USkeletalMeshComponent** OffHandMesh = WeaponMeshComponents.Find(EEquipmentSlot::OffHand))
	{
		if (*OffHandMesh)
		{
			FVector CurrentScale = (*OffHandMesh)->GetRelativeScale3D();
			FItemData ItemData;
			FName SocketName = OffHandStowSocket;
			if (GetEquippedItemData(EEquipmentSlot::OffHand, ItemData) && !ItemData.StowSocket.IsNone())
			{
				SocketName = ItemData.StowSocket;
			}
			if (OwnerMesh->DoesSocketExist(SocketName))
			{
				(*OffHandMesh)->AttachToComponent(OwnerMesh,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
				(*OffHandMesh)->SetRelativeScale3D(CurrentScale);
			}
		}
	}
	// Move off-hand to stow socket (static mesh fallback)
	else if (UStaticMeshComponent** OffHandStaticMesh = WeaponStaticMeshComponents.Find(EEquipmentSlot::OffHand))
	{
		if (*OffHandStaticMesh)
		{
			FVector CurrentScale = (*OffHandStaticMesh)->GetRelativeScale3D();
			FItemData ItemData;
			FName SocketName = OffHandStowSocket;
			if (GetEquippedItemData(EEquipmentSlot::OffHand, ItemData) && !ItemData.StowSocket.IsNone())
			{
				SocketName = ItemData.StowSocket;
			}
			if (OwnerMesh->DoesSocketExist(SocketName))
			{
				(*OffHandStaticMesh)->AttachToComponent(OwnerMesh,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
				(*OffHandStaticMesh)->SetRelativeScale3D(CurrentScale);
			}
		}
	}

	bWeaponsStowed = true;

	// Broadcast weapon type change to None (for animation to switch to unarmed)
	SetWeaponType(EWeaponType::None);
}

void UEquipmentComponent::DrawWeapons()
{
	if (!bWeaponsStowed)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	// Move primary weapon back to hand socket (skeletal mesh)
	FItemData PrimaryItemData;
	if (USkeletalMeshComponent** PrimaryMesh = WeaponMeshComponents.Find(EEquipmentSlot::PrimaryWeapon))
	{
		if (*PrimaryMesh)
		{
			FVector CurrentScale = (*PrimaryMesh)->GetRelativeScale3D();
			FName SocketName = PrimaryWeaponSocket;
			if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryItemData) && !PrimaryItemData.AttachSocket.IsNone())
			{
				SocketName = PrimaryItemData.AttachSocket;
			}
			(*PrimaryMesh)->AttachToComponent(OwnerMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
			(*PrimaryMesh)->SetRelativeScale3D(CurrentScale);
		}
	}
	// Move primary weapon back to hand socket (static mesh fallback)
	else if (UStaticMeshComponent** PrimaryStaticMesh = WeaponStaticMeshComponents.Find(EEquipmentSlot::PrimaryWeapon))
	{
		if (*PrimaryStaticMesh)
		{
			FVector CurrentScale = (*PrimaryStaticMesh)->GetRelativeScale3D();
			FName SocketName = PrimaryWeaponSocket;
			if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryItemData) && !PrimaryItemData.AttachSocket.IsNone())
			{
				SocketName = PrimaryItemData.AttachSocket;
			}
			(*PrimaryStaticMesh)->AttachToComponent(OwnerMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
			(*PrimaryStaticMesh)->SetRelativeScale3D(CurrentScale);
		}
	}

	// Move off-hand back to hand socket (skeletal mesh)
	if (USkeletalMeshComponent** OffHandMesh = WeaponMeshComponents.Find(EEquipmentSlot::OffHand))
	{
		if (*OffHandMesh)
		{
			FVector CurrentScale = (*OffHandMesh)->GetRelativeScale3D();
			FItemData ItemData;
			FName SocketName = OffHandSocket;
			if (GetEquippedItemData(EEquipmentSlot::OffHand, ItemData) && !ItemData.AttachSocket.IsNone())
			{
				SocketName = ItemData.AttachSocket;
			}
			(*OffHandMesh)->AttachToComponent(OwnerMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
			(*OffHandMesh)->SetRelativeScale3D(CurrentScale);
		}
	}
	// Move off-hand back to hand socket (static mesh fallback)
	else if (UStaticMeshComponent** OffHandStaticMesh = WeaponStaticMeshComponents.Find(EEquipmentSlot::OffHand))
	{
		if (*OffHandStaticMesh)
		{
			FVector CurrentScale = (*OffHandStaticMesh)->GetRelativeScale3D();
			FItemData ItemData;
			FName SocketName = OffHandSocket;
			if (GetEquippedItemData(EEquipmentSlot::OffHand, ItemData) && !ItemData.AttachSocket.IsNone())
			{
				SocketName = ItemData.AttachSocket;
			}
			(*OffHandStaticMesh)->AttachToComponent(OwnerMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
			(*OffHandStaticMesh)->SetRelativeScale3D(CurrentScale);
		}
	}

	bWeaponsStowed = false;

	// Play equip montage for primary weapon after drawing
	if (PrimaryItemData.IsValid())
	{
		PlayWeaponMontage(PrimaryItemData, true); // true = equip/draw
	}

	// Broadcast weapon type change back to actual type
	SetWeaponType(CurrentPrimaryWeaponType);
}

void UEquipmentComponent::ToggleWeaponStow()
{
	if (bWeaponsStowed)
	{
		DrawWeapons();
	}
	else
	{
		StowWeapons();
	}
}

// ==================== Combat Functions ====================

void UEquipmentComponent::LightAttack()
{
	// Don't attack if guarding or in bad state
	if (bIsGuarding || CurrentCombatState == ECombatState::Staggered ||
		CurrentCombatState == ECombatState::GuardBroken)
	{
		return;
	}

	// Check for drop attack - if airborne and can drop attack, do that instead
	if (CanDropAttack())
	{
		DropAttack();
		return;
	}

	// If currently attacking, try to buffer the input
	if (bIsAttacking && !bComboWindowOpen)
	{
		BufferInput(EBufferedInputType::LightAttack);
		return;
	}

	// Check stamina cost
	if (HealthComponent && !HealthComponent->HasStamina(CombatConfig.LightAttackStaminaCost))
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* MontageToPlay = nullptr;
	int32 MaxComboCount = 0;

	// Check if we have a weapon equipped (and not stowed)
	bool bHasWeaponEquipped = !bWeaponsStowed && CurrentPrimaryWeaponType != EWeaponType::None;

	if (bHasWeaponEquipped)
	{
		// Use weapon's light attack montage array
		FItemData WeaponData;
		if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, WeaponData))
		{
			MaxComboCount = WeaponData.LightAttackMontages.Num();
			if (MaxComboCount > 0)
			{
				// Get current combo montage (loop back to start)
				int32 ComboIdx = LightComboIndex % MaxComboCount;
				if (!WeaponData.LightAttackMontages[ComboIdx].IsNull())
				{
					MontageToPlay = WeaponData.LightAttackMontages[ComboIdx].LoadSynchronous();
				}
			}
		}
	}
	else
	{
		// Unarmed light attack combo
		MaxComboCount = UnarmedLightAttackMontages.Num();
		if (MaxComboCount > 0)
		{
			int32 ComboIdx = LightComboIndex % MaxComboCount;
			if (!UnarmedLightAttackMontages[ComboIdx].IsNull())
			{
				MontageToPlay = UnarmedLightAttackMontages[ComboIdx].LoadSynchronous();
			}
		}
	}

	if (MontageToPlay)
	{
		// Use stamina
		if (HealthComponent)
		{
			HealthComponent->UseStamina(CombatConfig.LightAttackStaminaCost);
		}

		bIsAttacking = true;
		bComboWindowOpen = false;
		SetCombatState(ECombatState::Attacking);
		SetComponentTickEnabled(true); // Enable tick for attack progress tracking

		// Clear combo window timer
		GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle);

		// Face lock-on target before attacking
		FaceLockedTarget();

		// Stop any currently playing montage first
		AnimInstance->StopAllMontages(0.1f);

		float Duration = AnimInstance->Montage_Play(MontageToPlay, 1.0f);

		if (Duration > 0.0f)
		{
			// Track attack progress for dodge cancel
			AttackStartTime = GetWorld()->GetTimeSeconds();
			CurrentAttackDuration = Duration;
			CurrentAttackProgress = 0.0f;

			// Calculate when attack recovery ends - use CombatConfig value for responsiveness
			float RecoveryTime = Duration * CombatConfig.AttackRecoveryPercent;

			GetWorld()->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(AttackRecoveryTimerHandle, [this]()
			{
				// Attack animation still playing but can chain now
				bIsAttacking = false;
				bComboWindowOpen = true;
				SetCombatState(ECombatState::Recovering);

				// Advance combo index for next attack
				LightComboIndex++;

				// Process any buffered input immediately
				ProcessBufferedInput();

				// Start combo window timer - if no attack within window, reset combo
				GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
				GetWorld()->GetTimerManager().SetTimer(ComboWindowTimerHandle, this,
					&UEquipmentComponent::CloseComboWindow, CombatConfig.ComboWindowTime, false);

			}, RecoveryTime, false);
		}
		else
		{
			bIsAttacking = false;
			SetCombatState(ECombatState::Idle);
		}
	}
}

void UEquipmentComponent::HeavyAttack()
{
	// Don't attack if guarding or in bad state
	if (bIsGuarding || CurrentCombatState == ECombatState::Staggered ||
		CurrentCombatState == ECombatState::GuardBroken)
	{
		return;
	}

	// If currently attacking, try to buffer the input
	if (bIsAttacking && !bComboWindowOpen)
	{
		BufferInput(EBufferedInputType::HeavyAttack);
		return;
	}

	// Check stamina cost
	if (HealthComponent && !HealthComponent->HasStamina(CombatConfig.HeavyAttackStaminaCost))
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* MontageToPlay = nullptr;
	int32 MaxComboCount = 0;

	// Check if we have a weapon equipped (and not stowed)
	bool bHasWeaponEquipped = !bWeaponsStowed && CurrentPrimaryWeaponType != EWeaponType::None;

	if (bHasWeaponEquipped)
	{
		// Use weapon's heavy attack montage array
		FItemData WeaponData;
		if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, WeaponData))
		{
			MaxComboCount = WeaponData.HeavyAttackMontages.Num();
			if (MaxComboCount > 0)
			{
				// Get current combo montage (loop back to start)
				int32 ComboIdx = HeavyComboIndex % MaxComboCount;
				if (!WeaponData.HeavyAttackMontages[ComboIdx].IsNull())
				{
					MontageToPlay = WeaponData.HeavyAttackMontages[ComboIdx].LoadSynchronous();
				}
			}
		}
	}
	else
	{
		// Unarmed heavy attack combo
		MaxComboCount = UnarmedHeavyAttackMontages.Num();
		if (MaxComboCount > 0)
		{
			int32 ComboIdx = HeavyComboIndex % MaxComboCount;
			if (!UnarmedHeavyAttackMontages[ComboIdx].IsNull())
			{
				MontageToPlay = UnarmedHeavyAttackMontages[ComboIdx].LoadSynchronous();
			}
		}
	}

	if (MontageToPlay)
	{
		// Use stamina
		if (HealthComponent)
		{
			HealthComponent->UseStamina(CombatConfig.HeavyAttackStaminaCost);
		}

		bIsAttacking = true;
		bComboWindowOpen = false;
		SetCombatState(ECombatState::Attacking);
		SetComponentTickEnabled(true); // Enable tick for attack progress tracking

		// Clear combo window timer
		GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle);

		// Face lock-on target before attacking
		FaceLockedTarget();

		// Stop any currently playing montage first
		AnimInstance->StopAllMontages(0.1f);

		float Duration = AnimInstance->Montage_Play(MontageToPlay, 1.0f);

		if (Duration > 0.0f)
		{
			// Track attack progress for dodge cancel
			AttackStartTime = GetWorld()->GetTimeSeconds();
			CurrentAttackDuration = Duration;
			CurrentAttackProgress = 0.0f;

			// Calculate when attack recovery ends - use CombatConfig value for responsiveness
			float RecoveryTime = Duration * CombatConfig.AttackRecoveryPercent;

			GetWorld()->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(AttackRecoveryTimerHandle, [this]()
			{
				// Attack animation still playing but can chain now
				bIsAttacking = false;
				bComboWindowOpen = true;
				SetCombatState(ECombatState::Recovering);

				// Advance combo index for next attack
				HeavyComboIndex++;

				// Process any buffered input immediately
				ProcessBufferedInput();

				// Start combo window timer - if no attack within window, reset combo
				GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
				GetWorld()->GetTimerManager().SetTimer(ComboWindowTimerHandle, this,
					&UEquipmentComponent::CloseComboWindow, CombatConfig.ComboWindowTime, false);

			}, RecoveryTime, false);
		}
		else
		{
			bIsAttacking = false;
			SetCombatState(ECombatState::Idle);
		}
	}
}

void UEquipmentComponent::StartGuard()
{
	// Can't guard while attacking or in certain states
	if (bIsAttacking || CurrentCombatState == ECombatState::Staggered ||
		CurrentCombatState == ECombatState::GuardBroken)
	{
		return;
	}

	// Record when guard was pressed (for tap vs hold detection)
	GuardPressTime = GetWorld()->GetTimeSeconds();

	// Immediately attempt parry on press (tap behavior)
	AttemptParry();
}

void UEquipmentComponent::StopGuard()
{
	float HoldDuration = GetWorld()->GetTimeSeconds() - GuardPressTime;

	// If it was a quick tap (< 0.15s), parry window continues
	// If it was a hold, transition out of blocking
	if (HoldDuration >= 0.15f || CurrentCombatState == ECombatState::Blocking)
	{
		bIsGuarding = false;
		if (CurrentCombatState == ECombatState::Blocking || CurrentCombatState == ECombatState::Parrying)
		{
			// Stop block montage when releasing guard
			StopBlockMontage();
			SetCombatState(ECombatState::Idle);
		}
	}
	else
	{
		// Was a tap - let the parry window continue, but mark not guarding
		// The parry window timer will handle state transition
		bIsGuarding = false;
	}
}

void UEquipmentComponent::ResetCombo()
{
	LightComboIndex = 0;
	HeavyComboIndex = 0;
	bComboWindowOpen = false;
	bIsAttacking = false;

	// Clear timers
	GetWorld()->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle);

	// Stop any playing attack montage
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (OwnerMesh)
	{
		UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->StopAllMontages(0.2f);
		}
	}
}

void UEquipmentComponent::CloseComboWindow()
{
	LightComboIndex = 0;
	HeavyComboIndex = 0;
	bComboWindowOpen = false;

	// Return to idle state
	if (CurrentCombatState == ECombatState::Recovering)
	{
		SetCombatState(ECombatState::Idle);
	}

	// Clear attack progress
	CurrentAttackProgress = 0.0f;
	CurrentAttackDuration = 0.0f;
}

void UEquipmentComponent::RestoreMovement()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void UEquipmentComponent::FaceLockedTarget()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Get the player controller and pawn - handle both ownership patterns
	APlayerController* PC = nullptr;
	APawn* OwnerPawn = nullptr;

	// Check if owner is a PlayerController
	PC = Cast<APlayerController>(Owner);
	if (PC)
	{
		OwnerPawn = PC->GetPawn();
	}
	else
	{
		// Owner is a Pawn
		OwnerPawn = Cast<APawn>(Owner);
		if (OwnerPawn)
		{
			PC = Cast<APlayerController>(OwnerPawn->GetController());
		}
	}

	// Need both PC and Pawn to proceed
	if (!PC || !OwnerPawn)
	{
		return;
	}

	// Find lock-on component on controller
	ULockOnComponent* LockOnComp = PC->FindComponentByClass<ULockOnComponent>();
	if (!LockOnComp || !LockOnComp->IsLockedOn())
	{
		return;
	}

	AActor* Target = LockOnComp->GetCurrentTarget();
	if (!Target)
	{
		return;
	}

	// Calculate direction to target - use pawn location since that's what we rotate
	FVector OwnerLocation = OwnerPawn->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector DirectionToTarget = TargetLocation - OwnerLocation;
	DirectionToTarget.Z = 0.0f; // Keep rotation on horizontal plane
	DirectionToTarget.Normalize();

	if (!DirectionToTarget.IsNearlyZero())
	{
		FRotator NewRotation = DirectionToTarget.Rotation();
		OwnerPawn->SetActorRotation(NewRotation);
	}
}

// ==================== Combat State Management ====================

void UEquipmentComponent::SetCombatState(ECombatState NewState)
{
	if (CurrentCombatState != NewState)
	{
		ECombatState OldState = CurrentCombatState;
		CurrentCombatState = NewState;
		OnCombatStateChanged.Broadcast(NewState, OldState);
	}
}

// ==================== Parry System ====================

bool UEquipmentComponent::CanParry() const
{
	// Check if currently in a state where we can parry
	if (bIsAttacking || CurrentCombatState == ECombatState::Staggered ||
		CurrentCombatState == ECombatState::GuardBroken)
	{
		return false;
	}

	// Check if we have a weapon/shield that can parry
	FItemData OffHandData;
	if (GetEquippedItemData(EEquipmentSlot::OffHand, OffHandData) && OffHandData.bCanParry)
	{
		return true;
	}

	FItemData PrimaryData;
	if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryData) && PrimaryData.bCanParry)
	{
		return true;
	}

	// Can always parry unarmed (just less effective timing)
	return true;
}

void UEquipmentComponent::AttemptParry()
{
	if (!CanParry())
	{
		return;
	}

	// Check stamina cost
	if (HealthComponent && !HealthComponent->HasStamina(CombatConfig.ParryStaminaCost))
	{
		return;
	}

	// Use stamina
	if (HealthComponent)
	{
		HealthComponent->UseStamina(CombatConfig.ParryStaminaCost);
	}

	// Set parry state
	SetCombatState(ECombatState::Parrying);
	bIsGuarding = true;

	// Play parry montage
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (OwnerMesh)
	{
		UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
		if (AnimInstance)
		{
			// Try to get parry montage from equipped off-hand first, then primary
			UAnimMontage* ParryMontage = nullptr;
			FItemData OffHandData;
			if (GetEquippedItemData(EEquipmentSlot::OffHand, OffHandData) && !OffHandData.ParryMontage.IsNull())
			{
				ParryMontage = OffHandData.ParryMontage.LoadSynchronous();
			}
			if (!ParryMontage)
			{
				FItemData PrimaryData;
				if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryData) && !PrimaryData.ParryMontage.IsNull())
				{
					ParryMontage = PrimaryData.ParryMontage.LoadSynchronous();
				}
			}

			if (ParryMontage)
			{
				AnimInstance->Montage_Play(ParryMontage);
			}
		}
	}

	// Open parry window
	OpenParryWindow();
}

void UEquipmentComponent::OpenParryWindow()
{
	bInParryWindow = true;

	// Set timer to close parry window
	GetWorld()->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(ParryWindowTimerHandle, this,
		&UEquipmentComponent::CloseParryWindow, CombatConfig.ParryWindowDuration, false);
}

void UEquipmentComponent::CloseParryWindow()
{
	bInParryWindow = false;

	// If we didn't parry anything, transition to blocking or idle
	if (CurrentCombatState == ECombatState::Parrying)
	{
		if (bIsGuarding)
		{
			SetCombatState(ECombatState::Blocking);

			// Play block montage if holding guard
			PlayBlockMontage();
		}
		else
		{
			SetCombatState(ECombatState::Idle);
		}
	}
}

void UEquipmentComponent::PlayBlockMontage()
{
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// Try to get block montage from equipped off-hand first (shield), then primary weapon
	UAnimMontage* BlockMontage = nullptr;

	FItemData OffHandData;
	if (GetEquippedItemData(EEquipmentSlot::OffHand, OffHandData) && OffHandData.bCanBlock && !OffHandData.BlockMontage.IsNull())
	{
		BlockMontage = OffHandData.BlockMontage.LoadSynchronous();
	}

	if (!BlockMontage)
	{
		FItemData PrimaryData;
		if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryData) && PrimaryData.bCanBlock && !PrimaryData.BlockMontage.IsNull())
		{
			BlockMontage = PrimaryData.BlockMontage.LoadSynchronous();
		}
	}

	if (BlockMontage)
	{
		// Stop any current montage first
		AnimInstance->StopAllMontages(0.15f);
		AnimInstance->Montage_Play(BlockMontage);
	}
}

void UEquipmentComponent::StopBlockMontage()
{
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// Get the current block montage to stop specifically
	UAnimMontage* BlockMontage = nullptr;

	FItemData OffHandData;
	if (GetEquippedItemData(EEquipmentSlot::OffHand, OffHandData) && !OffHandData.BlockMontage.IsNull())
	{
		BlockMontage = OffHandData.BlockMontage.LoadSynchronous();
	}

	if (!BlockMontage)
	{
		FItemData PrimaryData;
		if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryData) && !PrimaryData.BlockMontage.IsNull())
		{
			BlockMontage = PrimaryData.BlockMontage.LoadSynchronous();
		}
	}

	if (BlockMontage && AnimInstance->Montage_IsPlaying(BlockMontage))
	{
		AnimInstance->Montage_Stop(0.2f, BlockMontage);
	}
}

void UEquipmentComponent::OnParrySuccessful(AActor* AttackingActor)
{
	// Close parry window immediately
	GetWorld()->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
	bInParryWindow = false;

	// Set success state
	SetCombatState(ECombatState::ParrySuccess);
	ParriedTarget = AttackingActor;
	bCanRiposte = true;

	// Play parry sound from equipped off-hand or primary
	FItemData ParryingItemData;
	if (GetEquippedItemData(EEquipmentSlot::OffHand, ParryingItemData) && !ParryingItemData.ParrySound.IsNull())
	{
		USoundBase* ParrySFX = ParryingItemData.ParrySound.LoadSynchronous();
		if (ParrySFX)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ParrySFX, GetOwner()->GetActorLocation());
		}
	}
	else if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, ParryingItemData) && !ParryingItemData.ParrySound.IsNull())
	{
		USoundBase* ParrySFX = ParryingItemData.ParrySound.LoadSynchronous();
		if (ParrySFX)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ParrySFX, GetOwner()->GetActorLocation());
		}
	}

	// Broadcast events
	OnParrySuccess.Broadcast(AttackingActor);
	OnRiposteAvailable.Broadcast(true);

	// Play parry success montage
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (OwnerMesh)
	{
		UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
		if (AnimInstance)
		{
			UAnimMontage* SuccessMontage = nullptr;
			FItemData OffHandData;
			if (GetEquippedItemData(EEquipmentSlot::OffHand, OffHandData) && !OffHandData.ParrySuccessMontage.IsNull())
			{
				SuccessMontage = OffHandData.ParrySuccessMontage.LoadSynchronous();
			}
			if (!SuccessMontage)
			{
				FItemData PrimaryData;
				if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryData) && !PrimaryData.ParrySuccessMontage.IsNull())
				{
					SuccessMontage = PrimaryData.ParrySuccessMontage.LoadSynchronous();
				}
			}

			if (SuccessMontage)
			{
				AnimInstance->Montage_Play(SuccessMontage);
			}
		}
	}

	// Start riposte window timer
	GetWorld()->GetTimerManager().ClearTimer(RiposteWindowTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RiposteWindowTimerHandle, this,
		&UEquipmentComponent::EndRiposteWindow, CombatConfig.RiposteWindowDuration, false);
}

void UEquipmentComponent::EndRiposteWindow()
{
	bCanRiposte = false;
	ParriedTarget = nullptr;
	OnRiposteAvailable.Broadcast(false);

	if (CurrentCombatState == ECombatState::ParrySuccess)
	{
		SetCombatState(ECombatState::Idle);
	}
}

bool UEquipmentComponent::PerformRiposte()
{
	if (!bCanRiposte || !ParriedTarget)
	{
		return false;
	}

	// Clear riposte window
	GetWorld()->GetTimerManager().ClearTimer(RiposteWindowTimerHandle);
	bCanRiposte = false;

	// Face the target
	AActor* Owner = GetOwner();
	if (Owner && ParriedTarget)
	{
		FVector DirectionToTarget = ParriedTarget->GetActorLocation() - Owner->GetActorLocation();
		DirectionToTarget.Z = 0.0f;
		DirectionToTarget.Normalize();
		if (!DirectionToTarget.IsNearlyZero())
		{
			Owner->SetActorRotation(DirectionToTarget.Rotation());
		}
	}

	// Play riposte montage
	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (OwnerMesh)
	{
		UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
		if (AnimInstance)
		{
			UAnimMontage* RiposteMontage = nullptr;
			FItemData PrimaryData;
			if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, PrimaryData) && !PrimaryData.RiposteMontage.IsNull())
			{
				RiposteMontage = PrimaryData.RiposteMontage.LoadSynchronous();
			}

			if (RiposteMontage)
			{
				AnimInstance->StopAllMontages(0.1f);
				AnimInstance->Montage_Play(RiposteMontage);
			}
		}
	}

	// Set riposting state (distinct from normal attacking for combat feedback)
	SetCombatState(ECombatState::Riposting);
	bIsAttacking = true;
	SetComponentTickEnabled(true); // Enable tick for attack progress tracking

	// Clear parried target after use
	AActor* Target = ParriedTarget;
	ParriedTarget = nullptr;
	OnRiposteAvailable.Broadcast(false);

	return true;
}

// ==================== Damage Modification (Block/Parry) ====================

FDamageModifierResult UEquipmentComponent::ModifyIncomingDamage(float IncomingDamage, AActor* DamageCauser)
{
	FDamageModifierResult Result;
	Result.ModifiedDamage = IncomingDamage;

	// Check for parry first (takes priority)
	if (bInParryWindow)
	{
		Result.bWasParried = true;
		Result.ModifiedDamage = 0.0f;
		OnParrySuccessful(DamageCauser);
		return Result;
	}

	// Check for blocking
	if (bIsGuarding && (CurrentCombatState == ECombatState::Blocking || CurrentCombatState == ECombatState::Parrying))
	{
		Result.bWasBlocked = true;

		// Get block stability and sounds from equipped shield/weapon
		float Stability = 50.0f; // Default
		FItemData BlockingItemData;
		bool bHasBlockingItem = false;

		if (GetEquippedItemData(EEquipmentSlot::OffHand, BlockingItemData))
		{
			Stability = BlockingItemData.BlockStability;
			bHasBlockingItem = true;
		}
		else if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, BlockingItemData))
		{
			Stability = BlockingItemData.BlockStability;
			bHasBlockingItem = true;
		}

		// Calculate stamina drain: Damage * Multiplier * (1 - Stability/100)
		Result.StaminaDrain = IncomingDamage * CombatConfig.BlockStaminaDrainMultiplier * (1.0f - Stability / 100.0f);

		// Apply damage reduction
		Result.ModifiedDamage = IncomingDamage * (1.0f - CombatConfig.BlockDamageReduction);

		// Check for guard break
		if (HealthComponent && HealthComponent->GetStamina() < Result.StaminaDrain)
		{
			Result.bCausedGuardBreak = true;
			Result.ModifiedDamage = IncomingDamage * 0.5f; // Take 50% damage on guard break

			// Play guard break sound
			if (bHasBlockingItem && !BlockingItemData.GuardBreakSound.IsNull())
			{
				USoundBase* GuardBreakSFX = BlockingItemData.GuardBreakSound.LoadSynchronous();
				if (GuardBreakSFX)
				{
					UGameplayStatics::PlaySoundAtLocation(this, GuardBreakSFX, GetOwner()->GetActorLocation());
				}
			}

			// Enter guard broken state
			SetCombatState(ECombatState::GuardBroken);
			bIsGuarding = false;

			// Recovery timer for guard break
			GetWorld()->GetTimerManager().SetTimer(AttackRecoveryTimerHandle, [this]()
			{
				if (CurrentCombatState == ECombatState::GuardBroken)
				{
					SetCombatState(ECombatState::Idle);
				}
			}, CombatConfig.GuardBreakRecoveryTime, false);
		}
		else
		{
			// Successfully blocked - play block sound
			if (bHasBlockingItem && !BlockingItemData.BlockSound.IsNull())
			{
				USoundBase* BlockSFX = BlockingItemData.BlockSound.LoadSynchronous();
				if (BlockSFX)
				{
					UGameplayStatics::PlaySoundAtLocation(this, BlockSFX, GetOwner()->GetActorLocation());
				}
			}
		}

		// Drain stamina
		if (HealthComponent)
		{
			HealthComponent->UseStamina(Result.StaminaDrain);
		}
	}

	return Result;
}

// ==================== Input Buffer System ====================

void UEquipmentComponent::BufferInput(EBufferedInputType InputType)
{
	if (InputType == EBufferedInputType::None)
	{
		return;
	}

	// Only buffer if we're in a state that can accept it later
	if (bIsAttacking || CurrentCombatState == ECombatState::Recovering)
	{
		BufferedInput.Set(InputType, GetWorld()->GetTimeSeconds());
	}
	else
	{
		// Not busy, execute immediately
		ExecuteBufferedInput(InputType);
	}
}

bool UEquipmentComponent::CanAcceptBufferedInput() const
{
	// Can accept during combo window or when idle
	return bComboWindowOpen || CurrentCombatState == ECombatState::Idle || CurrentCombatState == ECombatState::Blocking;
}

void UEquipmentComponent::ProcessBufferedInput()
{
	if (!BufferedInput.bIsValid)
	{
		return;
	}

	// Check if buffer is still valid (not expired)
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - BufferedInput.TimeBuffered > CombatConfig.InputBufferWindow)
	{
		BufferedInput.Clear();
		return;
	}

	// Check if we can accept input now
	if (CanAcceptBufferedInput())
	{
		EBufferedInputType TypeToExecute = BufferedInput.InputType;
		BufferedInput.Clear();
		ExecuteBufferedInput(TypeToExecute);
	}
}

void UEquipmentComponent::ExecuteBufferedInput(EBufferedInputType InputType)
{
	switch (InputType)
	{
		case EBufferedInputType::LightAttack:
			LightAttack();
			break;
		case EBufferedInputType::HeavyAttack:
			HeavyAttack();
			break;
		case EBufferedInputType::Dodge:
			// Dodge is handled by the controller, but we reset combo
			ResetCombo();
			break;
		case EBufferedInputType::Parry:
			AttemptParry();
			break;
		default:
			break;
	}
}

bool UEquipmentComponent::CanDodgeCancel() const
{
	if (!bIsAttacking)
	{
		return true;
	}

	// Can dodge cancel if we're past the dodge cancel window
	return CurrentAttackProgress >= CombatConfig.DodgeCancelWindow;
}

void UEquipmentComponent::UpdateAttackProgress()
{
	if (!bIsAttacking || CurrentAttackDuration <= 0.0f)
	{
		CurrentAttackProgress = 0.0f;
		return;
	}

	float ElapsedTime = GetWorld()->GetTimeSeconds() - AttackStartTime;
	CurrentAttackProgress = FMath::Clamp(ElapsedTime / CurrentAttackDuration, 0.0f, 1.0f);

	// Process any buffered input when we can
	if (CurrentAttackProgress >= CombatConfig.AttackRecoveryPercent)
	{
		ProcessBufferedInput();
	}
}

// ==================== Drop Attack ====================

bool UEquipmentComponent::CanDropAttack() const
{
	// Can't drop attack if already attacking or in bad states
	if (bIsAttacking || bIsDropAttacking || bIsGuarding)
	{
		return false;
	}

	if (CurrentCombatState == ECombatState::Staggered || CurrentCombatState == ECombatState::GuardBroken)
	{
		return false;
	}

	// Check if we're airborne
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		// Try getting from controller
		APlayerController* PC = Cast<APlayerController>(GetOwner());
		if (PC)
		{
			Character = Cast<ACharacter>(PC->GetPawn());
		}
	}

	if (!Character || !Character->GetCharacterMovement())
	{
		return false;
	}

	// Must be falling
	if (!Character->GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	// Check minimum fall distance
	float CurrentHeight = Character->GetActorLocation().Z;
	float FallDistance = DropAttackStartHeight - CurrentHeight;

	if (FallDistance < CombatConfig.MinDropAttackHeight)
	{
		return false;
	}

	// Check stamina
	if (HealthComponent && !HealthComponent->HasStamina(CombatConfig.DropAttackStaminaCost))
	{
		return false;
	}

	return true;
}

bool UEquipmentComponent::DropAttack()
{
	if (!CanDropAttack())
	{
		return false;
	}

	// Use stamina
	if (HealthComponent)
	{
		HealthComponent->UseStamina(CombatConfig.DropAttackStaminaCost);
	}

	// Get the montage we'll play on landing
	UAnimMontage* MontageToPlay = nullptr;

	// Check if we have a weapon equipped
	bool bHasWeaponEquipped = !bWeaponsStowed && CurrentPrimaryWeaponType != EWeaponType::None;

	if (bHasWeaponEquipped)
	{
		// Use weapon's drop attack montage
		FItemData WeaponData;
		if (GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, WeaponData))
		{
			if (!WeaponData.DropAttackMontage.IsNull())
			{
				MontageToPlay = WeaponData.DropAttackMontage.LoadSynchronous();
			}
		}
	}

	// Fallback to unarmed drop attack
	if (!MontageToPlay && !UnarmedDropAttackMontage.IsNull())
	{
		MontageToPlay = UnarmedDropAttackMontage.LoadSynchronous();
	}

	if (!MontageToPlay)
	{
		return false;
	}

	// Set states - we're queued for drop attack, animation plays on landing
	bIsDropAttacking = true;
	bIsDropAttackFalling = true;
	CurrentDropAttackMontage = MontageToPlay;
	SetCombatState(ECombatState::DropAttacking);

	// Reset combos
	LightComboIndex = 0;
	HeavyComboIndex = 0;

	// Don't play animation yet - it plays when we land
	// Just mark that we're ready for drop attack

	return true;
}

void UEquipmentComponent::OnDropAttackLand()
{
	if (!bIsDropAttackFalling || !bIsDropAttacking)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// Calculate final damage multiplier based on total fall distance
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		APlayerController* PC = Cast<APlayerController>(GetOwner());
		if (PC)
		{
			Character = Cast<ACharacter>(PC->GetPawn());
		}
	}

	if (Character)
	{
		float CurrentHeight = Character->GetActorLocation().Z;
		float FallDistance = DropAttackStartHeight - CurrentHeight;

		// Interpolate damage multiplier based on fall distance
		float HeightRatio = FMath::Clamp(
			(FallDistance - CombatConfig.MinDropAttackHeight) /
			(CombatConfig.MaxDropAttackHeight - CombatConfig.MinDropAttackHeight),
			0.0f, 1.0f
		);

		CurrentDropAttackMultiplier = FMath::Lerp(
			CombatConfig.DropAttackDamageMultiplier,
			CombatConfig.MaxDropAttackDamageMultiplier,
			HeightRatio
		);
	}

	// No longer in falling state
	bIsDropAttackFalling = false;
	bIsAttacking = true;

	// Now play the full drop attack montage on landing
	if (CurrentDropAttackMontage)
	{
		// Stop any current montage
		AnimInstance->StopAllMontages(0.1f);

		float Duration = AnimInstance->Montage_Play(CurrentDropAttackMontage, 1.0f);

		if (Duration > 0.0f)
		{
			// Track attack progress
			AttackStartTime = GetWorld()->GetTimeSeconds();
			CurrentAttackDuration = Duration;
			CurrentAttackProgress = 0.0f;
			SetComponentTickEnabled(true);

			// Set recovery timer
			float RecoveryTime = Duration * CombatConfig.AttackRecoveryPercent;

			GetWorld()->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(AttackRecoveryTimerHandle, [this]()
			{
				bIsAttacking = false;
				bIsDropAttacking = false;
				bIsDropAttackFalling = false;
				CurrentDropAttackMontage = nullptr;
				CurrentDropAttackMultiplier = 1.0f;
				SetCombatState(ECombatState::Idle);
			}, RecoveryTime, false);
		}
		else
		{
			// Failed to play, reset state
			bIsAttacking = false;
			bIsDropAttacking = false;
			CurrentDropAttackMontage = nullptr;
			CurrentDropAttackMultiplier = 1.0f;
			SetCombatState(ECombatState::Idle);
		}
	}
}

void UEquipmentComponent::StartDropAttackTracking()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		APlayerController* PC = Cast<APlayerController>(GetOwner());
		if (PC)
		{
			Character = Cast<ACharacter>(PC->GetPawn());
		}
	}

	if (Character)
	{
		DropAttackStartHeight = Character->GetActorLocation().Z;
	}
}

void UEquipmentComponent::StopDropAttackTracking()
{
	// If we're in the falling portion of a drop attack, trigger the land
	if (bIsDropAttackFalling)
	{
		OnDropAttackLand();
	}

	// Reset tracking (but don't reset multiplier if attack is still playing)
	if (!bIsDropAttacking)
	{
		DropAttackStartHeight = 0.0f;
		CurrentDropAttackMultiplier = 1.0f;
	}
}

float UEquipmentComponent::GetDropAttackDamageMultiplier() const
{
	if (bIsDropAttacking)
	{
		return CurrentDropAttackMultiplier;
	}
	return 1.0f;
}
