// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "TurnNotifierWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/NativeWidgetHost.h"
#include "Components/VerticalBox.h"

#include "lager/cursor.hpp"
#include "lager/reader.hpp"
#include "Logic/DungeonGameState.h"
#include "Menus/MainMapMenu.h"
#include "Utility/HookFunctor.hpp"
#include "Utility/StoreConnectedClass.hpp"

#include "DungeonMainWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class DUNGEON_API UDungeonMainWidget : public UUserWidget, public FStoreConnectedClass<UDungeonMainWidget, TAction>
{
public:
  UDungeonMainWidget(const FObjectInitializer& ObjectInitializer);
  virtual bool Initialize() override;
private:
  GENERATED_BODY()

  DECLARE_DELEGATE_OneParam(FOnFocusLost, const FFocusEvent&)

public:
  FOnFocusLost FocusLost;

  UPROPERTY(meta=(BindWidgetAnim), Transient, EditAnywhere, BlueprintReadWrite)
  UWidgetAnimation* OnStart;
  
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UVerticalBox> UnitActionMenu;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Attack;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Wait;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Move;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UNativeWidgetHost> UnitDisplay;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UMainMapMenu> MainMapMenu;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UTurnNotifierWidget> TurnNotifierWidget;
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  ESlateVisibility MapMenuVisibility;
  
  UFUNCTION()
  void OnEndTurnClicked();
  
  lager::reader<TInteractionContext> contextCursor;
  lager::reader<int> turnStateReader;
};
