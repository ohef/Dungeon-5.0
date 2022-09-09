// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "EasyMenu.generated.h"

UCLASS()
class UEasyMenuEntry : public UObject
{
  GENERATED_BODY()

public:
  FString Label;
  TFunction<void()> handler;

  UFUNCTION()
  void Call()
  {
    handler();
  }
};

/**
 * A vertical box widget is a layout panel allowing child widgets to be automatically laid out
 * vertically.
 *
 * * Many Children
 * * Flows Vertical
 */
UCLASS()
class UEasyMenu : public UVerticalBox
{
  GENERATED_BODY()

public:
  TArray<FString> Entries;
  UVerticalBoxSlot* AddEntry(UEasyMenuEntry* entry);
};

inline UVerticalBoxSlot* UEasyMenu::AddEntry(UEasyMenuEntry* entry)
{
  auto Button = NewObject<UButton>(this, UButton::StaticClass());
  auto TextBlock = NewObject<UTextBlock>(this, UTextBlock::StaticClass());
  TextBlock->Text = FText::FromString(entry->Label);
  Button->AddChild(TextBlock);
  Button->OnClicked.AddDynamic(entry, &UEasyMenuEntry::Call);
  return this->AddChildToVerticalBox(Button);

  // this->GetWorld()->GetFirstPlayerController()->GetLocalPlayer();
  // if (LocalPlayer)
  // {
  // 	NewWidget->SetPlayerContext(FLocalPlayerContext(LocalPlayer, World));
  // }
  // Button->Initialize();
}