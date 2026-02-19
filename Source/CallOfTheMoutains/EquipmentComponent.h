// CallOfTheMoutains - Equipment Component for managing equipped items and hotbar

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "EquipmentComponent.generated.h"

class UInventoryComponent;
class UDataTable;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UHealthComponent;

// Delegates for equipment changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, EEquipmentSlot, Slot, FName, NewItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHotbarChanged, EHotbarSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEncumbranceChanged, bool, bIsOverEncumbered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponTypeChanged, EWeaponType, NewWeaponType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStateChanged, ECombatState, NewState, ECombatState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParrySuccess, AActor*, ParriedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRiposteAvailable, bool, bAvailable);

/**
 * Hotbar slot data - allows multiple items in rotation
 */
USTRUCT(BlueprintType)
struct FHotbarSlotData
{
	GENERATED_BODY()

	// Items assigned to this hotbar slot (can cycle through)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar")
	TArray<FName> AssignedItems;

	// Current active index
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar")
	int32 CurrentIndex = 0;

	FName GetCurrentItem() const
	{
		if (AssignedItems.IsValidIndex(CurrentIndex))
		{
			return AssignedItems[CurrentIndex];
		}
		return NAME_None;
	}

	void CycleNext()
	{
		if (AssignedItems.Num() > 1)
		{
			CurrentIndex = (CurrentIndex + 1) % AssignedItems.Num();
		}
	}

	void CyclePrevious()
	{
		if (AssignedItems.Num() > 1)
		{
			CurrentIndex = (CurrentIndex - 1 + AssignedItems.Num()) % AssignedItems.Num();
		}
	}
};

