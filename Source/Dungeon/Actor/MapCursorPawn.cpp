// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/MapCursorPawn.h"

#include <Kismet/KismetMathLibrary.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Engine/StaticMeshActor.h>
#include <Core/Public/Containers/Array.h>

#include "DungeonConstants.h"
#include "Components/CapsuleComponent.h"
#include "Components/DrawFrustumComponent.h"
#include "Dungeon/DungeonGameModeBase.h"
#include "Dungeon/Lenses/model.hpp"
#include "GameFramework/SpringArmComponent.h"
#include "Utility/ContextSwitchVisitor.hpp"

AMapCursorPawn::AMapCursorPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;

  CursorCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BoxCollider"));
  RootComponent = CursorCollider;

  Offset = CreateDefaultSubobject<USceneComponent>(TEXT("MapCursorOffset"));
  Offset->SetWorldLocation({0, 0, 0});
  CursorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorGraphic"));
  CursorMesh->SetupAttachment(Offset);
  // CursorMesh->SetRelativeLocation({50, 50, 0});

  SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("USpringArmComponent"));
  SpringArmComponent->SetupAttachment(CursorCollider);
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

  storedZoomLevels.Push(ZoomLevelNode{500});
  storedZoomLevels.Push(ZoomLevelNode{1000});
  storedZoomLevels.Push(ZoomLevelNode{1500});

  for (int i = 0; i < storedZoomLevels.Num() - 1; i++)
  {
    storedZoomLevels[i].NextNode = storedZoomLevels.GetData() + i + 1;
  }
  storedZoomLevels[storedZoomLevels.Num() - 1].NextNode = storedZoomLevels.GetData();

  currentZoom = storedZoomLevels.GetData();
  previousZoom = storedZoomLevels.GetData()->ZoomLevel;
}

STRUCT_CONTEXT_SWITCH_VISITOR(FContextHandlerr)
{
	STRUCT_CONTEXT_SWITCH_VISITOR_BODY(FContextHandlerr)

  AMapCursorPawn* MapCursorPawn;
  FDelegateHandle QueryDelegate;
  bool bound = false;

  FContextHandlerr(AMapCursorPawn* MapCursorPawn)
    : MapCursorPawn(MapCursorPawn), QueryDelegate(FDelegateHandle())
  {
  }

	template <class Tx>
	void transition(Tx previouss, FSelectingUnitContext nextt)
	{
    UKismetSystemLibrary::PrintString(MapCursorPawn, "Entering and binding delegate");
	  // QueryDelegate = MapCursorPawn->QueryInput.AddRaw(MapCursorPawn, &AMapCursorPawn::HandleSelectingQuery);
    QueryDelegate = MapCursorPawn->QueryInput.AddUObject(MapCursorPawn, &AMapCursorPawn::HandleSelectingQuery);
    // bound = true;
	}
	
	template <class Tx>
	void transition(FSelectingUnitContext previouss, Tx nextt)
	{
    UKismetSystemLibrary::PrintString(MapCursorPawn, "Entering and unbinding delegate");
    // if(bound)
    // if(QueryDelegate.IsValid())
    // {
      // MapCursorPawn->QueryInput.Remove(QueryDelegate);
      MapCursorPawn->QueryInput.RemoveAll(MapCursorPawn);
      // bound = false;
    // }
	}
};

// struct FContextHandlerNew
// {
//   AMapCursorPawn* MapCursorPawn;
//   
//   template <typename Tx, TEnableIf<TNot<TIsSame<Tx, FSelectingUnitContext>>::Value>::Type = 0>
//   void operator()(Tx l, FSelectingUnitContext r)
//   {
//     UKismetSystemLibrary::PrintString(MapCursorPawn, "test");
//   }
//
//   template <typename Tx, TEnableIf<TNot<TIsSame<Tx, FSelectingUnitContext>>::Value>::Type = 0>
//   void operator()(FSelectingUnitContext l, Tx r)
//   {
//     UKismetSystemLibrary::PrintString(MapCursorPawn, "test2");
//   }
//   
//   template<typename Tx>
//   void operator()(Tx l, Tx r)
//   {
//     UKismetSystemLibrary::PrintString(MapCursorPawn, "test3");
//   }
//   
//   template<typename Tx, typename Ty, TEnableIf<TNot<
//     TOr<
//       TIsSame<typename TDecay<Tx>::Type, FSelectingUnitContext>,
//       TIsSame<typename TDecay<Ty>::Type, FSelectingUnitContext>
//       >
//     >::Value>::Type = 0>
//   void operator()(Tx l, Ty r)
//   {
//     UKismetSystemLibrary::PrintString(MapCursorPawn, "test3");
//   }
// };

