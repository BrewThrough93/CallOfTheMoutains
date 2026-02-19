// CallOfTheMoutains - Item and Equipment Type Definitions

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimMontage.h"
#include "ItemTypes.generated.h"

class AItemPickup;

/**
 * Equipment slot types for armor, weapons, and accessories
 * Souls-like layout: armor pieces, weapons, rings, trinkets
 */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None			UMETA(DisplayName = "None"),
	// Armor slots
	Helmet			UMETA(DisplayName = "Helmet"),
	Chest			UMETA(DisplayName = "Chest"),
	Gloves			UMETA(DisplayName = "Gloves"),
	Legs			UMETA(DisplayName = "Legs"),
	Boots			UMETA(DisplayName = "Boots"),
	// Weapon slots
	PrimaryWeapon	UMETA(DisplayName = "Primary Weapon"),
	OffHand			UMETA(DisplayName = "Off Hand"),
	// Ring slots (4 total)
	Ring1			UMETA(DisplayName = "Ring 1"),
	Ring2			UMETA(DisplayName = "Ring 2"),
	Ring3			UMETA(DisplayName = "Ring 3"),
	Ring4			UMETA(DisplayName = "Ring 4"),
	// Trinket/Talisman slots (4 total)
	Trinket1		UMETA(DisplayName = "Trinket 1"),
	Trinket2		UMETA(DisplayName = "Trinket 2"),
	Trinket3		UMETA(DisplayName = "Trinket 3"),
	Trinket4		UMETA(DisplayName = "Trinket 4")
};

/**
 * Item category for filtering and sorting
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None			UMETA(DisplayName = "None"),
	Consumable		UMETA(DisplayName = "Consumable"),
	Equipment		UMETA(DisplayName = "Equipment"),
	KeyItem			UMETA(DisplayName = "Key Item"),
	Special			UMETA(DisplayName = "Special"),
	Material		UMETA(DisplayName = "Material")
};

/**
 * Weapon type for animations and combat behavior
 */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None			UMETA(DisplayName = "None"),
	Sword			UMETA(DisplayName = "Sword"),
	Greatsword		UMETA(DisplayName = "Greatsword"),
	Axe				UMETA(DisplayName = "Axe"),
	Spear			UMETA(DisplayName = "Spear"),
	Shield			UMETA(DisplayName = "Shield"),
	Dagger			UMETA(DisplayName = "Dagger"),
	Staff			UMETA(DisplayName = "Staff")
};

/**
 * Rarity tier affects stats and visual indicators
 */
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common			UMETA(DisplayName = "Common"),
	Uncommon		UMETA(DisplayName = "Uncommon"),
	Rare			UMETA(DisplayName = "Rare"),
	Epic			UMETA(DisplayName = "Epic"),
	Legendary		UMETA(DisplayName = "Legendary")
};

/**
 * Base stats that items can modify
 */
USTRUCT(BlueprintType)
struct FItemStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Health = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Stamina = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float PhysicalDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float PhysicalDefense = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Poise = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Weight = 0.0f;

	// Combine stats from another item (for equipment totals)
	FItemStats operator+(const FItemStats& Other) const
	{
		FItemStats Result;
		Result.Health = Health + Other.Health;
		Result.Stamina = Stamina + Other.Stamina;
		Result.PhysicalDamage = PhysicalDamage + Other.PhysicalDamage;
		Result.PhysicalDefense = PhysicalDefense + Other.PhysicalDefense;
		Result.Poise = Poise + Other.Poise;
		Result.Weight = Weight + Other.Weight;
		return Result;
	}
};

/**
 * Consumable effect data
 */
USTRUCT(BlueprintType)
struct FConsumableEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float HealthRestore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float StaminaRestore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	bool bIsInstant = true;
};

