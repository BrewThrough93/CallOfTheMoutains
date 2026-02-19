// CallOfTheMoutains - Melee Trace Component Implementation

#include "MeleeTraceComponent.h"
#include "EquipmentComponent.h"
#include "HealthComponent.h"
#include "ItemTypes.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UMeleeTraceComponent::UMeleeTraceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Default object types to trace against
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));           // Characters/AI
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_PhysicsBody));    // Physics objects
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));   // Dynamic actors (TestDummy, destructibles)
}

void UMeleeTraceComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache equipment component from controller
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (AController* Controller = OwnerPawn->GetController())
		{
			CachedEquipmentComponent = Controller->FindComponentByClass<UEquipmentComponent>();
		}
	}

	// Always ignore owner
	ActorsToIgnore.AddUnique(GetOwner());
}

void UMeleeTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsTracing)
	{
		PerformTrace();
	}
}

void UMeleeTraceComponent::StartTrace()
{
	if (bIsTracing)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("MeleeTrace: StartTrace called - MeshSource: %s, StartSocket: %s, TraceMode: %s"),
		MeshSource == EMeleeTraceMeshSource::WeaponMesh ? TEXT("WeaponMesh") : TEXT("CharacterMesh"),
		*StartSocket.ToString(),
		TraceMode == EMeleeTraceMode::Linear ? TEXT("Linear") : TEXT("Spherical"));

	// Re-cache equipment component if needed (controller might not have been ready at BeginPlay)
	if (!CachedEquipmentComponent)
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			if (AController* Controller = OwnerPawn->GetController())
			{
				CachedEquipmentComponent = Controller->FindComponentByClass<UEquipmentComponent>();
			}
		}
	}

	bIsTracing = true;
	bHasPreviousLocations = false;
	HitActorsThisTrace.Empty();

	// Enable tick
	SetComponentTickEnabled(true);

	// Broadcast event
	OnMeleeTraceStarted.Broadcast();

	// Get initial positions
	FVector StartLoc, EndLoc;
	if (GetSocketLocation(StartSocket, StartLoc))
	{
		UE_LOG(LogTemp, Warning, TEXT("MeleeTrace: Found start socket at %s"), *StartLoc.ToString());
		PrevStartLocation = StartLoc;

		if (TraceMode == EMeleeTraceMode::Linear && GetSocketLocation(EndSocket, EndLoc))
		{
			PrevEndLocation = EndLoc;
		}
		else
		{
			PrevEndLocation = StartLoc;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MeleeTrace: Failed to find start socket '%s'!"), *StartSocket.ToString());
	}
}

void UMeleeTraceComponent::StopTrace()
{
	if (!bIsTracing)
	{
		return;
	}

	bIsTracing = false;
	bHasPreviousLocations = false;

	// Disable tick
	SetComponentTickEnabled(false);

	// Broadcast event
	OnMeleeTraceEnded.Broadcast();
}

void UMeleeTraceComponent::SetMeshSource(EMeleeTraceMeshSource NewSource)
{
	MeshSource = NewSource;
	ManualWeaponMesh = nullptr; // Clear manual override when changing source
}

void UMeleeTraceComponent::SetSockets(FName NewStartSocket, FName NewEndSocket)
{
	StartSocket = NewStartSocket;
	if (!NewEndSocket.IsNone())
	{
		EndSocket = NewEndSocket;
	}
}

void UMeleeTraceComponent::ClearHitActors()
{
	HitActorsThisTrace.Empty();
}

void UMeleeTraceComponent::SetWeaponMesh(USkeletalMeshComponent* NewWeaponMesh)
{
	ManualWeaponMesh = NewWeaponMesh;
}

USkeletalMeshComponent* UMeleeTraceComponent::GetTargetMesh() const
{
	if (MeshSource == EMeleeTraceMeshSource::WeaponMesh)
	{
		// Check manual override first
		if (ManualWeaponMesh)
		{
			return ManualWeaponMesh;
		}
		return GetWeaponMeshFromEquipment();
	}
	else
	{
		return GetCharacterMesh();
	}
}

