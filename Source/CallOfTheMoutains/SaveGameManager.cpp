// CallOfTheMoutains - Save Game Manager Implementation

#include "SaveGameManager.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "HealthComponent.h"
#include "DayNightManager.h"
#include "WeatherSystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

USaveGameManager::USaveGameManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USaveGameManager::BeginPlay()
{
	Super::BeginPlay();

	CacheComponents();

	// Skip save/load on excluded levels (sandbox/test levels)
	if (IsCurrentLevelExcluded())
	{
		return;
	}

	// Load existing save if enabled
	if (bLoadOnBeginPlay && DoesSaveExist())
	{
		// Delay load slightly to ensure all components are initialized
		FTimerHandle LoadTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(LoadTimerHandle, [this]()
		{
			LoadGame();
		}, 0.5f, false);
	}

	// Start auto-save timer if enabled
	if (bAutoSaveEnabled)
	{
		StartAutoSaveTimer();
	}

	// Bind to inventory/equipment change events for immediate saving
	// Delay slightly to ensure components are initialized
	FTimerHandle BindTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(BindTimerHandle, [this]()
	{
		BindToChangeEvents();
	}, 0.6f, false);
}

void USaveGameManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear timers
	GetWorld()->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ChangeEventSaveTimerHandle);

	// Save on quit if enabled (skip on excluded levels)
	if (bSaveOnEndPlay && EndPlayReason == EEndPlayReason::Quit && !IsCurrentLevelExcluded())
	{
		SaveGame();
	}

	Super::EndPlay(EndPlayReason);
}

void USaveGameManager::CacheComponents()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	// Get components from pawn (that's where inventory/equipment are)
	if (APawn* Pawn = PC->GetPawn())
	{
		InventoryComponent = Pawn->FindComponentByClass<UInventoryComponent>();
		EquipmentComponent = Pawn->FindComponentByClass<UEquipmentComponent>();
		HealthComponent = Pawn->FindComponentByClass<UHealthComponent>();
	}

	// Also check controller for components (some setups put them there)
	if (!InventoryComponent)
	{
		InventoryComponent = PC->FindComponentByClass<UInventoryComponent>();
	}
	if (!EquipmentComponent)
	{
		EquipmentComponent = PC->FindComponentByClass<UEquipmentComponent>();
	}
}

bool USaveGameManager::SaveGame()
{
	// Skip on excluded levels
	if (IsCurrentLevelExcluded())
	{
		return false;
	}

	// Re-cache components in case they changed
	CacheComponents();

	UCOTMSaveGame* SaveObject = GetOrCreateSaveGame();
	if (!SaveObject)
	{
		OnSaveFailed.Broadcast(TEXT("Failed to create save object"));
		return false;
	}

	// Gather all data
	GatherSaveData(SaveObject);

	// Save to disk
	if (UGameplayStatics::SaveGameToSlot(SaveObject, SaveSlotName, UserIndex))
	{
		OnGameSaved.Broadcast();
		return true;
	}

	OnSaveFailed.Broadcast(TEXT("Failed to write save file"));
	return false;
}

bool USaveGameManager::LoadGame()
{
	// Skip on excluded levels
	if (IsCurrentLevelExcluded())
	{
		return false;
	}

	if (!DoesSaveExist())
	{
		return false;
	}

	// Re-cache components
	CacheComponents();

	// Load from disk
	UCOTMSaveGame* LoadedGame = Cast<UCOTMSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));

	if (!LoadedGame)
	{
		OnSaveFailed.Broadcast(TEXT("Failed to load save file"));
		return false;
	}

	CurrentSaveGame = LoadedGame;

	// Apply loaded data
	ApplySaveData(LoadedGame);

	OnGameLoaded.Broadcast();
	return true;
}

bool USaveGameManager::DoesSaveExist() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex);
}

bool USaveGameManager::DeleteSave()
{
	if (DoesSaveExist())
	{
		return UGameplayStatics::DeleteGameInSlot(SaveSlotName, UserIndex);
	}
	return true;
}

UCOTMSaveGame* USaveGameManager::GetOrCreateSaveGame()
{
	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UCOTMSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UCOTMSaveGame::StaticClass()));
	}
	return CurrentSaveGame;
}

void USaveGameManager::TriggerAutoSave()
{
	SaveGame();
}

