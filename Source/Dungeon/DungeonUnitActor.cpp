// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonUnitActor.h"

#include "DungeonConstants.h"
#include "DungeonGameModeBase.h"
#include "Editor.h"
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
		TEXT("/Game/Blueprints/Widgets/DamageWidgetBlueprint"));
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
	
	DamageWidgetComponent = CreateDefaultSubobject<UWidgetComponent>( GET_MEMBER_NAME_CHECKED(ADungeonUnitActor, DamageWidgetComponent));
	DamageWidgetComponent->SetWidgetClass(DamageWidgetClass);
	DamageWidgetComponent->SetWorldTransform(FTransform{FRotator::ZeroRotator, FVector{0, 0, 0}});
	DamageWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DamageWidgetComponent->SetDrawAtDesiredSize(true);
	
	if (USkeletalMeshComponent* SkeletalMesh = FindComponentByClass<USkeletalMeshComponent>())
	{
		DamageWidgetComponent->SetupAttachment(SkeletalMesh, "head");
	}
	else
	{
		DamageWidgetComponent->SetupAttachment(PathRotation);
	}

	UnitIndicatorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitIndicatorMeshComponent "));
	UnitIndicatorMeshComponent->SetStaticMesh(UnitIndicatorMesh);
	UnitIndicatorMeshComponent->AttachToComponent(PathRotation,
	                                              FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
	UnitIndicatorMeshComponent->SetWorldTransform(FTransform{FRotator::ZeroRotator, FVector{0, 0, 0.1}});
}

void ADungeonUnitActor::CreateDynamicMaterials()
{
	auto HealthBarWidgett = Cast<UHealthBarWidget>(HealthBarComponent->GetWidget());

	auto ResourceObject = HealthBarWidgett->HealthBarDisplay->Brush.GetResourceObject();
	auto Material = Cast<UMaterialInstance>(ResourceObject);
	if (!Material)
		UE_DEBUG_BREAK();

	HealthBarMaterialDynamic = UMaterialInstanceDynamic::Create(Material, this, "HealthBarMaterialDynamic");
	UnitIndicatorDynamic = UMaterialInstanceDynamic::Create(UnitIndicatorMaterial, this, "UnitIndicatorDynamic");
	UnitIndicatorMeshComponent->SetMaterial(0, UnitIndicatorDynamic);
}

// Called when the game starts or when spawned
void ADungeonUnitActor::BeginPlay()
{
	Super::BeginPlay();

	CreateDynamicMaterials();

	auto HealthBarWidgett = Cast<UHealthBarWidget>(HealthBarComponent->GetWidget());
	HealthBarWidgett->HealthBarDisplay->SetBrushFromMaterial(HealthBarMaterialDynamic);
}

void ADungeonUnitActor::HookIntoStore(const FDungeonStore& store, TDungeonActionDispatched& eventToUse)
{
	reader = store.zoom(
		SimpleCastTo<FDungeonWorldState>
		| lager::lenses::fan(unitDataLens(Id), unitIdToPositionOpt(Id), isUnitFinishedLens2(Id)));

	eventToUse.AddUObject(this, &ADungeonUnitActor::HandleGlobalEvent);

	static const TMap<int, FLinearColor> teamColorMap = {
		{0, FLinearColor::Blue},
		{1, FLinearColor::Red}
	};

	reader.bind(TPreviousHookFunctor<decltype(reader)::value_type>(
		reader.get(),
		[this](auto&& previousReaderVal, auto&& ReaderVal)
		{
			auto [DungeonLogicUnitOpt, UpdatedPosition , isFinished] = ReaderVal;
			if (DungeonLogicUnitOpt)
			{
				auto DungeonLogicUnit = *DungeonLogicUnitOpt;
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
			}
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
		auto [dungeonLogicUnit,x,y] = *_this->reader;
		if (dungeonLogicUnit)
		{
			auto thisId = dungeonLogicUnit->Id;
			if (event.targetedUnit == thisId)
			{
				if (UDamageWidget* widget = Cast<UDamageWidget>(_this->DamageWidgetComponent->GetWidget()))
				{
					widget->SetVisibility(ESlateVisibility::HitTestInvisible);
					widget->DamageDone->SetText(FText::AsNumber(event.damageValue));
					widget->PlayAnimation(widget->DamageDissappear.Get());
				}
			}

			if (event.InitiatorId == thisId)
			{
				_this->UseViewState(unitIdToActor(thisId))->ReactCombatAction(event);
			}
		}
	}

	void operator()(const FBackAction& event)
	{
		_this->InterpToMovementComponent->ResetControlPoints();
		_this->InterpToMovementComponent->StopMovementImmediately();
		auto unitPosition = _this->reader.zoom(second).make().get();
		if (unitPosition)
		{
			_this->SetActorLocation(TilePositionToWorldPoint(*unitPosition));
		}
	}

	void operator()(const FUnitInteraction& newState)
	{
		FVector Start = TilePositionToWorldPoint(_this->UseViewState(unitIdToPosition(newState.originatorID)));
		FVector Target = TilePositionToWorldPoint(_this->UseViewState(unitIdToPosition(newState.targetIDUnderFocus)));

		auto [dungeonLogicUnit,x,y] = *_this->reader;
		if (dungeonLogicUnit)
		{
			int thisId = dungeonLogicUnit->Id;
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
	}
};

void ADungeonUnitActor::HandleGlobalEvent(const TDungeonAction& action)
{
	Visit(FDungeonUnitActorHandler{this}, action);
}

void ADungeonUnitActor::ClearDamageTimer()
{
	this->GetWorldTimerManager().ClearTimer(this->timerHandle);
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