bool UMeleeTraceComponent::GetSocketLocation(FName SocketName, FVector& OutLocation) const
{
	USkeletalMeshComponent* TargetMesh = GetTargetMesh();
	if (!TargetMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("MeleeTrace: GetSocketLocation - No target mesh found! MeshSource: %d"), (int32)MeshSource);
		return false;
	}

	// Try socket first, then bone name (bones work as sockets in GetSocketLocation)
	if (TargetMesh->DoesSocketExist(SocketName))
	{
		OutLocation = TargetMesh->GetSocketLocation(SocketName);
		return true;
	}

	// Try as bone name directly (GetSocketLocation handles bones too)
	int32 BoneIndex = TargetMesh->GetBoneIndex(SocketName);
	if (BoneIndex != INDEX_NONE)
	{
		OutLocation = TargetMesh->GetBoneLocation(SocketName);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("MeleeTrace: Socket/Bone '%s' not found on mesh '%s'"),
		*SocketName.ToString(),
		TargetMesh->GetSkeletalMeshAsset() ? *TargetMesh->GetSkeletalMeshAsset()->GetName() : TEXT("nullptr"));
	return false;
}

void UMeleeTraceComponent::PerformTrace()
{
	FVector CurrentStartLoc, CurrentEndLoc;

	// Get current socket locations
	if (!GetSocketLocation(StartSocket, CurrentStartLoc))
	{
		return;
	}

	if (TraceMode == EMeleeTraceMode::Linear)
	{
		if (!GetSocketLocation(EndSocket, CurrentEndLoc))
		{
			return;
		}
	}
	else
	{
		CurrentEndLoc = CurrentStartLoc;
	}

	// If we have previous locations, interpolate
	if (bHasPreviousLocations)
	{
		for (int32 i = 0; i <= InterpolationSteps; ++i)
		{
			float Alpha = static_cast<float>(i) / static_cast<float>(InterpolationSteps);

			FVector InterpStart = FMath::Lerp(PrevStartLocation, CurrentStartLoc, Alpha);
			FVector InterpEnd = FMath::Lerp(PrevEndLocation, CurrentEndLoc, Alpha);

			if (TraceMode == EMeleeTraceMode::Linear)
			{
				PerformLinearTrace(InterpStart, InterpEnd);
			}
			else
			{
				PerformSphericalTrace(InterpStart);
			}
		}
	}
	else
	{
		// First frame - just do current position
		if (TraceMode == EMeleeTraceMode::Linear)
		{
			PerformLinearTrace(CurrentStartLoc, CurrentEndLoc);
		}
		else
		{
			PerformSphericalTrace(CurrentStartLoc);
		}
	}

	// Store for next frame
	PrevStartLocation = CurrentStartLoc;
	PrevEndLocation = CurrentEndLoc;
	bHasPreviousLocations = true;
}

void UMeleeTraceComponent::PerformLinearTrace(const FVector& Start, const FVector& End)
{
	// Sample multiple points along the weapon and do sphere overlaps at each
	// This follows the weapon's shape and rotation properly

	TArray<AActor*> IgnoreActors = ActorsToIgnore;
	IgnoreActors.Add(GetOwner());

	// Number of sample points along the weapon (more = more accurate but slower)
	const int32 NumSamples = 4;
	bool bAnyHit = false;

	for (int32 i = 0; i <= NumSamples; ++i)
	{
		float Alpha = static_cast<float>(i) / static_cast<float>(NumSamples);
		FVector SamplePoint = FMath::Lerp(Start, End, Alpha);

		TArray<AActor*> OverlappingActors;
		bool bHit = UKismetSystemLibrary::SphereOverlapActors(
			this,
			SamplePoint,
			TraceRadius,
			ObjectTypes,
			nullptr,
			IgnoreActors,
			OverlappingActors
		);

		if (bHit)
		{
			bAnyHit = true;
			for (AActor* HitActor : OverlappingActors)
			{
				if (HitActor)
				{
					ProcessHitActor(HitActor, SamplePoint);
				}
			}
		}

		// Debug draw sphere at each sample point
		if (bDrawDebug)
		{
			DrawDebugSphere(
				GetWorld(),
				SamplePoint,
				TraceRadius,
				8,
				bHit ? FColor::Red : FColor::Green,
				false,
				0.0f // No duration - redraws each frame to follow sockets
			);
		}
	}

	// Draw line connecting the sockets
	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			Start,
			End,
			bAnyHit ? FColor::Red : FColor::Green,
			false,
			0.0f
		);
	}
}

