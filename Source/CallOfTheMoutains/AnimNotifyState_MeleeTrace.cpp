// CallOfTheMoutains - Animation Notify State for Melee Tracing Implementation

#include "AnimNotifyState_MeleeTrace.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UAnimNotifyState_MeleeTrace::UAnimNotifyState_MeleeTrace()
{
	// Set default notify color in editor
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 50, 50, 255); // Red-ish for combat
#endif
}

void UAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UMeleeTraceComponent* TraceComp = GetMeleeTraceComponent(MeshComp);
	if (!TraceComp)
	{
		return;
	}

	// Store original values
	OriginalTraceMode = TraceComp->TraceMode;
	OriginalMeshSource = TraceComp->MeshSource;
	OriginalStartSocket = TraceComp->StartSocket;
	OriginalEndSocket = TraceComp->EndSocket;
	OriginalRadius = TraceComp->TraceRadius;
	OriginalDamageMultiplier = TraceComp->DamageMultiplier;
	OriginalBaseDamage = TraceComp->BaseDamage;
	OriginalUseWeaponDamage = TraceComp->bUseWeaponDamage;
	OriginalDrawDebug = TraceComp->bDrawDebug;

	// Apply overrides
	if (bOverrideTraceMode)
	{
		TraceComp->TraceMode = TraceMode;
	}

	if (bOverrideMeshSource)
	{
		TraceComp->SetMeshSource(MeshSource);
	}

	if (bOverrideSockets)
	{
		TraceComp->SetSockets(StartSocket, EndSocket);
	}

	if (bOverrideRadius)
	{
		TraceComp->TraceRadius = TraceRadius;
	}

	if (bOverrideBaseDamage)
	{
		TraceComp->BaseDamage = BaseDamage;
		TraceComp->bUseWeaponDamage = false; // Disable weapon damage when overriding
	}

	// Apply damage multiplier (stacks)
	TraceComp->SetDamageMultiplier(OriginalDamageMultiplier * DamageMultiplier);

	// Apply debug setting
	if (bDrawDebug)
	{
		TraceComp->bDrawDebug = true;
	}

	// Start the trace
	TraceComp->StartTrace();
}

void UAnimNotifyState_MeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UMeleeTraceComponent* TraceComp = GetMeleeTraceComponent(MeshComp);
	if (!TraceComp)
	{
		return;
	}

	// Stop the trace
	TraceComp->StopTrace();

	// Restore original values
	TraceComp->TraceMode = OriginalTraceMode;
	TraceComp->SetMeshSource(OriginalMeshSource);
	TraceComp->SetSockets(OriginalStartSocket, OriginalEndSocket);
	TraceComp->TraceRadius = OriginalRadius;
	TraceComp->SetDamageMultiplier(OriginalDamageMultiplier);
	TraceComp->BaseDamage = OriginalBaseDamage;
	TraceComp->bUseWeaponDamage = OriginalUseWeaponDamage;
	TraceComp->bDrawDebug = OriginalDrawDebug;
}

void UAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	// Trace is handled by the component's tick, but we could add per-frame logic here if needed
}

FString UAnimNotifyState_MeleeTrace::GetNotifyName_Implementation() const
{
	FString ModeName = bOverrideTraceMode ?
		(TraceMode == EMeleeTraceMode::Linear ? TEXT("Linear") : TEXT("Spherical")) :
		TEXT("Default");

	FString SourceName = bOverrideMeshSource ?
		(MeshSource == EMeleeTraceMeshSource::WeaponMesh ? TEXT("Weapon") : TEXT("Char")) :
		TEXT("Default");

	if (bOverrideSockets)
	{
		return FString::Printf(TEXT("Melee Trace [%s→%s] x%.1f"),
			*StartSocket.ToString(),
			*EndSocket.ToString(),
			DamageMultiplier);
	}

	return FString::Printf(TEXT("Melee Trace [%s/%s] x%.1f"), *ModeName, *SourceName, DamageMultiplier);
}

UMeleeTraceComponent* UAnimNotifyState_MeleeTrace::GetMeleeTraceComponent(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return Owner->FindComponentByClass<UMeleeTraceComponent>();
}
