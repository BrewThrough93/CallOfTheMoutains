// CallOfTheMoutains - Save Game Data Structure

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ItemTypes.h"
#include "DayNightTypes.h"
#include "COTMSaveGame.generated.h"

/**
 * Saved hotbar slot data
 */
USTRUCT(BlueprintType)
struct FSavedHotbarSlot
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	TArray<FName> AssignedItems;

	UPROPERTY(SaveGame)
	int32 CurrentIndex = 0;
};

/**
 * Main save game class for CallOfTheMoutains
 * Stores player location, inventory, and equipment
 */
UCLASS()
class CALLOFTHEMOUTAINS_API UCOTMSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UCOTMSaveGame();

	// ==================== Save Slot Info ====================

	/** Save slot name */
	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FString SaveSlotName;

	/** User index for the save */
	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	int32 UserIndex;

	/** Timestamp when saved */
	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FDateTime SaveTimestamp;

	// ==================== Player Transform ====================

	/** Player world location */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Player")
	FVector PlayerLocation;

	/** Player world rotation */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Player")
	FRotator PlayerRotation;

	// ==================== Inventory ====================

	/** All inventory slots */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Inventory")
	TArray<FInventorySlot> InventorySlots;

	// ==================== Equipment ====================

	/** Equipped items by slot */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Equipment")
	TMap<EEquipmentSlot, FName> EquippedItems;

	/** Hotbar slot assignments */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Equipment")
	TMap<EHotbarSlot, FSavedHotbarSlot> HotbarSlots;

	/** Are weapons currently stowed */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Equipment")
	bool bWeaponsStowed;

	// ==================== Player Stats ====================

	/** Current health (percentage of max) */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Stats")
	float HealthPercent;

	/** Current stamina (percentage of max) */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|Stats")
	float StaminaPercent;

	// ==================== Day/Night Cycle ====================

	/** Current game time */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|DayNight")
	FCOTMGameTime CurrentGameTime;

	/** Current weather type */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|DayNight")
	EWeatherType CurrentWeather = EWeatherType::Clear;

	/** Has day/night state been saved? */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveGame|DayNight")
	bool bHasDayNightData = false;

	// ==================== Helper Functions ====================

	/** Check if this save has valid data */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool IsValidSave() const { return !SaveSlotName.IsEmpty(); }
};