void UMeleeTraceComponent::PerformSphericalTrace(const FVector& Center)
{
	// Use sphere overlap to detect both blocking AND overlapping actors
	TArray<AActor*> OverlappingActors;
	TArray<AActor*> IgnoreActors = ActorsToIgnore;
	IgnoreActors.Add(GetOwner());

	bool bHit = UKismetSystemLibrary::SphereOverlapActors(
		this,
		Center,
		TraceRadius,
		ObjectTypes,
		nullptr, // ActorClassFilter
		IgnoreActors,
		OverlappingActors
	);

	// Debug draw
	if (bDrawDebug)
	{
		DrawDebugSphere(
			GetWorld(),
			Center,
			TraceRadius,
			12,
			bHit ? FColor::Red : FColor::Blue,
			false,
			DebugDrawDuration
		);
	}

	if (bHit)
	{
		for (AActor* HitActor : OverlappingActors)
		{
			if (HitActor)
			{
				ProcessHitActor(HitActor, Center);
			}
		}
	}
}

void UMeleeTraceComponent::ProcessHitActor(AActor* HitActor, const FVector& HitLocation)
{
	if (!HitActor)
	{
		return;
	}

	// Check if we already hit this actor
	if (!bAllowMultipleHitsPerActor && HitActorsThisTrace.Contains(HitActor))
	{
		return;
	}

	// Add to hit list
	HitActorsThisTrace.Add(HitActor);

	// Calculate base damage
	float FinalDamage = CalculateDamage();
	bool bWasParried = false;
	bool bWasBlocked = false;

	// Check if the target has EquipmentComponent for parry/block
	UEquipmentComponent* TargetEquipment = nullptr;

	// First check if target is a pawn with a controller
	if (APawn* TargetPawn = Cast<APawn>(HitActor))
	{
		if (AController* TargetController = TargetPawn->GetController())
		{
			TargetEquipment = TargetController->FindComponentByClass<UEquipmentComponent>();
		}
	}
	// Also check directly on the actor
	if (!TargetEquipment)
	{
		TargetEquipment = HitActor->FindComponentByClass<UEquipmentComponent>();
	}

	// If target has equipment, check for parry/block
	if (TargetEquipment)
	{
		FDamageModifierResult DamageResult = TargetEquipment->ModifyIncomingDamage(FinalDamage, GetOwner());
		FinalDamage = DamageResult.ModifiedDamage;
		bWasParried = DamageResult.bWasParried;
		bWasBlocked = DamageResult.bWasBlocked;

		// If parried, we don't apply damage (already handled in ModifyIncomingDamage)
		if (bWasParried)
		{
			// Create hit result struct for broadcast (no damage)
			FMeleeHitResult MeleeHit;
			MeleeHit.bHit = true;
			MeleeHit.HitActor = HitActor;
			MeleeHit.HitComponent = nullptr;
			MeleeHit.HitLocation = HitLocation;
			MeleeHit.HitNormal = FVector::ZeroVector;
			MeleeHit.BoneName = NAME_None;
			MeleeHit.AppliedDamage = 0.0f;

			OnMeleeHit.Broadcast(MeleeHit);
			return;
		}
	}

	// Try to apply damage via HealthComponent
	bool bAppliedDamage = false;
	if (UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>())
	{
		AController* InstigatorController = nullptr;
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			InstigatorController = OwnerPawn->GetController();
		}

		HealthComp->TakeDamage(FinalDamage, GetOwner(), InstigatorController);
		bAppliedDamage = true;
	}

	// Also try UE's damage system
	if (!bAppliedDamage && DamageTypeClass)
	{
		UGameplayStatics::ApplyDamage(
			HitActor,
			FinalDamage,
			Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr,
			GetOwner(),
			DamageTypeClass
		);
	}

	// Create hit result struct for broadcast
	FMeleeHitResult MeleeHit;
	MeleeHit.bHit = true;
	MeleeHit.HitActor = HitActor;
	MeleeHit.HitComponent = nullptr;
	MeleeHit.HitLocation = HitLocation;
	MeleeHit.HitNormal = FVector::ZeroVector;
	MeleeHit.BoneName = NAME_None;
	MeleeHit.AppliedDamage = FinalDamage;

	// Broadcast hit event
	OnMeleeHit.Broadcast(MeleeHit);
}

