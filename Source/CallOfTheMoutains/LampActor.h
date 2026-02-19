// CallOfTheMoutains - Lamp Actor for player handheld light source

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LampActor.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class UAudioComponent;
class USoundBase;

/**
 * Lamp Actor - A toggleable light source that can be equipped by the player
 *
 * Features:
 * - Point light component for illumination
 * - Toggle on/off functionality
 * - Attaches to player's hand socket when equipped
 * - Configurable light color, intensity, and radius
 */
UCLASS(Blueprintable)
class CALLOFTHEMOUTAINS_API ALampActor : public AActor
{
	GENERATED_BODY()

public:
	ALampActor();

protected:
	virtual void BeginPlay() override;

public:
	// ==================== Components ====================

	/** Root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lamp")
	USceneComponent* RootScene;

	/** Static mesh for the lamp model */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lamp")
	UStaticMeshComponent* LampMesh;

	/** Point light for illumination */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lamp")
	UPointLightComponent* LampLight;

	// ==================== Configuration ====================

	/** Light color when lamp is on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Light")
	FLinearColor LightColor = FLinearColor(1.0f, 0.85f, 0.6f, 1.0f);

	/** Light intensity when lamp is on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Light", meta = (ClampMin = "0.0"))
	float LightIntensity = 5000.0f;

	/** Light attenuation radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Light", meta = (ClampMin = "0.0"))
	float LightRadius = 800.0f;

	/** Is the lamp currently on? */
	UPROPERTY(BlueprintReadOnly, Category = "Lamp")
	bool bIsLampOn = false;

	/** Socket name to attach to on player (if empty, uses default hand socket) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp")
	FName AttachSocketName = TEXT("weapon_l");

	// ==================== Audio ====================

	/** Sound to play when lamp turns on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Audio")
	USoundBase* TurnOnSound;

	/** Sound to play when lamp turns off */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Audio")
	USoundBase* TurnOffSound;

	/** Volume multiplier for lamp sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SoundVolume = 1.0f;

	// ==================== Functions ====================

	/** Toggle the lamp on or off */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void ToggleLamp();

	/** Turn the lamp on */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void TurnOn();

	/** Turn the lamp off */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void TurnOff();

	/** Check if lamp is currently on */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lamp")
	bool IsLampOn() const { return bIsLampOn; }

	/** Attach lamp to a character's skeleton using default socket */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void AttachToCharacter(ACharacter* Character);

	/** Attach lamp to a character's skeleton at specific socket */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void AttachToCharacterAtSocket(ACharacter* Character, FName SocketName);

	/** Detach lamp from character */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void DetachFromCharacter();

	/** Set light properties */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void SetLightProperties(FLinearColor NewColor, float NewIntensity, float NewRadius);

	/** Set the lamp mesh */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void SetLampMesh(UStaticMesh* NewMesh);

protected:
	/** Update light component based on current state */
	void UpdateLightState();
};
