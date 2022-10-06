#include "AttackState.h"

void FAttackState::Enter()
{
  // gameMode.GetMapCursorPawn()->DisableInput(gameMode.GetWorld()->GetFirstPlayerController());
  Invoke(&AMapCursorPawn::DisableInput, *gameMode.GetMapCursorPawn(),
         gameMode.GetWorld()->GetFirstPlayerController());
}

void FAttackState::Exit()
{
  Invoke(&AMapCursorPawn::EnableInput, *gameMode.GetMapCursorPawn(), gameMode.GetWorld()->GetFirstPlayerController());
  // gameMode.GetMapCursorPawn()->EnableInput(gameMode.GetWorld()->GetFirstPlayerController());
  if (SingleSubmitHandler.IsValid())
  {
    SingleSubmitHandler->EndInteraction();
  }
}