/**
 * Manages equipped items in slots and souls-like hotbar system
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ==================== Configuration ====================

	/** Reference to the item data table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	UDataTable* ItemDataTable;

	/** Reference to inventory component (auto-found if not set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	UInventoryComponent* InventoryComponent;

	/** Max items per hotbar slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxHotbarItemsPerSlot = 5;

	// ==================== Weight/Encumbrance ====================

	/** Maximum carry weight before becoming encumbered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weight")
	float MaxCarryWeight = 100.0f;

	/** Current total equipped weight */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Weight")
	float CurrentEquippedWeight = 0.0f;

	/** Is player over-encumbered (can't move)? */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Weight")
	bool bIsOverEncumbered = false;

	// ==================== Socket Names ====================

	/** Socket name for primary weapon (right hand) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Sockets")
	FName PrimaryWeaponSocket = TEXT("weapon_r");

	/** Socket name for off-hand weapon/shield (left hand) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Sockets")
	FName OffHandSocket = TEXT("weapon_l");

	// ==================== Current Weapon State ====================

	/** Current primary weapon type (for animation selection) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Weapons")
	EWeaponType CurrentPrimaryWeaponType = EWeaponType::None;

	/** Current off-hand weapon type */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Weapons")
	EWeaponType CurrentOffHandWeaponType = EWeaponType::None;

	/** Are weapons currently stowed (hidden) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Weapons")
	bool bWeaponsStowed = false;

	/** Is currently guarding (for animation BP) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	bool bIsGuarding = false;

	/** Is currently attacking (prevents spam) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	bool bIsAttacking = false;

	// ==================== Combat Config (Parry/Block/Responsiveness) ====================
	// NOTE: Attack recovery is now configured via CombatConfig.AttackRecoveryPercent below

	/** Combat configuration - all tunable combat parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Combat")
	FCombatConfig CombatConfig;

	/** Current combat state (Idle, Attacking, Blocking, Parrying, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	ECombatState CurrentCombatState = ECombatState::Idle;

	/** Is the parry window currently open? */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	bool bInParryWindow = false;

	/** Can perform a riposte right now? */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	bool bCanRiposte = false;

	/** The actor we successfully parried (target for riposte) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	AActor* ParriedTarget = nullptr;

	/** Buffered input for combat responsiveness */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	FBufferedInput BufferedInput;

	/** Current attack animation progress (0-1) for dodge cancel */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	float CurrentAttackProgress = 0.0f;

	/** Reference to health component (for stamina checks) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	UHealthComponent* HealthComponent = nullptr;

	/** Socket name for stowed primary weapon (on back/hip) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Sockets")
	FName PrimaryWeaponStowSocket = TEXT("weapon_back");

	/** Socket name for stowed off-hand weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Sockets")
	FName OffHandStowSocket = TEXT("shield_back");

	// ==================== Unarmed Combat Montages ====================

	/** Unarmed light attack combo chain (left click with no weapon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Combat")
	TArray<TSoftObjectPtr<UAnimMontage>> UnarmedLightAttackMontages;

	/** Unarmed heavy attack combo chain (right click with no weapon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Combat")
	TArray<TSoftObjectPtr<UAnimMontage>> UnarmedHeavyAttackMontages;

	/** Unarmed drop/plunge attack montage (attack while airborne) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Combat")
	TSoftObjectPtr<UAnimMontage> UnarmedDropAttackMontage;

	// ==================== Combo System ====================

	/** Current light attack combo index */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	int32 LightComboIndex = 0;

	/** Current heavy attack combo index */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	int32 HeavyComboIndex = 0;

	/** Time window to continue combo after attack ends (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Combat", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float ComboWindowTime = 0.8f;

	/** Is combo window currently open? */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	bool bComboWindowOpen = false;

	// ==================== Drop Attack ====================

	/** Is currently performing a drop attack? */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	bool bIsDropAttacking = false;

	/** Is currently in the falling portion of drop attack (holding pose)? */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	bool bIsDropAttackFalling = false;

	/** Height at which falling started (for damage calculation) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	float DropAttackStartHeight = 0.0f;

	/** Current drop attack damage multiplier (based on fall distance) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	float CurrentDropAttackMultiplier = 1.0f;

	/** Currently playing drop attack montage (for resume on land) */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Combat")
	UAnimMontage* CurrentDropAttackMontage = nullptr;

	// ==================== Delegates ====================

	/** Called when equipment changes */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	/** Called when hotbar changes */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnHotbarChanged OnHotbarChanged;

	/** Called when total stats change */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnStatsChanged OnStatsChanged;

	/** Called when encumbrance state changes */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEncumbranceChanged OnEncumbranceChanged;

	/** Called when weapon type changes (for animation blueprint) */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnWeaponTypeChanged OnWeaponTypeChanged;

	/** Called when combat state changes */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnCombatStateChanged OnCombatStateChanged;

	/** Called when a parry is successful */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnParrySuccess OnParrySuccess;

	/** Called when riposte becomes available/unavailable */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnRiposteAvailable OnRiposteAvailable;

	// ==================== Equipment Functions ====================

	/** Equip item to appropriate slot */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItem(FName ItemID);

	/** Equip item to specific slot */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItemToSlot(FName ItemID, EEquipmentSlot Slot, bool bFromSaveLoad = false);

	/** Unequip item from slot */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipSlot(EEquipmentSlot Slot);

	/** Get equipped item in slot */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FName GetEquippedItem(EEquipmentSlot Slot) const;

	/** Get item data for equipped item */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool GetEquippedItemData(EEquipmentSlot Slot, FItemData& OutItemData) const;

	/** Check if slot has item equipped */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool IsSlotEquipped(EEquipmentSlot Slot) const;

	/** Get total stats from all equipped items */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FItemStats GetTotalEquippedStats() const;

	/** Get current equipped weight */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weight")
	float GetCurrentWeight() const { return CurrentEquippedWeight; }

	/** Get max carry weight */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weight")
	float GetMaxWeight() const { return MaxCarryWeight; }

	/** Check if over-encumbered */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weight")
	bool IsOverEncumbered() const { return bIsOverEncumbered; }

	/** Get weight ratio (0.0 - 1.0+) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weight")
	float GetWeightRatio() const { return MaxCarryWeight > 0 ? CurrentEquippedWeight / MaxCarryWeight : 0.0f; }

	// ==================== Hotbar Functions ====================

	/** Assign item to hotbar slot */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	bool AssignToHotbar(FName ItemID, EHotbarSlot HotbarSlot);

	/** Remove item from hotbar slot */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	bool RemoveFromHotbar(FName ItemID, EHotbarSlot HotbarSlot);

	/** Clear all items from hotbar slot */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void ClearHotbarSlot(EHotbarSlot HotbarSlot);

	/** Cycle to next item in hotbar slot */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void CycleHotbarNext(EHotbarSlot HotbarSlot);

	/** Cycle to previous item in hotbar slot */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void CycleHotbarPrevious(EHotbarSlot HotbarSlot);

	/** Get current item in hotbar slot */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	FName GetCurrentHotbarItem(EHotbarSlot HotbarSlot) const;

	/** Get hotbar slot data */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	FHotbarSlotData GetHotbarSlotData(EHotbarSlot HotbarSlot) const;

	/** Set hotbar current index directly (for save/load) */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void SetHotbarCurrentIndex(EHotbarSlot HotbarSlot, int32 Index);

	/** Use current item in consumable hotbar slot */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	bool UseConsumable();

	/** Use current special item */
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	bool UseSpecialItem();

	// ==================== Toggle Item Functions ====================

	/** Check if a toggle item is currently active */
	UFUNCTION(BlueprintCallable, Category = "Hotbar|Toggle")
	bool IsToggleItemActive(FName ItemID) const;

	/** Get the spawned actor for a toggle item */
	UFUNCTION(BlueprintCallable, Category = "Hotbar|Toggle")
	AActor* GetToggleItemActor(FName ItemID) const;

	// ==================== Weapon Functions ====================

	/** Get currently equipped primary weapon */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	FName GetPrimaryWeapon() const;

	/** Get currently equipped off-hand item */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	FName GetOffHandItem() const;

	/** Swap to next primary weapon in hotbar */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	void CyclePrimaryWeapon();

	/** Swap to next off-hand item in hotbar */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	void CycleOffHand();

	/** Stow weapons (move to back sockets) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	void StowWeapons();

	/** Draw weapons (move to hand sockets) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	void DrawWeapons();

	/** Toggle stow/draw state */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	void ToggleWeaponStow();

	/** Check if weapons are stowed */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	bool AreWeaponsStowed() const { return bWeaponsStowed; }

	/** Get current weapon type for animation selection
	 * Shield in off-hand overrides primary weapon type (for Sword+Shield animation sets) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	EWeaponType GetCurrentWeaponType() const
	{
		if (bWeaponsStowed) return EWeaponType::None;
		// Shield overrides primary for animation selection
		if (CurrentOffHandWeaponType == EWeaponType::Shield) return EWeaponType::Shield;
		return CurrentPrimaryWeaponType;
	}

	/** Play equip or unequip montage for a weapon */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapons")
	void PlayWeaponMontage(const FItemData& ItemData, bool bEquipping);

	// ==================== Combat Functions ====================

	/** Perform light attack (left click) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void LightAttack();

	/** Perform heavy attack (right click) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void HeavyAttack();

	/** Start guarding (Q key pressed) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void StartGuard();

	/** Stop guarding (Q key released) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void StopGuard();

	/** Check if currently guarding (for Animation BP) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool IsGuarding() const { return bIsGuarding; }

	/** Check if currently attacking (for Animation BP) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool IsAttacking() const { return bIsAttacking; }

	/** Reset combo chain (call on dodge/roll) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void ResetCombo();

	/** Perform drop/plunge attack (call when attacking while airborne) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool DropAttack();

	/** Check if player can perform a drop attack (airborne + sufficient height) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool CanDropAttack() const;

	/** Start tracking fall for drop attack (call when character starts falling) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void StartDropAttackTracking();

	/** Stop tracking fall (call when character lands) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void StopDropAttackTracking();

	/** Get current drop attack damage multiplier based on fall distance */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	float GetDropAttackDamageMultiplier() const;

	/** Check if currently performing drop attack */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool IsDropAttacking() const { return bIsDropAttacking; }

	/** Check if in falling portion of drop attack (holding pose) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool IsDropAttackFalling() const { return bIsDropAttackFalling; }

	/** Called when character lands during drop attack - resumes animation and deals damage */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void OnDropAttackLand();

	/** Get current light combo index */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	int32 GetLightComboIndex() const { return LightComboIndex; }

	/** Get current heavy combo index */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	int32 GetHeavyComboIndex() const { return HeavyComboIndex; }

	// ==================== Parry/Block/Riposte Functions ====================

	/** Attempt to parry (called on guard button tap) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void AttemptParry();

	/** Perform riposte on parried target */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool PerformRiposte();

	/** Called when we successfully parry an incoming attack */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void OnParrySuccessful(AActor* AttackingActor);

	/** Check if we can parry with current equipment */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool CanParry() const;

	/** Check if currently in parry window */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool IsInParryWindow() const { return bInParryWindow; }

	/** Check if riposte is available */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool CanRiposte() const { return bCanRiposte && ParriedTarget != nullptr; }

	/** Get current combat state */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	ECombatState GetCombatState() const { return CurrentCombatState; }

	/** Modify incoming damage based on combat state (blocking/parrying) */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	FDamageModifierResult ModifyIncomingDamage(float IncomingDamage, AActor* DamageCauser);

	// ==================== Input Buffer Functions ====================

	/** Buffer an input for later execution */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	void BufferInput(EBufferedInputType InputType);

	/** Check if we can accept buffered input right now */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool CanAcceptBufferedInput() const;

	/** Check if we can dodge-cancel the current attack */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Combat")
	bool CanDodgeCancel() const;

	// ==================== Item Data Access ====================

	/** Get item data from data table (public for UI access) */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool GetItemData(FName ItemID, FItemData& OutItemData) const;

