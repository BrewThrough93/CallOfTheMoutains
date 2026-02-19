// CallOfTheMoutains - Test Dummy for Lock-On Testing

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestDummyActor.generated.h"

class UTargetableComponent;
class UHealthComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UPointLightComponent;
class UBillboardComponent;
class UCapsuleComponent;

/**
 * Test Dummy Actor - A simple target for testing lock-on
 * Place in level to test targeting system
 */
UCLASS()
class CALLOFTHEMOUTAINS_API ATestDummyActor : public AActor
{
	GENERATED_BODY()

public:
	ATestDummyActor();

protected:
	virtual void BeginPlay() override;

public:
	/** Collision capsule (for lock-on detection) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleCollision;

	/** Visual mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DummyMesh;

	/** Targetable component for lock-on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTargetableComponent* TargetableComponent;

	/** Health component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	/** Lock-on indicator widget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* LockOnIndicator;

	/** Lock-on point light */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* LockOnLight;

	/** Lock-on sprite indicator */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* LockOnSprite;

	/** Light color when targeted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
	FLinearColor TargetedLightColor = FLinearColor::Red;

	/** Light intensity when targeted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
	float TargetedLightIntensity = 5000.0f;

	/** Should the dummy respawn after death? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	bool bRespawns = true;

	/** Time before respawning (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", meta = (EditCondition = "bRespawns", ClampMin = "0.1"))
	float RespawnDelay = 5.0f;

	/** Show/hide the lock-on indicator */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	void SetLockOnIndicatorVisible(bool bVisible);

protected:
	/** Called when this dummy is targeted */
	UFUNCTION()
	void OnTargetStateChanged(bool bIsTargeted);

	/** Called when dummy takes damage */
	UFUNCTION()
	void OnDamageReceived(float Damage, AActor* DamageCauser, AController* InstigatorController);

	/** Called when dummy dies */
	UFUNCTION()
	void OnDeath(AActor* KilledBy, AController* InstigatorController);

	/** Respawn the dummy */
	void Respawn();

private:
	/** Timer handle for respawn */
	FTimerHandle RespawnTimerHandle;
};
