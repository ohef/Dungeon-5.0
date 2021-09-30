// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/MapCursorPawn.h"

#include <Kismet/KismetMathLibrary.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Engine/StaticMeshActor.h>
#include <Core/Public/Containers/Array.h>

#include "DungeonConstants.h"

#define CONSTANT_STRING(x) FName G ## x = #x

CONSTANT_STRING(MoveRight);
CONSTANT_STRING(MoveUp);
CONSTANT_STRING(CameraRotate);

AMapCursorPawn::AMapCursorPawn()
{
	PrimaryActorTick.bCanEverTick = true;
  RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));

  CursorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorGraphic"));
  CursorMesh->SetupAttachment(RootComponent.Get());

  Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
  Camera->bAutoActivate = true;
	Camera->ComponentTags = TArray<FName> ();
	Camera->ComponentTags.Reserve(1);
	Camera->ComponentTags.Add(FName(TEXT("main")));
  Camera->SetRelativeLocation({ 1250,1250,1250 });
  Camera->SetRelativeRotation(UKismetMathLibrary::FindLookAtRotation(Camera->GetRelativeLocation(), FVector::ZeroVector));
}

void AMapCursorPawn::BeginPlay()
{
	Super::BeginPlay();
  // FVector lastMove = {0,0,0};
  // FVector pending = {1,0,0};
  // snapUpdate(pending, lastMove, [=] { return pending; });
  // pending = {0,1,0};
  // snapUpdate(pending, lastMove, [=] { return pending; });
  // pending = {0,1,0};
  // snapUpdate(pending, lastMove, [=] { return pending; });
  // pending = {0,1,0};
  // snapUpdate(pending, lastMove, [=] { return pending; });
  // pending = {0,1,0};
  // snapUpdate(pending, lastMove, [=] { return pending; });
  // pending = {-1,-1,0};
  // snapUpdate(pending, lastMove, [=] { return pending; });
  // pending = {-1,-1,0};
  // snapUpdate(pending, lastMove, [=] { return pending; });
}

bool AMapCursorPawn::snapUpdate(FVector PendingMovementInputVector, FVector LastMovementInputVector, TFunction<FVector()> consumeVector)
{
  FVector vector = LastMovementInputVector;
  vector -= PendingMovementInputVector;
  vector.Normalize();
  UKismetSystemLibrary::PrintString(this, "From some shit " + vector.ToString(), true, false);
  bool isSnap = vector.Size() >= 0.9;
  if (isSnap) {
    FVector inputVector = consumeVector();
    inputVector.Normalize();
    FIntPoint updatedPosition = FIntPoint(inputVector.X, inputVector.Y) + CurrentPosition;
    GetWorld()->GetAuthGameMode<ADungeonGameModeBase>()->SubmitLinearAnimation(this, CurrentPosition, updatedPosition, 1.0);
    CurrentPosition = updatedPosition;
    return true;
  }
  return false;
}

void AMapCursorPawn::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  FVector PendingMovementInputVector = GetPendingMovementInputVector();
  if (PendingMovementInputVector.IsNearlyZero()) return;
  
  // if (snapUpdate(PendingMovementInputVector, GetLastMovementInputVector(),
  //                [&] { return ConsumeMovementInputVector(); })) return;

  const int movementStrength = 2000;
  const FVector forwardVector = UKismetMathLibrary::ProjectVectorOnToPlane(Camera->GetForwardVector(), {0.0f, 0.0f, 1.0f});
  const FVector orthVector = UKismetMathLibrary::Cross_VectorVector(forwardVector, {0.0f, 0.0f, -1.0f});
  const FVector inputVector = this->ConsumeMovementInputVector();
  const FVector scaledInputVector = inputVector * movementStrength * DeltaTime;

  this->AddActorWorldOffset(orthVector * scaledInputVector.X + forwardVector * scaledInputVector.Y, true);
  const FVector currentLocation = this->GetActorLocation();
  const FIntPoint quantized = FIntPoint{ static_cast<int32>(FMath::Floor(currentLocation.X / TILE_POINT_SCALE)),static_cast<int32>(FMath::Floor(currentLocation.Y / TILE_POINT_SCALE)) };

  if (quantized == CurrentPosition)
    return;

  auto fromPoint = (FVector(CurrentPosition) /*+ FVector{ 0.5,0.5,0 }*/) * TILE_POINT_SCALE + FVector{ 0,0,1.0 };
  auto toPoint = (FVector(quantized) /*+ FVector{ 0.5,0.5,0 }*/) * TILE_POINT_SCALE + FVector{ 0,0,1.0 };
  UE_LOG(LogTemp, VeryVerbose, TEXT("fromPoint: %s, toPoint: %s"), *fromPoint.ToString(), *toPoint.ToString());
  this->CursorEvent.Broadcast(quantized);
  CurrentPosition = quantized;
}

void AMapCursorPawn::MoveRight(float Value) {
  this->AddMovementInput({ Value, 0.0,0.0 });
}

void AMapCursorPawn::MoveUp(float Value) {
  this->AddMovementInput({ 0.0,Value,0.0 });
}

void AMapCursorPawn::RotateCamera(float Value) {
  if (Value != 0.0)
  {
    Camera->SetRelativeLocation(UKismetMathLibrary::RotateAngleAxis(Camera->GetRelativeTransform().GetLocation(), Value * 2, { 0,0,1 }));
    Camera->SetRelativeRotation(UKismetMathLibrary::FindLookAtRotation(Camera->GetRelativeLocation(), FVector::ZeroVector));
  }
}

void AMapCursorPawn::Query()
{
  QueryPoint.Execute(CurrentPosition);
}

void AMapCursorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent(PlayerInputComponent);
  
  PlayerInputComponent->BindAxis(GMoveRight, this, &AMapCursorPawn::MoveRight);
  PlayerInputComponent->BindAxis(GMoveUp, this, &AMapCursorPawn::MoveUp);
  PlayerInputComponent->BindAxis(GCameraRotate, this, &AMapCursorPawn::RotateCamera);
}
