// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/MapCursorPawn.h"

#include <Kismet/KismetMathLibrary.h>
#include <Core/Public/Containers/Array.h>

#include "DungeonConstants.h"
#include "DungeonUnitActor.h"
#include "Components/DrawFrustumComponent.h"
#include "Dungeon/DungeonGameModeBase.h"
#include "Dungeon/Lenses/model.hpp"
#include "GameFramework/SpringArmComponent.h"
#include "Logic/StateQueries.hpp"
#include "zug/transducer/cycle.hpp"

decltype(auto) preIncrement(auto& v) { return ++v; };
decltype(auto) preDecrement (auto& v) { return --v; };

using TNumberMutator = int&(int&);

int CycleArrayIndex(TNumberMutator* indexMover, int numberOfElements, int currentPointingIndex)
{
	auto newIndex = indexMover(currentPointingIndex);
	return newIndex < 0 ? numberOfElements - 1 : newIndex % numberOfElements;
}

AMapCursorPawn::AMapCursorPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	CursorCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BoxCollider"));
	RootComponent = CursorCollider;

	Offset = CreateDefaultSubobject<USceneComponent>(TEXT("MapCursorOffset"));
	Offset->SetWorldLocation({0, 0, 0});
	CursorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorGraphic"));
	CursorMesh->SetupAttachment(Offset);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("USpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->SetWorldRotation(FRotator(-45, 0, 0));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArmComponent);
	Camera->bAutoActivate = true;

	FrustumComponent = CreateDefaultSubobject<UDrawFrustumComponent>(TEXT("UDrawFrustumComponent"));
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("UFloatingPawnMovement"));
	MovementComponent->Acceleration = 16000;
	MovementComponent->Deceleration = 16000;

	InterpToMovementComponent = CreateDefaultSubobject<UInterpToMovementComponent>(TEXT("UInterpToMovementComponent"));
	InterpToMovementComponent->Duration = 1;
	InterpToMovementComponent->BehaviourType = EInterpToBehaviourType::OneShot;

	zoomLevelsInCentimeters = {100, 500, 1000, 1500};
	currentZoomPointer = 2;
}

const auto IsInAbilityContext = interactionContextLens
	| unreal_alternative_pipeline<FSelectingUnitAbilityTarget>
	| map_opt(attr(&FSelectingUnitAbilityTarget::abilityId));

