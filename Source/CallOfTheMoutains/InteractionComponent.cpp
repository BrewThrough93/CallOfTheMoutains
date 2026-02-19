// CallOfTheMoutains - Interaction Component Implementation

#include "InteractionComponent.h"
#include "InteractableInterface.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f; // Tick every frame, but trace at TraceInterval
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastTrace += DeltaTime;

	if (TimeSinceLastTrace >= TraceInterval)
	{
		PerformInteractionTrace();
		TimeSinceLastTrace = 0.0f;
	}
}

void UInteractionComponent::PerformInteractionTrace()
{
	FVector TraceStart, TraceEnd;
	GetTracePositions(TraceStart, TraceEnd);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;

	// Sphere trace for better detection
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(InteractionRadius),
		QueryParams
	);

	// Debug visualization
	if (bShowDebugTraces)
	{
		FColor TraceColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, TraceColor, false, TraceInterval, 0, 2.0f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, InteractionRadius, 12, FColor::Green, false, TraceInterval);
		}
	}

	AActor* HitActor = nullptr;

	if (bHit && HitResult.GetActor())
	{
		// Check if the actor implements the interactable interface
		if (HitResult.GetActor()->Implements<UInteractableInterface>())
		{
			// Check if it can be interacted with
			APawn* OwnerPawn = Cast<APawn>(GetOwner());
			bool bCanInteract = IInteractableInterface::Execute_CanInteract(HitResult.GetActor(), OwnerPawn);

			if (bCanInteract)
			{
				HitActor = HitResult.GetActor();
			}
		}
	}

	SetFocusedActor(HitActor);
}

void UInteractionComponent::SetFocusedActor(AActor* NewFocusedActor)
{
	if (FocusedActor != NewFocusedActor)
	{
		APawn* OwnerPawn = Cast<APawn>(GetOwner());

		// Unfocus old actor
		if (FocusedActor && FocusedActor->Implements<UInteractableInterface>())
		{
			IInteractableInterface::Execute_OnUnfocused(FocusedActor, OwnerPawn);
		}

		// Focus new actor
		FocusedActor = NewFocusedActor;

		if (FocusedActor && FocusedActor->Implements<UInteractableInterface>())
		{
			IInteractableInterface::Execute_OnFocused(FocusedActor, OwnerPawn);

			// Get prompt and broadcast
			FText Prompt = IInteractableInterface::Execute_GetInteractionPrompt(FocusedActor);
			OnInteractionPromptChanged.Broadcast(true, Prompt);
		}
		else
		{
			OnInteractionPromptChanged.Broadcast(false, FText::GetEmpty());
		}

		OnInteractableFocusChanged.Broadcast(FocusedActor);
	}
}

bool UInteractionComponent::TryInteract()
{
	if (!FocusedActor)
	{
		return false;
	}

	if (!FocusedActor->Implements<UInteractableInterface>())
	{
		return false;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	// Final check before interacting
	if (!IInteractableInterface::Execute_CanInteract(FocusedActor, OwnerPawn))
	{
		return false;
	}

	// Execute the interaction
	bool bSuccess = IInteractableInterface::Execute_OnInteract(FocusedActor, OwnerPawn);

	// Force trace update after interaction (object may have been destroyed or changed state)
	ForceTraceUpdate();

	return bSuccess;
}

FText UInteractionComponent::GetCurrentPrompt() const
{
	if (!FocusedActor || !FocusedActor->Implements<UInteractableInterface>())
	{
		return FText::GetEmpty();
	}

	return IInteractableInterface::Execute_GetInteractionPrompt(FocusedActor);
}

void UInteractionComponent::ForceTraceUpdate()
{
	PerformInteractionTrace();
	TimeSinceLastTrace = 0.0f;
}

UCameraComponent* UInteractionComponent::GetCamera() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return Owner->FindComponentByClass<UCameraComponent>();
}

void UInteractionComponent::GetTracePositions(FVector& OutStart, FVector& OutEnd) const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		OutStart = GetOwner()->GetActorLocation();
		OutEnd = OutStart + GetOwner()->GetActorForwardVector() * InteractionRange;
		return;
	}

	// Try to use camera direction first (better for third-person)
	UCameraComponent* Camera = GetCamera();
	if (Camera)
	{
		OutStart = Camera->GetComponentLocation();
		OutEnd = OutStart + Camera->GetForwardVector() * InteractionRange;
		return;
	}

	// Fallback to pawn view
	FVector ViewLocation;
	FRotator ViewRotation;
	OwnerPawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	OutStart = ViewLocation;
	OutEnd = OutStart + ViewRotation.Vector() * InteractionRange;
}
