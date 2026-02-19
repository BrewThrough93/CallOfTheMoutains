// CallOfTheMoutains - Bile Projectile for Half Man ranged attack
// A vomit/bile projectile that damages and slows the player

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BileProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UAudioComponent;
class USoundBase;

/**
 * Bile Projectile - Ranged attack for the Half Man enemy
 *
 * Features:
 * - Arcing projectile trajectory
 * - Direct damage on impact
 * - Slow debuff effect on player
 * - VFX for in-flight and impact
 * - SFX for flight and impact
 */
UCLASS()
class CALLOFTHEMOUTAINS_API ABileProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABileProjectile();

protected:
	virtual void BeginPlay() override;

public:
	// ==================== Components ====================

	/** Collision sphere for hit detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	/** Niagara effect for in-flight visuals */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* BileEffect;

	/** Audio for flight sound */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* FlightSound;

	// ==================== Damage Settings ====================

	/** Direct damage on impact */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Damage")
	float DirectDamage = 15.0f;

	/** Damage type class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Damage")
	TSubclassOf<UDamageType> DamageTypeClass;

	// ==================== Slow Debuff Settings ====================

	/** Speed reduction percentage (0.5 = 50% reduction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Debuff", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowPercent = 0.5f;

	/** Duration of slow effect in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Debuff", meta = (ClampMin = "0.1"))
	float SlowDuration = 3.0f;

	// ==================== Movement Settings ====================

	/** Initial projectile speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Movement")
	float ProjectileSpeed = 1200.0f;

	/** Gravity scale for arcing trajectory (0 = straight, 1 = normal gravity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Movement", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float GravityScale = 0.3f;

	/** Projectile lifetime before auto-destroy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Movement")
	float Lifetime = 5.0f;

	// ==================== VFX Settings ====================

	/** Niagara system for impact effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|VFX")
	UNiagaraSystem* ImpactEffect;

	/** Scale for impact effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|VFX")
	FVector ImpactEffectScale = FVector(1.0f);

	// ==================== Audio Settings ====================

	/** Sound for impact */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Audio")
	USoundBase* ImpactSound;

	/** Volume for impact sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bile|Audio")
	float ImpactSoundVolume = 1.0f;

	// ==================== Functions ====================

	/** Initialize the projectile with direction and owner */
	UFUNCTION(BlueprintCallable, Category = "Bile")
	void InitializeProjectile(AActor* InOwner, FVector Direction);

protected:
	/** Called when projectile hits something */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Apply slow debuff to target */
	void ApplySlowDebuff(AActor* Target);

	/** Spawn impact effects */
	void SpawnImpactEffects(FVector Location, FVector Normal);

	/** Apply damage to target */
	void ApplyDamage(AActor* Target, const FHitResult& Hit);

	/** Destroy the projectile */
	void DestroyProjectile();

private:
	/** The actor that fired this projectile */
	UPROPERTY()
	AActor* OwnerActor;

	/** Has this projectile already hit something? */
	bool bHasHit = false;
};
