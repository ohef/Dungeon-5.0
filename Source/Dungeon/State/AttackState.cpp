#include "AttackState.h"

void FAttackState::Enter()
{
}

void FAttackState::Exit()
{
  if (SingleSubmitHandler.IsValid())
  {
    // SingleSubmitHandler->EndInteraction();
  }
}

