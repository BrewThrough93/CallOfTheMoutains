// CallOfTheMoutains - Save Game Implementation

#include "COTMSaveGame.h"

UCOTMSaveGame::UCOTMSaveGame()
{
	SaveSlotName = TEXT("CallOfTheMoutainsSave");
	UserIndex = 0;
	SaveTimestamp = FDateTime::Now();

	PlayerLocation = FVector::ZeroVector;
	PlayerRotation = FRotator::ZeroRotator;

	bWeaponsStowed = false;
	HealthPercent = 1.0f;
	StaminaPercent = 1.0f;
}