void AMapCursorPawn::BeginPlay()
{
	Super::BeginPlay();

	auto handleInteractionContextUpdated = [&](const FChangeState& = {})
	{
		auto maybeTargeting = UseViewState(IsInAbilityContext);
		if (maybeTargeting.has_value() && *maybeTargeting == EAbilityId::IdAttack)
		{
			auto interactionPosition =
				UseViewState(unitIdToPosition(
					UseViewState(
						attr(&FDungeonWorldState::WaitingForResolution) | unreal_alternative<FCombatAction> |
						ignoreOptional).InitiatorId));

			TSet<FIntPoint> IntPoints = GetInteractablePositions(reader.get(), interactionPosition);
			cycler = TCycleArrayIterator(zug::unreal::into(TArray<FIntPoint>{}, zug::identity, IntPoints));
			currentCyclerIndex = 0;
		}
	};

	UseEvent().AddLambda(Dungeon::MatchEffect(
		[&, handleInteractionContextUpdated](const FBackAction&)
		{
			handleInteractionContextUpdated();
			if(CurrentPosition != UseViewState(cursorPositionLens))
				RootComponent->SetWorldLocation(TilePositionToWorldPoint(UseViewState(cursorPositionLens)));
		},
		handleInteractionContextUpdated
	));

	auto beginningLocation = SpringArmComponent->GetRelativeLocation();
	auto respondToAllInteractionContextSwitches = [&, beginningLocation](const auto& context) mutable 
				{
					auto CurrentRotation = SpringArmComponent->GetRelativeRotation().Euler();
					CurrentRotation[1] = -45;
					SpringArmComponent->SetRelativeRotation(FRotator::MakeFromEuler(CurrentRotation));
					SpringArmComponent->SetRelativeLocation(beginningLocation);
		
					using TContext = decltype(context);
					if constexpr (isInGuiControlledState<TContext>() || TIsInTypeUnion<TContext, FControlledInteraction>::Value)
					{
						MovementComponent->SetActive(false);
					}
					else
					{
						MovementComponent->SetActive(true);
					}
				};

	reader = UseStoreNode();
	reader.bind(zug::comp(Dungeon::Match(MoveTemp(respondToAllInteractionContextSwitches))
			(
				[this](const FUnitInteraction& interaction) mutable
				{
					MovementComponent->SetActive(false);
					auto SkeletalMeshComponentTarget =
						UseViewState(unitIdToActor(interaction.targetIDUnderFocus))
						->FindComponentByClass<USkeletalMeshComponent>();
					auto SkeletalMeshComponentInitiator =
						UseViewState(unitIdToActor(interaction.originatorID))
						->FindComponentByClass<USkeletalMeshComponent>();

					auto headPositionTarget = SkeletalMeshComponentTarget
					->GetSocketTransform("head", RTS_World)
					.GetLocation();
					auto headPositionInitiator = SkeletalMeshComponentInitiator->GetSocketTransform("head", RTS_World).
						GetLocation();

					//TODO: Revisit this math...
					SpringArmComponent->SetWorldLocation(
						FVector(1, 1, 0) * ((headPositionTarget + headPositionInitiator) / 2.0)
						+ FVector(0, 0, 1) * SpringArmComponent->GetComponentLocation());
					FVector headDirection = ( headPositionTarget - headPositionInitiator ) * FVector(1,1,0);
					auto initiatorFacingDirection = FVector::DownVector.Cross(headDirection);
					auto project = (headPositionInitiator - Camera->GetComponentLocation() * FVector(1,1,0))
						.ProjectOnTo(initiatorFacingDirection);
					FRotator rotator;
					if (project.Dot(initiatorFacingDirection) >= 0)
					{
						rotator = UKismetMathLibrary::MakeRotationFromAxes(initiatorFacingDirection, headDirection,
						                                                   FVector::UpVector);
					}
					else
					{
						rotator = UKismetMathLibrary::MakeRotationFromAxes(-initiatorFacingDirection, -headDirection,
						                                                   FVector::UpVector);
					}
					
					SpringArmComponent->SetWorldRotation(FRotator::MakeFromEuler(
						FVector(0, 0, 1) * rotator.Euler()
						+ FVector(1, 1, 0) * SpringArmComponent->GetRelativeRotation().Euler()));
				}), [](auto&& model) { return DUNGEON_FOWARD(model).InteractionContext; }));
}

void AMapCursorPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto currentZoomLevel = zoomLevelsInCentimeters[currentZoomPointer];
	SpringArmComponent->TargetArmLength = previousZoom =
		FMath::FInterpTo(previousZoom, currentZoomLevel, DeltaTime, 50.0);

	FVector currentLocation = this->GetActorLocation();

	const FIntPoint quantized = FIntPoint{
		(FMath::CeilToInt((currentLocation.X - TILE_POINT_SCALE * .5) / TILE_POINT_SCALE)),
		(FMath::CeilToInt((currentLocation.Y - TILE_POINT_SCALE * .5) / TILE_POINT_SCALE))
	};

	auto toPoint = FVector(quantized) * TILE_POINT_SCALE + FVector{0, 0, 1.0};
	Offset->SetWorldLocation(toPoint, true);

	if (quantized == CurrentPosition)
		return;

	StoreDispatch(TDungeonAction(TInPlaceType<FCursorPositionUpdated>{}, quantized));

	CursorEvent.Broadcast(quantized);
	DynamicCursorEvent.Broadcast(quantized);
	CurrentPosition = quantized;
}

