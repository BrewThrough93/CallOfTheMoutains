// CallOfTheMoutains - Melee Trace Component
// Flexible melee hit detection supporting weapon sockets or character mesh sockets

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeTraceComponent.generated.h"

class USkeletalMeshComponent;
class UEquipmentComponent;
class UHealthComponent;

/**
 * Trace mode for melee detection
 */
UENUM(BlueprintType)
enum class EMeleeTraceMode : uint8
{
	/** Line trace between two sockets (e.g., weapon Base to Tip) */
	Linear		UMETA(DisplayName = "Linear (Socket to Socket)"),

	/** Sphere trace from single socket with radius (e.g., fist/hand) */
	Spherical	UMETA(DisplayName = "Spherical (Single Socket + Radius)")
};

/**
 * Source mesh for socket locations
 */
UENUM(BlueprintType)
enum class EMeleeTraceMeshSource : uint8
{
	/** Use the equipped weapon's skeletal mesh sockets */
	WeaponMesh		UMETA(DisplayName = "Weapon Mesh"),

	/** Use the character's skeletal mesh sockets */
	CharacterMesh	UMETA(DisplayName = "Character Mesh")
};

/**
 * Result of a melee trace hit
 */
USTRUCT(BlueprintType)
struct FMeleeHitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	bool bHit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	AActor* HitActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	UPrimitiveComponent* HitComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	FVector HitNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	FName BoneName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Melee")
	float AppliedDamage = 0.0f;
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeHit, const FMeleeHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMeleeTraceStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMeleeTraceEnded);

/**
 * Melee Trace Component
 * Performs socket-based hit detection for melee combat.
 * Supports both weapon mesh sockets (Base/Tip) and character mesh sockets (hand_r).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CALLOFTHEMOUTAINS_API UMeleeTraceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMeleeTraceComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== Configuration ====================

	/** Trace mode: Linear (two sockets) or Spherical (one socket + radius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Config")
	EMeleeTraceMode TraceMode = EMeleeTraceMode::Linear;

	/** Source mesh for socket locations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Config")
	EMeleeTraceMeshSource MeshSource = EMeleeTraceMeshSource::WeaponMesh;

	// ==================== Socket Configuration ====================

	/** Start socket name for Linear mode, or center socket for Spherical mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets")
	FName StartSocket = TEXT("Base");

	/** End socket name for Linear mode (ignored in Spherical mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets")
	FName EndSocket = TEXT("Tip");

	/** Radius for Spherical mode, or trace thickness for Linear mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Sockets", meta = (ClampMin = "1.0"))
	float TraceRadius = 15.0f;

	// ==================== Damage Configuration ====================

	/** Base damage to apply on hit (multiplied by DamageMultiplier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Damage")
	float BaseDamage = 20.0f;

	/** Damage multiplier (can be overridden by AnimNotify) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Damage")
	float DamageMultiplier = 1.0f;

	/** Use weapon's PhysicalDamage stat instead of BaseDamage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Damage")
	bool bUseWeaponDamage = true;

	/** Damage type class to apply */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Damage")
	TSubclassOf<UDamageType> DamageTypeClass;

	// ==================== Trace Configuration ====================

	/** Object types to trace against */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Trace")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	/** Actors to ignore (owner is always ignored) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Trace")
	TArray<AActor*> ActorsToIgnore;

	/** Number of interpolation steps between frames (higher = more accurate but slower) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Trace", meta = (ClampMin = "1", ClampMax = "10"))
	int32 InterpolationSteps = 3;

	/** Can hit the same actor multiple times per trace activation? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Trace")
	bool bAllowMultipleHitsPerActor = false;

	// ==================== Debug ====================

	/** Draw debug traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Debug")
	bool bDrawDebug = false;

	/** Debug draw duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Debug", meta = (EditCondition = "bDrawDebug", ClampMin = "0.0"))
	float DebugDrawDuration = 0.5f;

	// ==================== Events ====================

	/** Called when a hit is registered */
	UPROPERTY(BlueprintAssignable, Category = "Melee Trace|Events")
	FOnMeleeHit OnMeleeHit;

	/** Called when trace starts */
	UPROPERTY(BlueprintAssignable, Category = "Melee Trace|Events")
	FOnMeleeTraceStarted OnMeleeTraceStarted;

	/** Called when trace ends */
	UPROPERTY(BlueprintAssignable, Category = "Melee Trace|Events")
	FOnMeleeTraceEnded OnMeleeTraceEnded;

	// ==================== Functions ====================

	/** Start tracing for hits */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void StartTrace();

	/** Stop tracing for hits */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void StopTrace();

	/** Is currently tracing? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Melee Trace")
	bool IsTracing() const { return bIsTracing; }

	/** Set trace mode at runtime */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void SetTraceMode(EMeleeTraceMode NewMode) { TraceMode = NewMode; }

	/** Set mesh source at runtime */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void SetMeshSource(EMeleeTraceMeshSource NewSource);

	/** Set socket names at runtime */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void SetSockets(FName NewStartSocket, FName NewEndSocket = NAME_None);

	/** Set damage multiplier (used by AnimNotify for heavy attacks, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void SetDamageMultiplier(float NewMultiplier) { DamageMultiplier = NewMultiplier; }

	/** Clear the hit actors list (allows re-hitting) */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void ClearHitActors();

	/** Get the target mesh component based on current MeshSource */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	USkeletalMeshComponent* GetTargetMesh() const;

	/** Manually set the weapon mesh to trace against (overrides auto-detection) */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void SetWeaponMesh(USkeletalMeshComponent* NewWeaponMesh);

protected:
	/** Is currently tracing? */
	bool bIsTracing = false;

	/** Previous frame socket locations for interpolation */
	FVector PrevStartLocation = FVector::ZeroVector;
	FVector PrevEndLocation = FVector::ZeroVector;
	bool bHasPreviousLocations = false;

	/** Actors already hit this trace (to prevent duplicate hits) */
	UPROPERTY()
	TSet<AActor*> HitActorsThisTrace;

	/** Cached equipment component reference */
	UPROPERTY()
	UEquipmentComponent* CachedEquipmentComponent;

	/** Manually set weapon mesh (if any) */
	UPROPERTY()
	USkeletalMeshComponent* ManualWeaponMesh;

	/** Get socket world location from target mesh */
	bool GetSocketLocation(FName SocketName, FVector& OutLocation) const;

	/** Perform the trace and return hits */
	void PerformTrace();

	/** Perform linear trace between two points */
	void PerformLinearTrace(const FVector& Start, const FVector& End);

	/** Perform spherical trace at a point */
	void PerformSphericalTrace(const FVector& Center);

	/** Process a hit result and apply damage */
	void ProcessHit(const FHitResult& Hit);

	/** Process a hit actor directly (for overlap-based detection) */
	void ProcessHitActor(AActor* HitActor, const FVector& HitLocation);

	/** Calculate final damage to apply */
	float CalculateDamage() const;

	/** Get the weapon mesh from equipment component */
	USkeletalMeshComponent* GetWeaponMeshFromEquipment() const;

	/** Get the character mesh */
	USkeletalMeshComponent* GetCharacterMesh() const;

	/** Draw debug visualization */
	void DrawDebugTrace(const FVector& Start, const FVector& End, bool bHit) const;
};
