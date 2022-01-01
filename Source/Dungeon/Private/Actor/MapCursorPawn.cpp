// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/MapCursorPawn.h"

#include <Kismet/KismetMathLibrary.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Engine/StaticMeshActor.h>
#include <Core/Public/Containers/Array.h>

#include "DungeonConstants.h"
#include "Components/CapsuleComponent.h"
#include "Components/DrawFrustumComponent.h"


AMapCursorPawn::AMapCursorPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

  CursorCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BoxCollider"));
  RootComponent = CursorCollider;

  Offset = CreateDefaultSubobject<USceneComponent>(TEXT("MapCursorOffset"));
  Offset->SetWorldLocation({0,0,0});
  CursorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorGraphic"));
  CursorMesh->SetupAttachment(Offset);
  CursorMesh->SetRelativeLocation({50, 50, 0});

  Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
  Camera->SetupAttachment(CursorCollider);
  Camera->bAutoActivate = true;
	Camera->ComponentTags = TArray<FName> ();
	Camera->ComponentTags.Reserve(1);
	Camera->ComponentTags.Add(FName(TEXT("main")));
  Camera->SetRelativeLocation({ 1250,1250,1250 });
  Camera->SetRelativeRotation(UKismetMathLibrary::FindLookAtRotation(Camera->GetRelativeLocation(), FVector::ZeroVector));

  FrustumComponent = CreateDefaultSubobject<UDrawFrustumComponent>(TEXT("UDrawFrustumComponent"));
  
  cursorState = CursorState::Free;
}

void AMapCursorPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AMapCursorPawn::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  FVector PendingMovementInputVector = GetPendingMovementInputVector();
  if (PendingMovementInputVector.IsNearlyZero()) return;

  const int movementStrength = 2000;
  const FVector forwardVector = UKismetMathLibrary::ProjectVectorOnToPlane(Camera->GetForwardVector(), {0.0f, 0.0f, 1.0f});
  const FVector orthVector = UKismetMathLibrary::Cross_VectorVector(forwardVector, {0.0f, 0.0f, -1.0f});
  FVector inputVector = this->ConsumeMovementInputVector();
  inputVector.Normalize();
  const FVector scaledInputVector = inputVector * movementStrength * DeltaTime;

  FHitResult OutSweepHitResult = FHitResult();
  FVector DeltaLocation = orthVector * scaledInputVector.X + forwardVector * scaledInputVector.Y;
  this->AddActorWorldOffset(DeltaLocation, true, &OutSweepHitResult);
  if (OutSweepHitResult.GetActor() != NULL)
  {
    UE_LOG(LogTemp, Warning, TEXT("HEY %s"), *OutSweepHitResult.ToString());
    this->AddActorWorldOffset(UKismetMathLibrary::ProjectVectorOnToPlane(DeltaLocation, OutSweepHitResult.ImpactNormal ), true, &OutSweepHitResult);
  }
  
  FVector currentLocation = this->GetActorLocation();
  const FIntPoint quantized = FIntPoint{ static_cast<int32>(FMath::Floor(currentLocation.X / TILE_POINT_SCALE)),static_cast<int32>(FMath::Floor(currentLocation.Y / TILE_POINT_SCALE)) };

  auto fromPoint = FVector(CurrentPosition) * TILE_POINT_SCALE + FVector{0, 0, 1.0};
  auto toPoint = FVector(quantized) * TILE_POINT_SCALE + FVector{0, 0, 1.0};
  Offset->SetWorldLocation(toPoint, true);
  
  if (quantized == CurrentPosition)
    return;

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
  QueryPoint.ExecuteIfBound(CurrentPosition);
}

void AMapCursorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent(PlayerInputComponent);
  
  PlayerInputComponent->BindAxis(GMoveRight, this, &AMapCursorPawn::MoveRight);
  PlayerInputComponent->BindAxis(GMoveUp, this, &AMapCursorPawn::MoveUp);
  PlayerInputComponent->BindAxis(GCameraRotate, this, &AMapCursorPawn::RotateCamera);
  PlayerInputComponent->BindAction(GQuery, EInputEvent::IE_Pressed, this, &AMapCursorPawn::Query);
}
