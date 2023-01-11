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
#include "Logic/StateQueries.hpp"
#include "zug/transducer/cycle.hpp"

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

void AMapCursorPawn::BeginPlay()
{
  Super::BeginPlay();

  auto handleChangeState = [&](const FChangeState& = {})
  {
    auto maybeTargeting = UseViewState(interactionContextLens | unreal_alternative<FSelectingUnitAbilityTarget>);
    if (maybeTargeting.IsSet() && maybeTargeting->abilityId == EAbilityId::IdAttack)
    {
      TSet<FIntPoint> IntPoints = GetInteractablePositions(reader.get());
      cycler = TCycleArrayIterator(zug::unreal::into(TArray<FIntPoint>{}, zug::identity, IntPoints));
    }
  };
  
  UseEvent().AddLambda(Dungeon::MatchEffect(
    [&, handleChangeState](const FBackAction&)
    {
      handleChangeState();
      RootComponent->SetWorldLocation(TilePositionToWorldPoint(UseViewState(cursorPositionLens)));
    },
    handleChangeState 
  ));
  
  reader = UseState(SimpleCastTo<FDungeonWorldState>).make();
  reader.bind(zug::comp(Dungeon::Match
      (
        [&](const auto& context)
        {
          using TContext = decltype(context);
          if constexpr (isInGuiControlledState<TContext>())
          {
            MovementComponent->SetActive(false);
          }
          else
          {
            MovementComponent->SetActive(true);
          }
        }
      )())
    | [](auto&& model) { return model.InteractionContext; }); 
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
  return FMatrix(planeOrthoganal, planeForward, FVector::ZeroVector, FVector::ZeroVector).TransformVector(inputVector);
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
  StoreDispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, CurrentPosition));
}

//TODO this is horrible
void AMapCursorPawn::MoveRight(float Value)
{
  auto context = UseViewState(interactionContextLens | unreal_alternative_pipeline<FSelectingUnitAbilityTarget> | map_opt(attr( &FSelectingUnitAbilityTarget::abilityId)) );
  if (!(context.has_value() && context.value() == EAbilityId::IdAttack ))
  {
    AddMovementInput(ConvertInputToCameraPlaneInput(FVector{Value, 0.0, 0.0}));
  }
}

void AMapCursorPawn::MoveUp(float Value)
{
  auto context = UseViewState(interactionContextLens | unreal_alternative_pipeline<FSelectingUnitAbilityTarget> | map_opt(attr( &FSelectingUnitAbilityTarget::abilityId)) );
  if (!(context.has_value() && context.value() == EAbilityId::IdAttack ))
  {
    AddMovementInput(ConvertInputToCameraPlaneInput(FVector{0.0, Value, 0.0}));
  }
}

void AMapCursorPawn::Next()
{
  auto context = UseViewState(interactionContextLens | unreal_alternative_pipeline<FSelectingUnitAbilityTarget> | map_opt(attr( &FSelectingUnitAbilityTarget::abilityId)) );
  if (context.has_value() && context.value() == EAbilityId::IdAttack )
  {
    TOptional<FIntPoint> maybeIntPoint = cycler.Current();
    if(maybeIntPoint.IsSet())
    {
      RootComponent->SetWorldLocation(TilePositionToWorldPoint(*maybeIntPoint));
    }
    cycler.Forward();
  }
}

void AMapCursorPawn::Previous()
{
  auto context = UseViewState(interactionContextLens | unreal_alternative_pipeline<FSelectingUnitAbilityTarget> | map_opt(attr( &FSelectingUnitAbilityTarget::abilityId)) );
  if (context.has_value() && context.value() == EAbilityId::IdAttack )
  {
    TOptional<FIntPoint> maybeIntPoint = cycler.Current();
    if(maybeIntPoint.IsSet())
    {
      RootComponent->SetWorldLocation(TilePositionToWorldPoint(*maybeIntPoint));
    }
    cycler.Backwards();
  }
}

void AMapCursorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  auto useDiscreteTargeting = [&]
  {
    auto& bindNext = PlayerInputComponent->BindAction("Next", EInputEvent::IE_Pressed, this, &AMapCursorPawn::Next);
    bindNext.bConsumeInput = false;
    auto& bindPrevious = PlayerInputComponent->BindAction("Previous", EInputEvent::IE_Pressed , this, &AMapCursorPawn::Previous);
    bindPrevious.bConsumeInput = false;

    return [&]
    {
      [&](auto& ...bindings)
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