inline FVector4 AMapCursorPawn::ConvertInputToCameraPlaneInput(FVector inputVector)
{
	FVector cameraForward = Camera->GetForwardVector();
	const FVector planeForward = UKismetMathLibrary::ProjectVectorOnToPlane(cameraForward, {0.0f, 0.0f, 1.0f});
	const FVector planeOrthoganal = UKismetMathLibrary::Cross_VectorVector(planeForward, {0.0f, 0.0f, -1.0f});
	return FMatrix(planeOrthoganal, planeForward, FVector::ZeroVector, FVector::ZeroVector).
		TransformVector(inputVector);
}

void AMapCursorPawn::RotateCamera(float Value)
{
	if (Value != 0.0)
	{
		SpringArmComponent->AddRelativeRotation(FRotator(0, Value * 2, 0));
	}
}

void AMapCursorPawn::CycleZoom()
{
	previousZoom = zoomLevelsInCentimeters[currentZoomPointer];
	currentZoomPointer = CycleArrayIndex(&preIncrement, zoomLevelsInCentimeters.Num(), currentZoomPointer);
}

void AMapCursorPawn::Query()
{
	StoreDispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, CurrentPosition));
}

//TODO this is horrible
void AMapCursorPawn::MoveRight(float Value)
{
	auto context = UseViewState(IsInAbilityContext);
	if (!(context.has_value() && context.value() == EAbilityId::IdAttack))
	{
		AddMovementInput(ConvertInputToCameraPlaneInput(FVector{Value, 0.0, 0.0}));
	}
}

void AMapCursorPawn::MoveUp(float Value)
{
	auto context = UseViewState(IsInAbilityContext);
	if (!(context.has_value() && context.value() == EAbilityId::IdAttack))
	{
		AddMovementInput(ConvertInputToCameraPlaneInput(FVector{0.0, Value, 0.0}));
	}
}

void AMapCursorPawn::CycleSelect(TNumberMutator* indexMover)
{
	auto context = UseViewState(IsInAbilityContext);
	if (context.has_value() && context.value() == EAbilityId::IdAttack)
	{
		currentCyclerIndex = CycleArrayIndex(indexMover, cycler.iteratee.Num(), currentCyclerIndex);
		auto maybeIntPoint = cycler.iteratee[currentCyclerIndex];
		RootComponent->SetWorldLocation(TilePositionToWorldPoint(maybeIntPoint));
	}
}

void AMapCursorPawn::Next()
{
	CycleSelect(&preIncrement);
}

void AMapCursorPawn::Previous()
{
	CycleSelect(&preDecrement);
}

void AMapCursorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto useDiscreteTargeting = [&]
	{
		auto& bindNext = PlayerInputComponent->BindAction("Next", EInputEvent::IE_Pressed, this, &AMapCursorPawn::Next);
		bindNext.bConsumeInput = false;
		auto& bindPrevious = PlayerInputComponent->BindAction("Previous", EInputEvent::IE_Pressed, this, &AMapCursorPawn::Previous);
		bindPrevious.bConsumeInput = false;

		return [&]
		{
			[&](auto&... bindings)
			{
				(PlayerInputComponent->RemoveActionBindingForHandle(bindings.GetHandle()), ...);
			}(bindNext, bindPrevious);
		};
	};

	useDiscreteTargeting();

	PlayerInputComponent->BindAxis(GMoveRight, this, &AMapCursorPawn::MoveRight);
	PlayerInputComponent->BindAxis(GMoveUp, this, &AMapCursorPawn::MoveUp);

	PlayerInputComponent->BindAxis(GCameraRotate, this, &AMapCursorPawn::RotateCamera);
	PlayerInputComponent->BindAction(GQuery, EInputEvent::IE_Pressed, this, &AMapCursorPawn::Query);
	PlayerInputComponent->BindAction(GZoom, EInputEvent::IE_Pressed, this, &AMapCursorPawn::CycleZoom);
}