void USaveGameManager::GatherSaveData(UCOTMSaveGame* SaveObject)
{
	if (!SaveObject)
	{
		return;
	}

	SaveObject->SaveSlotName = SaveSlotName;
	SaveObject->UserIndex = UserIndex;
	SaveObject->SaveTimestamp = FDateTime::Now();

	// Get player controller
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	// Save player location
	if (APawn* Pawn = PC->GetPawn())
	{
		SaveObject->PlayerLocation = Pawn->GetActorLocation();
		SaveObject->PlayerRotation = Pawn->GetActorRotation();

		// Save health/stamina from health component
		if (HealthComponent)
		{
			SaveObject->HealthPercent = HealthComponent->GetHealthPercent();
			SaveObject->StaminaPercent = HealthComponent->GetStaminaPercent();
		}
	}

	// Save inventory
	if (InventoryComponent)
	{
		SaveObject->InventorySlots = InventoryComponent->GetAllSlots();
	}

	// Save equipment
	if (EquipmentComponent)
	{
		// Get equipped items
		SaveObject->EquippedItems.Empty();
		TArray<EEquipmentSlot> AllSlots = {
			EEquipmentSlot::Helmet, EEquipmentSlot::Chest, EEquipmentSlot::Gloves,
			EEquipmentSlot::Legs, EEquipmentSlot::Boots,
			EEquipmentSlot::PrimaryWeapon, EEquipmentSlot::OffHand,
			EEquipmentSlot::Ring1, EEquipmentSlot::Ring2, EEquipmentSlot::Ring3, EEquipmentSlot::Ring4,
			EEquipmentSlot::Trinket1, EEquipmentSlot::Trinket2, EEquipmentSlot::Trinket3, EEquipmentSlot::Trinket4
		};

		for (EEquipmentSlot Slot : AllSlots)
		{
			FName ItemID = EquipmentComponent->GetEquippedItem(Slot);
			if (!ItemID.IsNone())
			{
				SaveObject->EquippedItems.Add(Slot, ItemID);
			}
		}

		// Save hotbar
		SaveObject->HotbarSlots.Empty();
		TArray<EHotbarSlot> HotbarSlots = {
			EHotbarSlot::Special, EHotbarSlot::PrimaryWeapon,
			EHotbarSlot::OffHand, EHotbarSlot::Consumable
		};

		for (EHotbarSlot HSlot : HotbarSlots)
		{
			FHotbarSlotData SlotData = EquipmentComponent->GetHotbarSlotData(HSlot);
			FSavedHotbarSlot SavedSlot;
			SavedSlot.AssignedItems = SlotData.AssignedItems;
			SavedSlot.CurrentIndex = SlotData.CurrentIndex;
			SaveObject->HotbarSlots.Add(HSlot, SavedSlot);
		}

		// Save weapon stow state
		SaveObject->bWeaponsStowed = EquipmentComponent->AreWeaponsStowed();
	}

	// Save day/night cycle state
	ADayNightManager* DayNightManager = ADayNightManager::GetDayNightManager(this);
	if (DayNightManager)
	{
		EWeatherType Weather;
		DayNightManager->GetSaveData(SaveObject->CurrentGameTime, Weather);
		SaveObject->CurrentWeather = Weather;
		SaveObject->bHasDayNightData = true;
	}
}