void UMeleeTraceComponent::ProcessHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return;
	}

	// Check if we already hit this actor
	if (!bAllowMultipleHitsPerActor && HitActorsThisTrace.Contains(HitActor))
	{
		return;
	}

	// Add to hit list
	HitActorsThisTrace.Add(HitActor);

	// Calculate and apply damage
	float FinalDamage = CalculateDamage();

	// Try to apply damage via HealthComponent
	bool bAppliedDamage = false;
	if (UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>())
	{
		AController* InstigatorController = nullptr;
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			InstigatorController = OwnerPawn->GetController();
		}

		HealthComp->TakeDamage(FinalDamage, GetOwner(), InstigatorController);
		bAppliedDamage = true;
	}

	// Also try UE's damage system
	if (!bAppliedDamage && DamageTypeClass)
	{
		UGameplayStatics::ApplyDamage(
			HitActor,
			FinalDamage,
			Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr,
			GetOwner(),
			DamageTypeClass
		);
	}

	// Create hit result struct
	FMeleeHitResult MeleeHit;
	MeleeHit.bHit = true;
	MeleeHit.HitActor = HitActor;
	MeleeHit.HitComponent = Hit.GetComponent();
	MeleeHit.HitLocation = Hit.ImpactPoint;
	MeleeHit.HitNormal = Hit.ImpactNormal;
	MeleeHit.BoneName = Hit.BoneName;
	MeleeHit.AppliedDamage = FinalDamage;

	// Broadcast hit event
	OnMeleeHit.Broadcast(MeleeHit);
}

float UMeleeTraceComponent::CalculateDamage() const
{
	float FinalDamage = BaseDamage;

	// Try to get weapon damage from equipment
	if (bUseWeaponDamage && CachedEquipmentComponent)
	{
		FItemData ItemData;
		if (CachedEquipmentComponent->GetEquippedItemData(EEquipmentSlot::PrimaryWeapon, ItemData))
		{
			if (ItemData.Stats.PhysicalDamage > 0.0f)
			{
				FinalDamage = ItemData.Stats.PhysicalDamage;
			}
		}
	}

	return FinalDamage * DamageMultiplier;
}

USkeletalMeshComponent* UMeleeTraceComponent::GetWeaponMeshFromEquipment() const
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return nullptr;
	}

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh)
	{
		return nullptr;
	}

	// Get equipment component to find weapon sockets
	UEquipmentComponent* EquipComp = CachedEquipmentComponent;
	if (!EquipComp)
	{
		// Try to find it from controller
		if (AController* Controller = Character->GetController())
		{
			EquipComp = Controller->FindComponentByClass<UEquipmentComponent>();
		}
	}

	// Get expected weapon socket names
	FName PrimarySocket = EquipComp ? EquipComp->PrimaryWeaponSocket : FName(TEXT("weapon_r"));
	FName OffHandSocketName = EquipComp ? EquipComp->OffHandSocket : FName(TEXT("weapon_l"));

	// Search ALL skeletal mesh components on the character actor
	TArray<USkeletalMeshComponent*> AllSkelMeshes;
	Character->GetComponents<USkeletalMeshComponent>(AllSkelMeshes);

	for (USkeletalMeshComponent* SkelMesh : AllSkelMeshes)
	{
		if (!SkelMesh || SkelMesh == CharMesh)
		{
			continue; // Skip character mesh itself
		}

		// Check if this mesh is attached to a weapon socket
		FName AttachSocket = SkelMesh->GetAttachSocketName();

		if (AttachSocket == PrimarySocket || AttachSocket == OffHandSocketName)
		{
			// Check if this mesh has the sockets we need
			bool bHasStartSocket = SkelMesh->DoesSocketExist(StartSocket);
			bool bHasEndSocket = SkelMesh->DoesSocketExist(EndSocket);

			if (bHasStartSocket && bHasEndSocket)
			{
				return SkelMesh;
			}
		}
	}

	return nullptr;
}

USkeletalMeshComponent* UMeleeTraceComponent::GetCharacterMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Try ACharacter first (most common case)
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		return Character->GetMesh();
	}

	// Fallback for non-Character pawns (like AI enemies that aren't ACharacter)
	// Find the main skeletal mesh component
	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}

void UMeleeTraceComponent::DrawDebugTrace(const FVector& Start, const FVector& End, bool bHit) const
{
	if (!bDrawDebug)
	{
		return;
	}

	FColor Color = bHit ? FColor::Red : FColor::Green;

	if (TraceMode == EMeleeTraceMode::Linear)
	{
		DrawDebugCapsule(
			GetWorld(),
			(Start + End) * 0.5f,
			FVector::Dist(Start, End) * 0.5f,
			TraceRadius,
			FQuat::FindBetweenNormals(FVector::UpVector, (End - Start).GetSafeNormal()),
			Color,
			false,
			DebugDrawDuration
		);
	}
	else
	{
		DrawDebugSphere(
			GetWorld(),
			Start,
			TraceRadius,
			12,
			Color,
			false,
			DebugDrawDuration
		);
	}
}
