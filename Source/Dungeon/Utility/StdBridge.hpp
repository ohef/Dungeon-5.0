#pragma once

#include <optional>

template <typename T>
TOptional<T> ConvertToTOptional(std::optional<T>&& stdOptional)
{
  return stdOptional.has_value() ? stdOptional.value() : TOptional<T>();
}