void USaveGameManager::ApplySaveData(UCOTMSaveGame* SaveObject)
{
	if (!SaveObject)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	// Apply player location
	if (APawn* Pawn = PC->GetPawn())
	{
		// Only teleport if we have a valid saved location
		if (!SaveObject->PlayerLocation.IsNearlyZero())
		{
			Pawn->SetActorLocation(SaveObject->PlayerLocation);
			Pawn->SetActorRotation(SaveObject->PlayerRotation);
		}

		// Restore health and stamina
		if (HealthComponent)
		{
			float MaxHealth = HealthComponent->GetMaxHealth();
			float MaxStamina = HealthComponent->GetMaxStamina();
			HealthComponent->SetHealth(MaxHealth * SaveObject->HealthPercent);
			HealthComponent->SetStamina(MaxStamina * SaveObject->StaminaPercent);
		}
	}

	// IMPORTANT: Apply inventory FIRST before equipment to prevent item duplication
	// This ensures the inventory state is correct before we equip items
	if (InventoryComponent)
	{
		InventoryComponent->SetInventorySlots(SaveObject->InventorySlots);
	}

	// Apply equipment AFTER inventory is loaded
	if (EquipmentComponent)
	{
		// Unequip all first (silently, without returning items to inventory)
		TArray<EEquipmentSlot> AllSlots = {
			EEquipmentSlot::Helmet, EEquipmentSlot::Chest, EEquipmentSlot::Gloves,
			EEquipmentSlot::Legs, EEquipmentSlot::Boots,
			EEquipmentSlot::PrimaryWeapon, EEquipmentSlot::OffHand,
			EEquipmentSlot::Ring1, EEquipmentSlot::Ring2, EEquipmentSlot::Ring3, EEquipmentSlot::Ring4,
			EEquipmentSlot::Trinket1, EEquipmentSlot::Trinket2, EEquipmentSlot::Trinket3, EEquipmentSlot::Trinket4
		};

		for (EEquipmentSlot Slot : AllSlots)
		{
			EquipmentComponent->UnequipSlot(Slot);
		}

		// Equip saved items (pass true to skip inventory check - items are already equipped, not in inventory)
		for (const auto& Pair : SaveObject->EquippedItems)
		{
			EquipmentComponent->EquipItemToSlot(Pair.Value, Pair.Key, true);
		}

		// Restore hotbar
		TArray<EHotbarSlot> HotbarSlotTypes = {
			EHotbarSlot::Special, EHotbarSlot::PrimaryWeapon,
			EHotbarSlot::OffHand, EHotbarSlot::Consumable
		};

		for (EHotbarSlot HSlot : HotbarSlotTypes)
		{
			EquipmentComponent->ClearHotbarSlot(HSlot);

			if (const FSavedHotbarSlot* SavedSlot = SaveObject->HotbarSlots.Find(HSlot))
			{
				for (const FName& ItemID : SavedSlot->AssignedItems)
				{
					EquipmentComponent->AssignToHotbar(ItemID, HSlot);
				}
				// Restore the saved current index
				EquipmentComponent->SetHotbarCurrentIndex(HSlot, SavedSlot->CurrentIndex);
			}
		}

		// Restore weapon stow state
		if (SaveObject->bWeaponsStowed && !EquipmentComponent->AreWeaponsStowed())
		{
			EquipmentComponent->StowWeapons();
		}
		else if (!SaveObject->bWeaponsStowed && EquipmentComponent->AreWeaponsStowed())
		{
			EquipmentComponent->DrawWeapons();
		}
	}

	// Restore day/night cycle state
	if (SaveObject->bHasDayNightData)
	{
		ADayNightManager* DayNightManager = ADayNightManager::GetDayNightManager(this);
		if (DayNightManager)
		{
			DayNightManager->LoadSaveData(SaveObject->CurrentGameTime, SaveObject->CurrentWeather);
		}
	}
}

void USaveGameManager::StartAutoSaveTimer()
{
	if (AutoSaveInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutoSaveTimerHandle,
			this,
			&USaveGameManager::OnAutoSaveTimer,
			AutoSaveInterval,
			true // Looping
		);
	}
}

void USaveGameManager::OnAutoSaveTimer()
{
	SaveGame();
}

bool USaveGameManager::IsCurrentLevelExcluded() const
{
	if (!GetWorld())
	{
		return false;
	}

	FString CurrentLevelName = GetWorld()->GetMapName();
	// Remove any streaming prefix (e.g., "UEDPIE_0_")
	CurrentLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	for (const FString& ExcludedLevel : ExcludedLevels)
	{
		if (CurrentLevelName.Contains(ExcludedLevel))
		{
			return true;
		}
	}

	return false;
}

void USaveGameManager::BindToChangeEvents()
{
	// Skip on excluded levels
	if (IsCurrentLevelExcluded())
	{
		return;
	}

	// Re-cache in case components weren't ready yet
	CacheComponents();

	// Bind to inventory changes
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &USaveGameManager::OnInventoryChangedCallback);
	}

	// Bind to equipment changes
	if (EquipmentComponent)
	{
		EquipmentComponent->OnEquipmentChanged.AddDynamic(this, &USaveGameManager::OnEquipmentChangedCallback);
	}
}

void USaveGameManager::OnInventoryChangedCallback()
{
	// Skip on excluded levels
	if (IsCurrentLevelExcluded())
	{
		return;
	}

	// Debounce: If multiple changes happen quickly, only save once after 0.5 seconds
	GetWorld()->GetTimerManager().ClearTimer(ChangeEventSaveTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(ChangeEventSaveTimerHandle, [this]()
	{
		SaveGame();
	}, 0.5f, false);
}

void USaveGameManager::OnEquipmentChangedCallback(EEquipmentSlot Slot, FName NewItemID)
{
	// Skip on excluded levels
	if (IsCurrentLevelExcluded())
	{
		return;
	}

	// Debounce: If multiple changes happen quickly, only save once after 0.5 seconds
	GetWorld()->GetTimerManager().ClearTimer(ChangeEventSaveTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(ChangeEventSaveTimerHandle, [this]()
	{
		SaveGame();
	}, 0.5f, false);
}
