// CallOfTheMoutains - Faith Component Implementation

#include "FaithComponent.h"

UFaithComponent::UFaithComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFaithComponent::BeginPlay()
{
	Super::BeginPlay();
}

int32 UFaithComponent::AddFaith(int32 Amount)
{
	if (Amount <= 0)
	{
		return CurrentFaith;
	}

	CurrentFaith += Amount;
	BroadcastFaithChange(Amount, true);

	return CurrentFaith;
}

bool UFaithComponent::SpendFaith(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}

	if (CurrentFaith < Amount)
	{
		return false;
	}

	CurrentFaith -= Amount;
	BroadcastFaithChange(-Amount, false);

	return true;
}

void UFaithComponent::LoseAllFaith()
{
	if (CurrentFaith <= 0)
	{
		return;
	}

	// If there's already lost faith, it's gone forever (died twice)
	if (bHasLostFaith)
	{
		ClearLostFaith();
	}

	// Store faith for potential recovery
	LostFaith = CurrentFaith;
	LostFaithLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	bHasLostFaith = true;

	int32 AmountLost = CurrentFaith;
	CurrentFaith = 0;

	OnFaithLost.Broadcast(AmountLost);
	BroadcastFaithChange(-AmountLost, false);
}

int32 UFaithComponent::RecoverLostFaith()
{
	if (!bHasLostFaith || LostFaith <= 0)
	{
		return 0;
	}

	int32 Recovered = LostFaith;
	CurrentFaith += Recovered;

	// Clear lost faith state
	LostFaith = 0;
	LostFaithLocation = FVector::ZeroVector;
	bHasLostFaith = false;

	BroadcastFaithChange(Recovered, true);

	return Recovered;
}

void UFaithComponent::ClearLostFaith()
{
	LostFaith = 0;
	LostFaithLocation = FVector::ZeroVector;
	bHasLostFaith = false;
}

void UFaithComponent::SetFaith(int32 Amount)
{
	int32 OldFaith = CurrentFaith;
	CurrentFaith = FMath::Max(0, Amount);

	int32 Delta = CurrentFaith - OldFaith;
	if (Delta != 0)
	{
		BroadcastFaithChange(Delta, Delta > 0);
	}
}

void UFaithComponent::BroadcastFaithChange(int32 Delta, bool bWasGained)
{
	OnFaithChanged.Broadcast(CurrentFaith, Delta, bWasGained);
}
