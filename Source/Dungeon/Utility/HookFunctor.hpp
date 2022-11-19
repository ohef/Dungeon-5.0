#pragma once

template<typename T>
struct THookFunctor
{
  T previousDependencyValue;
  TFunction<void(T)> hook;

  THookFunctor(T dependencyValue, TFunction<void(T)> hook)
    : previousDependencyValue(dependencyValue), hook(hook)
  {
  }

  void operator()(T newValue)
  {
    if (previousDependencyValue == newValue)
      return;

    hook(newValue);
    previousDependencyValue = newValue;
  }
};

template<typename T>
using TReducerSignature = TFunction<T(T&& prev, T&& current)>;

template<typename T>
struct TPreviousHookFunctor;

template<typename T>
struct TPreviousHookFunctor
{
  T previousDependencyValue;
  TReducerSignature<T> hook;

  TPreviousHookFunctor(T dependencyValue, TReducerSignature<T>&& hook)
    : previousDependencyValue(dependencyValue), hook(Forward<TReducerSignature<T>>(hook) )
  {
  }

  void operator()(T nextValue)
  {
    previousDependencyValue = hook(Forward<T>(previousDependencyValue), Forward<T>(nextValue));
  }
};