#pragma once

template <typename T>
auto SimpleCastTo = zug::comp([](auto&& f)
{
  return [f](auto&& p)
  {
    return f(static_cast<T>(LAGER_FWD(p)))([](auto&& x) { return LAGER_FWD(x); });
  };
});