/**
 * Main item data structure - used in DataTable
 */
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	// Unique identifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID;

	// Display name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	// Item description for UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
	FText Description;

	// Category for filtering
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemCategory Category = EItemCategory::None;

	// Equipment slot (if equippable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

	// Weapon type (if weapon)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EWeaponType WeaponType = EWeaponType::None;

	// Rarity tier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity = EItemRarity::Common;

	// UI icon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	// World mesh for pickups and equipped visuals
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	// Skeletal mesh for equipped weapons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	// Socket name for attachment when equipped (weapons in hand)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName AttachSocket = NAME_None;

	// Socket name for stowed weapon (on back/hip when not in use)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName StowSocket = NAME_None;

	// Scale to apply to weapon/shield mesh when equipped (default 1,1,1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

	// Animation montage to play when equipping this weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSoftObjectPtr<UAnimMontage> EquipMontage;

	// Animation montage to play when unequipping/stowing this weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSoftObjectPtr<UAnimMontage> UnequipMontage;

	// Animation montages for light attack combo chain (left click)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TArray<TSoftObjectPtr<UAnimMontage>> LightAttackMontages;

	// Animation montages for heavy attack combo chain (right click)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TArray<TSoftObjectPtr<UAnimMontage>> HeavyAttackMontages;

	// Stats provided by this item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FItemStats Stats;

	// Consumable effects (if consumable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FConsumableEffect ConsumableEffect;

	// Max stack size (1 = not stackable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	// Can this item be dropped?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bCanDrop = true;

	// Is this a key/quest item?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bIsKeyItem = false;

	// Is this a toggle item? (infinite use, does not consume on use - e.g., lamp, lantern)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bIsToggleItem = false;

	// Actor class to spawn when toggle item is used (e.g., LampActor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "bIsToggleItem"))
	TSubclassOf<AActor> ToggleActorClass;

	// Value for selling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Value = 0;

	// Custom pickup class to spawn when dropping (leave empty for default AItemPickup)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<AItemPickup> PickupClass;

	// === Combat Properties ===

	/** Poise damage dealt by this weapon (higher = more stagger) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat", meta = (ClampMin = "0.0"))
	float PoiseDamage = 20.0f;

	/** Block stability - resistance to guard break (higher = more stable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float BlockStability = 50.0f;

	/** Can this weapon/shield parry attacks? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat")
	bool bCanParry = false;

	/** Can this weapon/shield block attacks (hold guard)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Combat")
	bool bCanBlock = false;

	/** Animation montage for parry attempt (quick Q tap) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSoftObjectPtr<UAnimMontage> ParryMontage;

	/** Animation montage for successful parry deflection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSoftObjectPtr<UAnimMontage> ParrySuccessMontage;

	/** Animation montage for blocking stance (hold Q) - should be looping or use a blend space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSoftObjectPtr<UAnimMontage> BlockMontage;

	/** Animation montage for riposte attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSoftObjectPtr<UAnimMontage> RiposteMontage;

	/** Animation montage for drop/plunge attack (attack while airborne) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Animation")
	TSoftObjectPtr<UAnimMontage> DropAttackMontage;

	// === Combat Sound Effects ===

	/** Sound effect when blocking an attack with this item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Audio")
	TSoftObjectPtr<USoundBase> BlockSound;

	/** Sound effect when parry is successful with this item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Audio")
	TSoftObjectPtr<USoundBase> ParrySound;

	/** Sound effect when guard breaks while using this item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Audio")
	TSoftObjectPtr<USoundBase> GuardBreakSound;

	bool IsValid() const { return !ItemID.IsNone(); }
	bool IsEquipment() const { return Category == EItemCategory::Equipment && EquipmentSlot != EEquipmentSlot::None; }
	bool IsConsumable() const { return Category == EItemCategory::Consumable; }
	bool IsWeapon() const { return EquipmentSlot == EEquipmentSlot::PrimaryWeapon || EquipmentSlot == EEquipmentSlot::OffHand; }
	bool IsStackable() const { return MaxStackSize > 1; }
	bool IsToggleItem() const { return bIsToggleItem; }
};

/**
 * Inventory slot - holds item reference and quantity
 */
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Inventory")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Inventory")
	int32 Quantity = 0;

	bool IsEmpty() const { return ItemID.IsNone() || Quantity <= 0; }
	void Clear() { ItemID = NAME_None; Quantity = 0; }
};

/**
 * Hotbar slot types - D-pad style layout
 * Up = Special/Spells, Down = Consumables, Left = Off-hand, Right = Primary
 */
UENUM(BlueprintType)
enum class EHotbarSlot : uint8
{
	Special			UMETA(DisplayName = "Special/Spell (Up)"),
	PrimaryWeapon	UMETA(DisplayName = "Primary Weapon (Right)"),
	OffHand			UMETA(DisplayName = "Off Hand (Left)"),
	Consumable		UMETA(DisplayName = "Consumable (Down)")
};

// ==================== Combat System Enums ====================

/**
 * Combat state for tracking player/enemy combat actions
 */
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle			UMETA(DisplayName = "Idle"),
	Attacking		UMETA(DisplayName = "Attacking"),
	Blocking		UMETA(DisplayName = "Blocking"),
	Parrying		UMETA(DisplayName = "Parrying"),			// Active parry window
	ParrySuccess	UMETA(DisplayName = "Parry Success"),		// Successful parry, can riposte
	Riposting		UMETA(DisplayName = "Riposting"),			// Executing a riposte attack
	DropAttacking	UMETA(DisplayName = "Drop Attacking"),		// Plunge attack while airborne
	Staggered		UMETA(DisplayName = "Staggered"),
	GuardBroken		UMETA(DisplayName = "Guard Broken"),
	Dodging			UMETA(DisplayName = "Dodging"),
	Recovering		UMETA(DisplayName = "Recovering")			// Post-attack recovery
};

/**
 * Types of stagger that can be applied
 */
UENUM(BlueprintType)
enum class EStaggerType : uint8
{
	None			UMETA(DisplayName = "None"),
	Light			UMETA(DisplayName = "Light"),				// Brief flinch, quick recovery
	Heavy			UMETA(DisplayName = "Heavy"),				// Longer stagger, vulnerable
	GuardBreak		UMETA(DisplayName = "Guard Break"),			// Stamina depleted while blocking
	Parried			UMETA(DisplayName = "Parried")				// Hit during opponent's parry window
};

