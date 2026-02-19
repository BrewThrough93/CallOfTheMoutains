// CallOfTheMoutains - Targetable Component for Lock-On System

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetableComponent.generated.h"

class UBillboardComponent;
class UPointLightComponent;
class UTexture2D;

/**
 * Add this component to any actor that can be locked onto
 * Works with LockOnComponent to enable Souls-like targeting
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UTargetableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetableComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** Whether this target can currently be locked onto */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
	bool bCanBeTargeted = true;

	/** Priority for target selection (higher = more likely to be selected) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On", meta = (ClampMin = "0", ClampMax = "100"))
	int32 TargetPriority = 50;

	/** Offset from actor origin for the lock-on point (where camera looks) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
	FVector TargetOffset = FVector(0.0f, 0.0f, 40.0f); // Waist/lower chest height

	/** Max distance at which this target can be acquired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On", meta = (ClampMin = "100.0"))
	float MaxLockOnDistance = 2000.0f;

	// ==================== Visual Indicator Settings ====================

	/** Show a visual indicator when locked on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator")
	bool bShowLockOnIndicator = true;

	/** Sprite texture for lock-on indicator */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator")
	TSoftObjectPtr<UTexture2D> LockOnSprite;

	/** Sprite scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator", meta = (ClampMin = "0.1"))
	float SpriteScale = 0.5f;

	/** Show a point light when locked on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator")
	bool bShowLockOnLight = true;

	/** Point light color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator")
	FLinearColor LockOnLightColor = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f); // Orange

	/** Point light intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator", meta = (ClampMin = "0.0"))
	float LockOnLightIntensity = 5000.0f;

	/** Point light attenuation radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator", meta = (ClampMin = "10.0"))
	float LockOnLightRadius = 100.0f;

	/** Attach indicator to a bone instead of actor origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator")
	bool bAttachToBone = true;

	/** Bone name to attach the indicator to (e.g., spine_01) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator", meta = (EditCondition = "bAttachToBone"))
	FName AttachBoneName = TEXT("spine_01");

	/** Offset from attachment point/bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Indicator")
	FVector IndicatorOffset = FVector(0.0f, 0.0f, 0.0f);

	// ==================== Functions ====================

	/** Get the world location of the lock-on point */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	FVector GetTargetLocation() const;

	/** Check if this target can currently be locked onto */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	bool IsTargetable() const { return bCanBeTargeted; }

	/** Set whether this target can be locked onto (notifies lock-on system if becoming non-targetable) */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	void SetTargetable(bool bNewTargetable);

	/** Called when this target is locked onto */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lock On")
	void OnTargeted();

	/** Called when lock-on is released from this target */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lock On")
	void OnTargetLost();

	/** Native versions for C++ */
	void NotifyTargeted();
	void NotifyTargetLost();

	/** Delegate broadcast when targeted/untargeted */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetStateChanged, bool, bIsTargeted);

	UPROPERTY(BlueprintAssignable, Category = "Lock On")
	FOnTargetStateChanged OnTargetStateChanged;

protected:
	bool bIsCurrentlyTargeted = false;

	/** Billboard sprite component for lock-on indicator */
	UPROPERTY()
	UBillboardComponent* LockOnSpriteComponent = nullptr;

	/** Point light component for lock-on indicator */
	UPROPERTY()
	UPointLightComponent* LockOnLightComponent = nullptr;

	/** Create the visual indicator components */
	void CreateIndicatorComponents();

	/** Show the lock-on indicator */
	void ShowIndicator();

	/** Hide the lock-on indicator */
	void HideIndicator();

	/** Get the skeletal mesh component from owner (for bone attachment) */
	class USkeletalMeshComponent* GetOwnerSkeletalMesh() const;
};
