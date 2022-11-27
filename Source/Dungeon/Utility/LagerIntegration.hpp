#pragma once

#include "zug/compose.hpp"

template <typename T>
struct unreal_alternative_pipeline_t : zug::detail::pipeable
{
	using Part = std::optional<T>;
	template <typename F>
	auto operator()(F&& f) const
	{
		return [f = Forward<F>(f)](auto&& p)
		{
			using Whole = typename TDecay<decltype(p)>::Type;
			return f([&]() -> Part
			{
				if (p.template IsType<T>())
				{
					return p.template Get<T>();
				}
				else
				{
                    return std::nullopt;
				}
			}())([&](Part x) -> Whole
			{
				if (x.has_value() && p.template IsType<T>())
				{
                    return Whole(TInPlaceType<T>{}, std::move(x).value());
				}
				else
				{
					return LAGER_FWD(p);
				}
			});
		};
	}
};

//TODO: I'm just being lazy and using lagers optional chaining instead of writing my own version for unreal, what a pain
template <typename T>
inline auto unreal_alternative_pipeline = unreal_alternative_pipeline_t<T>{};

template <typename T>
struct unreal_alternative_t : zug::detail::pipeable
{
	using Part = TOptional<T>;

	template <typename F>
	auto operator()(F&& f) const
	{
		return [f = Forward<F>(f)](auto&& p)
		{
			using Whole = typename TDecay<decltype(p)>::Type;
			return f([&]() -> Part
			{
				if (p.template IsType<T>())
				{
					return p.template Get<T>();
				}
				else
				{
					return TOptional<T>();
				}
			}())([&](Part x) -> Whole
			{
				if (x.IsSet() && p.template IsType<T>())
				{
					return decltype(p)(MoveTemp(x).GetValue());
				}
				else
				{
					return LAGER_FWD(p);
				}
			});
		};
	}
};

template <typename T>
inline auto unreal_alternative = unreal_alternative_t<T>{};

constexpr auto deref =
	zug::comp([](auto&& f)
	{
		return [&, f = LAGER_FWD(f)](auto&& p)
		{
			return f(LAGER_FWD(*p))
			([&](auto&& x)
			{
				*p = LAGER_FWD(x);
				return p;
			});
		};
	});

const auto getter = [](auto member)
{
	return zug::comp([member](auto&& f)
	{
		return [&, f = LAGER_FWD(f)](auto&& p)
		{
			return f((p->*member)())([&](auto&& x) -> decltype(p)
			{
				return p;
			});
		};
	});
};

template <typename Key>
auto Find(Key key)
{
	return zug::comp([key](auto&& f)
	{
		return [f = LAGER_FWD(f), &key](auto&& whole)
		{
			using Part = TOptional<std::decay_t<decltype(*(whole.Find(key)))>>;
			using Whole = decltype(whole);
			return f([&]() -> Part
			{
				if (LAGER_FWD(whole).Contains(key))
				{
					return *LAGER_FWD(whole).Find(key);
				}
				return Part();
			}())([&](Part part)
			{
				auto r = std::forward<Whole>(whole);
				if (part.IsSet() && r.Contains(key))
				{
					if constexpr (TIsTMap<typename TDecay<Whole>::Type>::Value)
					{
						r.Emplace(key, std::forward<Part>(part).GetValue());
					}
				}
				return r;
			});
		};
	});
}

constexpr auto ignoreOptional = zug::comp([](auto&& f)
{
	return [f](auto&& p)
	{
		using Part = typename TDecay<decltype(p.GetValue())>::Type;
		return f(Part(LAGER_FWD(p).GetValue()))(
			[&](auto&& x) { return TOptional<std::decay_t<decltype(p)>>(LAGER_FWD(x)); });
	};
});

/*!
 * `X -> Lens<[X], X>`
 */
template <typename T>
auto unreal_value_or(T&& t)
{
    return zug::comp([t = std::forward<T>(t)](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            return f(LAGER_FWD(whole).Get(std::move(t)))(
                [&](auto&& x) { return LAGER_FWD(x); });
        };
    });
}
