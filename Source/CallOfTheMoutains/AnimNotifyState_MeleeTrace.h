// CallOfTheMoutains - Animation Notify State for Melee Tracing
// Activates melee trace during animation window with per-animation configuration

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "MeleeTraceComponent.h"
#include "AnimNotifyState_MeleeTrace.generated.h"

/**
 * Animation Notify State - Melee Trace
 * Place on attack animations to enable hit detection during the active frames.
 * Automatically starts/stops the MeleeTraceComponent and can override settings per-animation.
 *
 * Usage Examples:
 * 1. Sword Attack: Use WeaponMesh source with Base/Tip sockets
 * 2. Unarmed Punch: Use CharacterMesh source with hand_r socket + Spherical mode
 * 3. Heavy Attack: Set higher DamageMultiplier
 */
UCLASS(meta = (DisplayName = "Melee Trace"))
class CALLOFTHEMOUTAINS_API UAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_MeleeTrace();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	// ==================== Override Settings ====================

	/** Override the trace mode for this animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace")
	bool bOverrideTraceMode = false;

	/** Trace mode to use (if overriding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace", meta = (EditCondition = "bOverrideTraceMode"))
	EMeleeTraceMode TraceMode = EMeleeTraceMode::Linear;

	/** Override the mesh source for this animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace")
	bool bOverrideMeshSource = false;

	/** Mesh source to use (if overriding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace", meta = (EditCondition = "bOverrideMeshSource"))
	EMeleeTraceMeshSource MeshSource = EMeleeTraceMeshSource::WeaponMesh;

	/** Override socket names for this animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets")
	bool bOverrideSockets = false;

	/** Start socket name (if overriding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets", meta = (EditCondition = "bOverrideSockets"))
	FName StartSocket = TEXT("Base");

	/** End socket name for Linear mode (if overriding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets", meta = (EditCondition = "bOverrideSockets"))
	FName EndSocket = TEXT("Tip");

	/** Override trace radius for this animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets")
	bool bOverrideRadius = false;

	/** Trace radius (if overriding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets", meta = (EditCondition = "bOverrideRadius", ClampMin = "1.0"))
	float TraceRadius = 15.0f;

	// ==================== Damage Settings ====================

	/** Damage multiplier for this attack (stacks with component's multiplier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Damage", meta = (ClampMin = "0.1"))
	float DamageMultiplier = 1.0f;

	/** Override base damage for this animation (ignores weapon stats) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Damage")
	bool bOverrideBaseDamage = false;

	/** Base damage to use (if overriding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Damage", meta = (EditCondition = "bOverrideBaseDamage", ClampMin = "0.0"))
	float BaseDamage = 20.0f;

	// ==================== Debug ====================

	/** Show debug traces for this notify */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Debug")
	bool bDrawDebug = false;

protected:
	/** Cached original values to restore after notify ends */
	EMeleeTraceMode OriginalTraceMode;
	EMeleeTraceMeshSource OriginalMeshSource;
	FName OriginalStartSocket;
	FName OriginalEndSocket;
	float OriginalRadius;
	float OriginalDamageMultiplier;
	float OriginalBaseDamage;
	bool OriginalUseWeaponDamage;
	bool OriginalDrawDebug;

	/** Get the MeleeTraceComponent from the owner */
	UMeleeTraceComponent* GetMeleeTraceComponent(USkeletalMeshComponent* MeshComp) const;
};
