// CallOfTheMoutains - Souls-like Game Mode Implementation

#include "SoulsLikeGameMode.h"
#include "SoulsLikeCharacter.h"
#include "SoulsLikePlayerController.h"
#include "UObject/ConstructorHelpers.h"

ASoulsLikeGameMode::ASoulsLikeGameMode()
{
	// Set default pawn class to our souls-like character
	DefaultPawnClass = ASoulsLikeCharacter::StaticClass();

	// Set default player controller to our souls-like controller
	PlayerControllerClass = ASoulsLikePlayerController::StaticClass();
}
