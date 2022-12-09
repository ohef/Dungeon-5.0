// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonUnitActor.h"

#include "DungeonConstants.h"
#include "DungeonGameModeBase.h"
#include "GraphAStar.h"
#include "Algo/ForEach.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "lager/lenses/tuple.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Utility/HookFunctor.hpp"

// Sets default values
ADungeonUnitActor::ADungeonUnitActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<UDamageWidget> DamageWidgetClasss(
		TEXT("/Game/Blueprints/Widgets/DamageWidget"));

	DamageWidgetClass = DamageWidgetClasss.Class;
	InterpToMovementComponent = CreateDefaultSubobject<UInterpToMovementComponent>(TEXT("InterpToMovementComponent"));
	this->RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")); 
	PathRotation = CreateDefaultSubobject<USceneComponent>(TEXT("PathRotation"));
	PathRotation->AttachToComponent(this->RootComponent,
	                                FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
	
	// InterpToMovementComponent->UpdatedComponent = this->RootComponent;
	// InterpToMovementComponent->BehaviourType = EInterpToBehaviourType::PingPong;
	// InterpToMovementComponent->SetActive(true);
	// InterpToMovementComponent->SetComponentTickEnabled(true);
}

// Called when the game starts or when spawned
void ADungeonUnitActor::BeginPlay()
{
	Super::BeginPlay();

	auto damageWidgetToAdd = NewObject<UDamageWidget>(this, DamageWidgetClass);
	DamageWidget = damageWidgetToAdd;
	DamageWidget->SetPlayerContext(FLocalPlayerContext(this->GetWorld()->GetFirstPlayerController()));
	DamageWidget->Initialize();
	DamageWidget->AddToViewport();
	DamageWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void ADungeonUnitActor::hookIntoStore()
{
	reader = UseState(lager::lenses::fan(thisUnitLens(id), unitIdToPosition(id), isUnitFinishedLens2(id)))
	.make();

	UseEvent().AddUObject(this, &ADungeonUnitActor::HandleGlobalEvent);

	lastPosition = lager::view(second, reader.get());
	reader.bind(TPreviousHookFunctor<decltype(reader)::value_type>(
		reader.get(),
		[&](auto&& previousReaderVal, auto&& ReaderVal)
		{
			FDungeonLogicUnit DungeonLogicUnit = lager::view(first, ReaderVal);
			FIntPoint UpdatedPosition = lager::view(second, previousReaderVal);
			lastPosition = UpdatedPosition;
			DungeonLogicUnit.state = lager::view(element<2>, ReaderVal).IsSet()
				                         ? UnitState::ActionTaken
				                         : UnitState::Free;

			this->React(DungeonLogicUnit);
			return ReaderVal;
		}));
}

// s

void ADungeonUnitActor::HandleGlobalEvent(const TDungeonAction& action)
{
	Visit([this](auto&& event)
	{
		using TEventType = typename TDecay<decltype(event)>::Type;
		if constexpr (TIsSame<FCombatAction, TEventType>::Value)
		{
			const FCombatAction& castedEvent = event;
			if (lager::view(first, reader.get()).Id == castedEvent.updatedUnit.Id)
			{
				FVector2D ScreenPosition;
				const FIntPoint& IntPoint = lager::view(second, this->reader.get());
				UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
					this->GetWorld()->GetFirstPlayerController(), TilePositionToWorldPoint(IntPoint),
					ScreenPosition,
					false);
				DamageWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
				DamageWidget->SetRenderTranslation(FVector2D(ScreenPosition));
				DamageWidget->DamageDone->SetText(FText::AsNumber(castedEvent.damageValue));
				DamageWidget->DamageDone->SetRenderOpacity(1.0);
				DamageWidget->PlayAnimation(DamageWidget->DamageDissappear.Get());

				UKismetSystemLibrary::PrintString(
					this, "Wow you dealt THIS much damage bro GOOD JOB " + FString::FormatAsNumber(
						castedEvent.damageValue));
			}
		}
		else if constexpr (TIsSame<FMoveAction, TEventType>::Value)
		{
			const FMoveAction& castedEvent = event;
			if (lager::view(first, reader.get()).Id == castedEvent.InitiatorId)
			{
				auto ReaderVal = reader.get();
				FDungeonLogicUnit DungeonLogicUnit = lager::view(first, ReaderVal);
				FIntPoint UpdatedPosition = lager::view(second, ReaderVal);

				TArray<FIntPoint> aStarResult;
				FSimpleTileGraph SimpleTileGraph = FSimpleTileGraph(UseViewState(attr(&FDungeonWorldState::map)),
				                                                    DungeonLogicUnit.Movement);
				FGraphAStar aStarGraph(SimpleTileGraph);
				auto Result = aStarGraph.FindPath(lastPosition, UpdatedPosition, SimpleTileGraph, aStarResult);

				InterpToMovementComponent->ResetControlPoints();
				InterpToMovementComponent->Duration = aStarResult.Num() / 3.0f;
				InterpToMovementComponent->AddControlPointPosition(TilePositionToWorldPoint(lastPosition), false);
				Algo::ForEach(aStarResult, [&](auto x) mutable
				{
					InterpToMovementComponent->AddControlPointPosition(TilePositionToWorldPoint(x), false);
				});
				InterpToMovementComponent->FinaliseControlPoints();
				InterpToMovementComponent->RestartMovement();
			}
		}
		else if constexpr (TIsSame<FBackAction, TEventType>::Value) {
			SetActorLocation(TilePositionToWorldPoint(reader.zoom(second).make().get()));
		}
	}, action);
}

// Called every frame
void ADungeonUnitActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	PathRotation->SetWorldRotation(InterpToMovementComponent->Velocity.ToOrientationRotator());
}
