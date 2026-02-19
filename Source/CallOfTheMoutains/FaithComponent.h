// CallOfTheMoutains - Faith Component
// Souls-like currency system - earned from enemies, selling items, consumables

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FaithComponent.generated.h"

// Delegate for faith changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFaithChanged, int32, CurrentFaith, int32, Delta, bool, bWasGained);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFaithLost, int32, AmountLost);

/**
 * Faith Component - Manages the player's faith currency
 * Faith is earned by:
 * - Defeating enemies
 * - Selling items to vendors
 * - Consuming certain items
 *
 * Faith is lost on death (can be recovered from death location)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UFaithComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFaithComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ==================== Events ====================

	/** Called when faith amount changes */
	UPROPERTY(BlueprintAssignable, Category = "Faith|Events")
	FOnFaithChanged OnFaithChanged;

	/** Called when faith is lost (death, spent, etc.) */
	UPROPERTY(BlueprintAssignable, Category = "Faith|Events")
	FOnFaithLost OnFaithLost;

	// ==================== State ====================

	/** Current faith amount */
	UPROPERTY(BlueprintReadOnly, Category = "Faith|State")
	int32 CurrentFaith = 0;

	/** Faith lost on last death (can be recovered) */
	UPROPERTY(BlueprintReadOnly, Category = "Faith|State")
	int32 LostFaith = 0;

	/** Location where faith was lost (for recovery) */
	UPROPERTY(BlueprintReadOnly, Category = "Faith|State")
	FVector LostFaithLocation;

	/** Has unrecovered faith from death? */
	UPROPERTY(BlueprintReadOnly, Category = "Faith|State")
	bool bHasLostFaith = false;

	// ==================== Functions ====================

	/** Add faith (from kills, pickups, etc.). Returns new total. */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	int32 AddFaith(int32 Amount);

	/** Spend faith (purchasing, leveling, etc.). Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	bool SpendFaith(int32 Amount);

	/** Check if player has enough faith */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Faith")
	bool HasFaith(int32 Amount) const { return CurrentFaith >= Amount; }

	/** Get current faith amount */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Faith")
	int32 GetFaith() const { return CurrentFaith; }

	/** Lose all faith (called on death) - stores for recovery */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	void LoseAllFaith();

	/** Recover lost faith (called when player reaches death location) */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	int32 RecoverLostFaith();

	/** Clear lost faith without recovering (second death before recovery) */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	void ClearLostFaith();

	/** Set faith directly (for loading saves, cheats, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Faith")
	void SetFaith(int32 Amount);

	/** Get the amount of lost faith available for recovery */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Faith")
	int32 GetLostFaith() const { return LostFaith; }

	/** Get location where faith was lost */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Faith")
	FVector GetLostFaithLocation() const { return LostFaithLocation; }

private:
	/** Broadcast faith change event */
	void BroadcastFaithChange(int32 Delta, bool bWasGained);
};
