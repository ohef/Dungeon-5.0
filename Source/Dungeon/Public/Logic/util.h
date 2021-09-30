#pragma once

#include <CoreMinimal.h>

TArray<FIntPoint> manhattanReachablePoints(const FIntPoint& point, int xLimit, int yLimit, int distance);

TArray<FIntPoint> pointsInSquareInclusive(const FIntPoint& offset, int xLimit, int yLimit);

template <typename T> struct TIsSmartPointer { static constexpr bool Value = false; };
template <typename T> struct TIsSmartPointer<TSharedRef<T>> { static constexpr bool Value = true; };
template <typename T> struct TIsSmartPointer<TSharedPtr<T>> { static constexpr bool Value = true; };

template<typename T>
TTuple<int, T> convertToIdTuple(const T& value)
{
  if constexpr (TOr<TIsPointer<T>, TIsSmartPointer<T>>::Value) {
    return TTuple<int, T>(value->id, *value);
  }
  else {
    return TTuple<int, T>(value.id, value);
  }
}