void AMapCursorPawn::HandleSelectingQuery(FIntPoint queryPt) {
  auto foundUnitOpt = UseState(interactionContextLens
                     | unreal_alternative_pipeline_t<FSelectingUnitContext>()
                     | map_opt(lager::lenses::attr(&FSelectingUnitContext::unitUnderCursor))
                     | or_default)
                   .make().get();
  
  

  if (!foundUnitOpt.IsSet())
    return;

  auto foundUnitId = foundUnitOpt.GetValue() ;
  auto isFinished = !UseViewState(isUnitFinishedLens2(foundUnitId)).IsSet();
  
  auto& TurnState = UseViewState(attr(&FDungeonWorldState::TurnState));
  auto DungeonLogicUnit = UseViewState(unitDataLens(foundUnitId) | ignoreOptional);
  
  auto isOnTeam = TurnState.teamId == DungeonLogicUnit.teamId ;
  if (isFinished && isOnTeam)
  {
    StoreDispatch(TAction(TInPlaceType<FChangeState>{}, TInteractionContext(TInPlaceType<FUnitMenu>{}, foundUnitId)));
  }
}


void AMapCursorPawn::BeginPlay()
{
  Super::BeginPlay();

  interactionContextReader = UseState(interactionContextLens).make();
  interactionContextReader.bind(TPreviousHookFunctor<TInteractionContext>(interactionContextReader.get(),
  	                                      [this](auto&& previous, auto&& next)
  	                                      {
  		                                      Visit(FContextHandlerr(this),
  		                                            Forward<decltype(previous)>(previous),
  		                                            Forward<decltype(next)>(next));
  		                                      return next;
  	                                      }));
  
  reader = UseState(SimpleCastTo<FDungeonWorldState>).make();
  reader.bind([&](const FDungeonWorldState& model)
  {
    Visit(lager::visitor{
            [&](auto context)
            {
              if constexpr (isInGuiControlledState<decltype(context)>())
              {
                MovementComponent->SetActive(false);
              }
              else
              {
                MovementComponent->SetActive(true);
              }
            },
          }, model.InteractionContext);
  });
}

void AMapCursorPawn::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  SpringArmComponent->TargetArmLength = previousZoom =
    FMath::FInterpTo(previousZoom, currentZoom->ZoomLevel, DeltaTime, 50.0);

  FVector currentLocation = this->GetActorLocation();

  const FIntPoint quantized = FIntPoint{
    (FMath::CeilToInt((currentLocation.X - TILE_POINT_SCALE * .5) / TILE_POINT_SCALE)),
    (FMath::CeilToInt((currentLocation.Y - TILE_POINT_SCALE * .5) / TILE_POINT_SCALE))
  };

  auto toPoint = FVector(quantized) * TILE_POINT_SCALE + FVector{0, 0, 1.0};
  Offset->SetWorldLocation(toPoint, true);

  if (quantized == CurrentPosition)
    return;

  StoreDispatch(TAction(TInPlaceType<FCursorPositionUpdated>{}, quantized));
  
  CursorEvent.Broadcast(quantized);
  DynamicCursorEvent.Broadcast(quantized);
  CurrentPosition = quantized;
}

inline FVector4 AMapCursorPawn::ConvertInputToCameraPlaneInput(FVector inputVector)
{
  FVector cameraForward = Camera->GetForwardVector();
  const FVector planeForward = UKismetMathLibrary::ProjectVectorOnToPlane(cameraForward, {0.0f, 0.0f, 1.0f});
  const FVector planeOrthoganal = UKismetMathLibrary::Cross_VectorVector(planeForward, {0.0f, 0.0f, -1.0f});
  return FMatrix(planeOrthoganal, planeForward, FVector::ZeroVector, FVector::ZeroVector).TransformVector(inputVector);
}

void AMapCursorPawn::MoveRight(float Value)
{
  AddMovementInput(ConvertInputToCameraPlaneInput(FVector{Value, 0.0, 0.0}));
}

void AMapCursorPawn::MoveUp(float Value)
{
  AddMovementInput(ConvertInputToCameraPlaneInput(FVector{0.0, Value, 0.0}));
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
  previousZoom = currentZoom->ZoomLevel;
  currentZoom = currentZoom->NextNode;
}

void AMapCursorPawn::Query()
{
  QueryInput.Broadcast(CurrentPosition);
}

void AMapCursorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  PlayerInputComponent->BindAxis(GMoveRight, this, &AMapCursorPawn::MoveRight);
  PlayerInputComponent->BindAxis(GMoveUp, this, &AMapCursorPawn::MoveUp);
  PlayerInputComponent->BindAxis(GCameraRotate, this, &AMapCursorPawn::RotateCamera);
  PlayerInputComponent->BindAction(GQuery, EInputEvent::IE_Pressed, this, &AMapCursorPawn::Query);
  PlayerInputComponent->BindAction(GZoom, EInputEvent::IE_Pressed, this, &AMapCursorPawn::CycleZoom);
}
