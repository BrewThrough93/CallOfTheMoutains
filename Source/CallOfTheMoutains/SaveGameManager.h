// CallOfTheMoutains - Save Game Manager Component

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "COTMSaveGame.h"
#include "ItemTypes.h"
#include "SaveGameManager.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class UHealthComponent;

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameSaved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveFailed, const FString&, Reason);

/**
 * Manages saving and loading game state
 * Attach to PlayerController for automatic save/load functionality
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API USaveGameManager : public UActorComponent
{
	GENERATED_BODY()

public:
	USaveGameManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ==================== Configuration ====================

	/** Save slot name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame|Config")
	FString SaveSlotName = TEXT("COTMSave");

	/** User index for save */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame|Config")
	int32 UserIndex = 0;

	/** Auto-save enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame|Config")
	bool bAutoSaveEnabled = true;

	/** Auto-save interval in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame|Config", meta = (ClampMin = "30.0", EditCondition = "bAutoSaveEnabled"))
	float AutoSaveInterval = 60.0f;

	/** Load save on begin play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame|Config")
	bool bLoadOnBeginPlay = true;

	/** Save on end play (quitting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame|Config")
	bool bSaveOnEndPlay = true;

	/** Levels where saving/loading is disabled (sandbox/test levels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame|Config")
	TArray<FString> ExcludedLevels = { TEXT("Lvl_ThirdPerson"), TEXT("ThirdPersonMap"), TEXT("ThirdPersonExampleMap") };

	// ==================== Events ====================

	/** Called when game is saved successfully */
	UPROPERTY(BlueprintAssignable, Category = "SaveGame|Events")
	FOnGameSaved OnGameSaved;

	/** Called when game is loaded successfully */
	UPROPERTY(BlueprintAssignable, Category = "SaveGame|Events")
	FOnGameLoaded OnGameLoaded;

	/** Called when save fails */
	UPROPERTY(BlueprintAssignable, Category = "SaveGame|Events")
	FOnSaveFailed OnSaveFailed;

	// ==================== Save Functions ====================

	/** Save the current game state */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveGame();

	/** Load the saved game state */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool LoadGame();

	/** Check if a save file exists */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool DoesSaveExist() const;

	/** Delete the save file */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool DeleteSave();

	/** Get the current save data (creates new if none exists) */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	UCOTMSaveGame* GetOrCreateSaveGame();

	/** Force an auto-save now */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void TriggerAutoSave();

protected:
	/** Cached reference to inventory component */
	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	/** Cached reference to equipment component */
	UPROPERTY()
	UEquipmentComponent* EquipmentComponent;

	/** Cached reference to health component */
	UPROPERTY()
	UHealthComponent* HealthComponent;

	/** Current save game object */
	UPROPERTY()
	UCOTMSaveGame* CurrentSaveGame;

	/** Auto-save timer handle */
	FTimerHandle AutoSaveTimerHandle;

	/** Debounce timer for change-triggered saves (prevents rapid saving) */
	FTimerHandle ChangeEventSaveTimerHandle;

	/** Find and cache component references */
	void CacheComponents();

	/** Gather current game state into save object */
	void GatherSaveData(UCOTMSaveGame* SaveObject);

	/** Apply loaded data to game state */
	void ApplySaveData(UCOTMSaveGame* SaveObject);

	/** Start the auto-save timer */
	void StartAutoSaveTimer();

	/** Auto-save callback */
	void OnAutoSaveTimer();

	/** Check if current level is excluded from saving */
	bool IsCurrentLevelExcluded() const;

	/** Bind to inventory/equipment change events for immediate saving */
	void BindToChangeEvents();

	/** Called when inventory changes - triggers save */
	UFUNCTION()
	void OnInventoryChangedCallback();

	/** Called when equipment changes - triggers save */
	UFUNCTION()
	void OnEquipmentChangedCallback(EEquipmentSlot Slot, FName NewItemID);
};
