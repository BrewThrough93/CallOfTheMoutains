// FireActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireActor.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UAudioComponent;

UCLASS()
class CALLOFTHEMOUTAINS_API AFireActor : public AActor
{
	GENERATED_BODY()

public:
	AFireActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Damage Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire|Damage")
	float DamagePerTick = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire|Damage")
	float DamageInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire|Damage")
	bool bIsActive = true;

	// Functions
	UFUNCTION(BlueprintCallable, Category = "Fire")
	void SetFireActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Fire")
	bool IsFireActive() const { return bIsActive; }

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* DamageVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* FireEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* FireSound;

	// Overlap events
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Apply damage to overlapping actors
	void ApplyDamageToOverlappingActors();

private:
	// Track actors currently in the fire
	UPROPERTY()
	TArray<AActor*> ActorsInFire;

	// Damage timer
	float DamageTimer = 0.0f;
};