/**
 * Input types that can be buffered during combat
 */
UENUM(BlueprintType)
enum class EBufferedInputType : uint8
{
	None			UMETA(DisplayName = "None"),
	LightAttack		UMETA(DisplayName = "Light Attack"),
	HeavyAttack		UMETA(DisplayName = "Heavy Attack"),
	Dodge			UMETA(DisplayName = "Dodge"),
	Parry			UMETA(DisplayName = "Parry")
};

// ==================== Combat System Structs ====================

/**
 * Combat configuration - all tunable combat parameters
 */
USTRUCT(BlueprintType)
struct FCombatConfig
{
	GENERATED_BODY()

	// === Parry Settings ===

	/** Duration of the parry window (souls standard ~200ms) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parry", meta = (ClampMin = "0.1", ClampMax = "0.5"))
	float ParryWindowDuration = 0.2f;

	/** Stamina cost to attempt a parry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parry", meta = (ClampMin = "0.0"))
	float ParryStaminaCost = 10.0f;

	/** Time window to perform riposte after successful parry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parry", meta = (ClampMin = "0.5"))
	float RiposteWindowDuration = 1.5f;

	/** Damage multiplier for riposte attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parry", meta = (ClampMin = "1.0"))
	float RiposteDamageMultiplier = 2.5f;

	// === Block Settings ===

	/** Percentage of damage blocked (0.8 = 80% blocked) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockDamageReduction = 0.8f;

	/** Stamina drain multiplier when blocking (damage * this = stamina cost) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "0.5"))
	float BlockStaminaDrainMultiplier = 1.5f;

	/** Time stunned when guard is broken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "0.5"))
	float GuardBreakRecoveryTime = 1.2f;

	// === Attack Stamina Costs ===

	/** Stamina cost for light attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float LightAttackStaminaCost = 15.0f;

	/** Stamina cost for heavy attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float HeavyAttackStaminaCost = 30.0f;

	// === Responsiveness Settings ===

	/** Percentage of attack animation before player can chain next attack (lower = faster chains) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Responsiveness", meta = (ClampMin = "0.1", ClampMax = "0.6"))
	float AttackRecoveryPercent = 0.25f;

	/** Time window for buffered inputs to be stored and executed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Responsiveness", meta = (ClampMin = "0.1", ClampMax = "0.5"))
	float InputBufferWindow = 0.35f;

	/** Percentage of attack animation after which dodge cancel is allowed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Responsiveness", meta = (ClampMin = "0.3", ClampMax = "0.9"))
	float DodgeCancelWindow = 0.5f;

	/** Time window to chain combo attacks (more forgiving) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Responsiveness", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float ComboWindowTime = 1.0f;

	// === Drop Attack Settings ===

	/** Stamina cost for drop/plunge attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropAttack", meta = (ClampMin = "0.0"))
	float DropAttackStaminaCost = 20.0f;

	/** Damage multiplier for drop attacks (based on fall distance) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropAttack", meta = (ClampMin = "1.0"))
	float DropAttackDamageMultiplier = 2.0f;

	/** Minimum fall distance (units) required to perform drop attack (0 = any airborne state) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropAttack", meta = (ClampMin = "0.0"))
	float MinDropAttackHeight = 0.0f;

	/** Fall distance at which drop attack reaches max damage multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropAttack", meta = (ClampMin = "100.0"))
	float MaxDropAttackHeight = 800.0f;

	/** Maximum damage multiplier at max fall height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropAttack", meta = (ClampMin = "1.0"))
	float MaxDropAttackDamageMultiplier = 4.0f;
};

/**
 * Result of damage modification (blocking/parrying)
 */
USTRUCT(BlueprintType)
struct FDamageModifierResult
{
	GENERATED_BODY()

	/** Final damage after modification */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float ModifiedDamage = 0.0f;

	/** Was the attack blocked? */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bWasBlocked = false;

	/** Was the attack parried? */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bWasParried = false;

	/** Stamina drain from blocking */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float StaminaDrain = 0.0f;

	/** Did blocking this attack cause guard break? */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bCausedGuardBreak = false;
};

/**
 * Buffered input for combat responsiveness
 */
USTRUCT(BlueprintType)
struct FBufferedInput
{
	GENERATED_BODY()

	/** Type of input that was buffered */
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	EBufferedInputType InputType = EBufferedInputType::None;

	/** Time when the input was buffered */
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	float TimeBuffered = 0.0f;

	/** Is there a valid buffered input? */
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	bool bIsValid = false;

	void Clear()
	{
		InputType = EBufferedInputType::None;
		TimeBuffered = 0.0f;
		bIsValid = false;
	}

	void Set(EBufferedInputType InType, float InTime)
	{
		InputType = InType;
		TimeBuffered = InTime;
		bIsValid = true;
	}
};
