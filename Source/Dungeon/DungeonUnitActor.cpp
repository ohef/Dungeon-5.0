// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonUnitActor.h"

#include "DungeonConstants.h"
#include "DungeonGameModeBase.h"
#include "GraphAStar.h"
#include "Algo/ForEach.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "lager/lenses/tuple.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Utility/HookFunctor.hpp"
#include "Widget/HealthBarWidget.h"

// Sets default values
ADungeonUnitActor::ADungeonUnitActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<UDamageWidget> DamageWidgetClasss(
		TEXT("/Game/Blueprints/Widgets/DamageWidget"));
	static ConstructorHelpers::FClassFinder<UHealthBarWidget> HealthBarWidegtClasss(
		TEXT("/Game/Blueprints/Widgets/HealthBar"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterial> HealthBarMateriall(
		TEXT("Material'/Game/Blueprints/Widgets/HealthBarMaterial.HealthBarMaterial'"));

	static ConstructorHelpers::FObjectFinder<UMaterialInstance> Team1(
		TEXT("MaterialInstanceConstant'/Game/Blueprints/Team1.Team1'"));

	UnitIndicatorMaterial = Team1.Object;

	this->UnitIndicatorMesh =
		ConstructorHelpers::FObjectFinder<UStaticMesh>(
			TEXT("StaticMesh'/Game/untitled_category/untitled_asset/Circle.Circle'")).Object;

	DamageWidgetClass = DamageWidgetClasss.Class;
	InterpToMovementComponent = CreateDefaultSubobject<UInterpToMovementComponent>(TEXT("InterpToMovementComponent"));
	this->RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	PathRotation = CreateDefaultSubobject<USceneComponent>(TEXT("PathRotation"));
	PathRotation->AttachToComponent(this->RootComponent,
	                                FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));

	HealthBarMaterial = HealthBarMateriall.Get();
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	HealthBarComponent->SetWidgetClass(HealthBarWidegtClasss.Class);
	HealthBarComponent->SetupAttachment(PathRotation);
	HealthBarComponent->SetWorldTransform(FTransform{FRotator::ZeroRotator, FVector{0, 0, 0}});
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawAtDesiredSize(true);

	UnitIndicatorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitIndicatorMeshComponent "));
	UnitIndicatorMeshComponent->SetStaticMesh(UnitIndicatorMesh);
	UnitIndicatorMeshComponent->AttachToComponent(PathRotation,
	                                              FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
	UnitIndicatorMeshComponent->SetWorldTransform(FTransform{FRotator::ZeroRotator, FVector{0, 0, 0.1}});
}

// Called when the game starts or when spawned
void ADungeonUnitActor::BeginPlay()
{
	Super::BeginPlay();

	HealthBarMaterialDynamic = UMaterialInstanceDynamic::Create(HealthBarMaterial, this, "HealthBarMaterialDynamic");
	UnitIndicatorDynamic = UMaterialInstanceDynamic::Create(UnitIndicatorMaterial, this, "UnitIndicatorDynamic");
	UnitIndicatorMeshComponent->SetMaterial(0, UnitIndicatorDynamic);

	DamageWidget = NewObject<UDamageWidget>(this, DamageWidgetClass);
	DamageWidget->SetPlayerContext(FLocalPlayerContext(this->GetWorld()->GetFirstPlayerController()));
	DamageWidget->Initialize();
	DamageWidget->AddToViewport();
	DamageWidget->SetVisibility(ESlateVisibility::Collapsed);

	auto HealthBarWidgett = Cast<UHealthBarWidget>(HealthBarComponent->GetWidget());
	HealthBarWidgett->HealthBarDisplay->SetBrushFromMaterial(HealthBarMaterialDynamic);
}

void ADungeonUnitActor::HookIntoStore()
{
	reader = UseState(lager::lenses::fan(unitIdToData(Id), unitIdToPosition(Id), isUnitFinishedLens2(Id)))
		.make();

	UseEvent().AddUObject(this, &ADungeonUnitActor::HandleGlobalEvent);

	static const TMap<int, FLinearColor> teamColorMap = {
		{0, FLinearColor::Blue},
		{1, FLinearColor::Red}};

	// lastPosition = lager::view(second, reader.get());
	reader.bind(TPreviousHookFunctor<decltype(reader)::value_type>(
		reader.get(),
		[this](auto&& previousReaderVal, auto&& ReaderVal)
		{
			auto [DungeonLogicUnit, UpdatedPosition , isFinished] = ReaderVal;
			// lastPosition = lager::view(second,previousReaderVal) ;
			DungeonLogicUnit.state = isFinished.IsSet()
				                         ? UnitState::ActionTaken
				                         : UnitState::Free;

			UnitIndicatorDynamic->SetVectorParameterValue(
				"Color", isFinished.IsSet()
				? FLinearColor::Gray
				: teamColorMap.FindChecked(DungeonLogicUnit.teamId - 1));
			HealthBarMaterialDynamic->SetScalarParameterValue(
				"HealthFraction",
				DungeonLogicUnit.HitPoints / static_cast<float>(DungeonLogicUnit.HitPointsTotal));

			this->React(DungeonLogicUnit);
			return ReaderVal;
		}));
}

const auto getId = [](FDungeonLogicUnit x) { return x.Id; };

template <size_t N>
const auto getElement = zug::comp([](auto x) { return std::get<N>(x); });

const auto getThisId = getId | getElement<0>;

struct FDungeonUnitActorHandler
{
	ADungeonUnitActor* _this;

	//Sink function; just does nothing
	template <typename T>
	void operator()(T)
	{
	}

	void operator()(const FChangeState& event)
	{
		Visit(*this, event.newState);
	}
	
	void operator()(const FCombatAction& event)
	{
		auto thisId = getThisId(*_this->reader);

		if (event.targetedUnit == thisId)
		{
			FVector2D ScreenPosition;
			const FIntPoint& IntPoint = lager::view(second, _this->reader.get());
			UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				_this->GetWorld()->GetFirstPlayerController(), TilePositionToWorldPoint(IntPoint),
				ScreenPosition,
				false);
			_this->DamageWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			_this->DamageWidget->SetRenderTranslation(FVector2D(ScreenPosition));
			_this->DamageWidget->DamageDone->SetText(FText::AsNumber(event.damageValue));
			_this->DamageWidget->DamageDone->SetRenderOpacity(1.0);
			_this->DamageWidget->PlayAnimation(_this->DamageWidget->DamageDissappear.Get());
		}

		if (event.InitiatorId == thisId)
		{
			_this->UseViewState(unitIdToActor(thisId) | ignoreOptional)->ReactCombatAction(event);
		}
	}

	void operator()(const FMoveAction& event)
	{
		//TODO: ok...where should these events be handled...
		// if (getThisId(*_this->reader) == event.InitiatorId)
		// {
		// 	auto ReaderVal = _this->reader.get();
		// 	FDungeonLogicUnit DungeonLogicUnit = lager::view(first, ReaderVal);
		// 	FIntPoint UpdatedPosition = lager::view(second, ReaderVal);
		//
		// 	TArray<FIntPoint> aStarResult;
		// 	FSimpleTileGraph SimpleTileGraph = FSimpleTileGraph(_this->UseViewState(attr(&FDungeonWorldState::Map)),
		// 	                                                    DungeonLogicUnit.Movement);
		// 	FGraphAStar aStarGraph(SimpleTileGraph);
		// 	auto Result = aStarGraph.FindPath(_this->lastPosition, UpdatedPosition, SimpleTileGraph, aStarResult);
		//
		// 	_this->InterpToMovementComponent->ResetControlPoints();
		// 	_this->InterpToMovementComponent->Duration = aStarResult.Num() / 3.0f;
		// 	_this->InterpToMovementComponent->AddControlPointPosition(TilePositionToWorldPoint(_this->lastPosition),
		// 	                                                          false);
		// 	Algo::ForEach(aStarResult, [&](auto x) mutable
		// 	{
		// 		_this->InterpToMovementComponent->AddControlPointPosition(TilePositionToWorldPoint(x), false);
		// 	});
		// 	_this->InterpToMovementComponent->FinaliseControlPoints();
		// 	_this->InterpToMovementComponent->RestartMovement();
		// }
	}

	void operator()(const FBackAction& event)
	{
		_this->InterpToMovementComponent->ResetControlPoints();
		_this->InterpToMovementComponent->StopMovementImmediately();
		_this->SetActorLocation(TilePositionToWorldPoint(_this->reader.zoom(second).make().get()));
	}

	void operator()(const FUnitInteraction& newState)
	{
		FVector Start = TilePositionToWorldPoint(_this->UseViewState(unitIdToPosition(newState.originatorID)));
		FVector Target = TilePositionToWorldPoint(_this->UseViewState(unitIdToPosition(newState.targetIDUnderFocus)));
		int thisId = *_this->reader.zoom(first | attr(&FDungeonLogicUnit::Id)).make();

		if (thisId == newState.originatorID)
		{
			FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(Start, Target);
			_this->PathRotation->SetWorldRotation(NewRotation);
		}
		else if (thisId == newState.targetIDUnderFocus)
		{
			FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(Target, Start);
			_this->PathRotation->SetWorldRotation(NewRotation);
		}
	}
};

void ADungeonUnitActor::HandleGlobalEvent(const TDungeonAction& action)
{
	Visit(FDungeonUnitActorHandler{this}, action);
}

// Called every frame
void ADungeonUnitActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//TODO: Look to see if we can do this using the component itself 
	if (InterpToMovementComponent->Velocity.Length() > 0)
	{
		PathRotation->SetWorldRotation(InterpToMovementComponent->Velocity.ToOrientationRotator());
	}
}