protected:
	/** Equipment slots - maps slot type to equipped item ID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TMap<EEquipmentSlot, FName> EquippedItems;

	/** Hotbar slots */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hotbar")
	TMap<EHotbarSlot, FHotbarSlotData> HotbarSlots;

	/** Spawned weapon mesh components (skeletal meshes) */
	UPROPERTY()
	TMap<EEquipmentSlot, USkeletalMeshComponent*> WeaponMeshComponents;

	/** Spawned weapon static mesh components (fallback for items without skeletal mesh) */
	UPROPERTY()
	TMap<EEquipmentSlot, UStaticMeshComponent*> WeaponStaticMeshComponents;

	/** Spawned armor skeletal mesh components (use Leader Pose) */
	UPROPERTY()
	TMap<EEquipmentSlot, USkeletalMeshComponent*> ArmorMeshComponents;

	/** Spawned toggle item actors (e.g., lamps) - maps ItemID to spawned actor */
	UPROPERTY()
	TMap<FName, AActor*> SpawnedToggleActors;

	/** Recalculate and broadcast stat changes */
	void UpdateStats();

	/** Recalculate weight and check encumbrance */
	void UpdateWeight();

	/** Equip weapon from hotbar */
	void EquipFromHotbar(EHotbarSlot HotbarSlot);

	/** Spawn and attach weapon mesh */
	void AttachWeaponMesh(EEquipmentSlot Slot, const FItemData& ItemData);

	/** Remove weapon mesh */
	void RemoveWeaponMesh(EEquipmentSlot Slot);

	/** Spawn and attach armor mesh with leader pose */
	void AttachArmorMesh(EEquipmentSlot Slot, const FItemData& ItemData);

	/** Remove armor mesh */
	void RemoveArmorMesh(EEquipmentSlot Slot);

	/** Update weapon type tracking */
	void UpdateWeaponTypes();

	/** Set weapon type and broadcast change */
	void SetWeaponType(EWeaponType NewType);

	/** Get the character's skeletal mesh (for leader pose) */
	USkeletalMeshComponent* GetOwnerMesh() const;

	/** Timer handle for attack recovery */
	FTimerHandle AttackRecoveryTimerHandle;

	/** Timer handle for combo window expiration */
	FTimerHandle ComboWindowTimerHandle;

	/** Timer handle for parry window */
	FTimerHandle ParryWindowTimerHandle;

	/** Timer handle for riposte window expiration */
	FTimerHandle RiposteWindowTimerHandle;

	/** Timer handle for attack progress tracking */
	FTimerHandle AttackProgressTimerHandle;

	/** Time when guard button was pressed (for tap vs hold detection) */
	float GuardPressTime = 0.0f;

	/** Time when current attack started */
	float AttackStartTime = 0.0f;

	/** Duration of current attack animation */
	float CurrentAttackDuration = 0.0f;

	/** Close combo window and reset combo */
	void CloseComboWindow();

	/** Re-enable movement after attack */
	void RestoreMovement();

	/** Rotate to face lock-on target (if locked on) */
	void FaceLockedTarget();

	// ==================== Internal Combat Functions ====================

	/** Set combat state and broadcast event */
	void SetCombatState(ECombatState NewState);

	/** Open the parry window */
	void OpenParryWindow();

	/** Close the parry window */
	void CloseParryWindow();

	/** Play blocking montage from equipped off-hand or primary */
	void PlayBlockMontage();

	/** Stop blocking montage when guard released */
	void StopBlockMontage();

	/** End the riposte opportunity window */
	void EndRiposteWindow();

	/** Process any buffered input */
	void ProcessBufferedInput();

	/** Update attack progress (called on tick during attack) */
	void UpdateAttackProgress();

	/** Execute a buffered input */
	void ExecuteBufferedInput(EBufferedInputType InputType);
};
