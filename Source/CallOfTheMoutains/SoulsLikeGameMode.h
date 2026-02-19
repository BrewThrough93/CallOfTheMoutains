// CallOfTheMoutains - Souls-like Game Mode
// Sets up the correct character, controller, and game rules

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoulsLikeGameMode.generated.h"

/**
 * Game Mode for souls-like gameplay
 * Sets default pawn to SoulsLikeCharacter
 * Sets default controller to SoulsLikePlayerController
 */
UCLASS()
class CALLOFTHEMOUTAINS_API ASoulsLikeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASoulsLikeGameMode();
};
