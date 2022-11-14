#pragma once

template<typename T>
struct FContextSwitchVisitor
{
	~FContextSwitchVisitor() = default;
	
	template <class Tx, class Ty>
	void transition(Tx previouss, Ty nextt)
	{
	}

	template <class Tx>
	void loop(Tx previouss, Tx nextt)
	{
	}
	
	template <class Tx, class Ty>
	void operator()(Tx previouss, Ty nextt)
	{
		auto dThis = static_cast<T*>(this);
		if constexpr (TNot<TIsSame<Tx, Ty>>::value)
		{
			dThis->transition(previouss, nextt);
		}
		else 
		{
			dThis->loop(previouss, nextt);
		}
	}
};

#define STRUCT_CONTEXT_SWITCH_VISITOR(ClassName_) \
struct ClassName_ : FContextSwitchVisitor<ClassName_>

#define STRUCT_CONTEXT_SWITCH_VISITOR_BODY(ClassName_) \
using FContextSwitchVisitor<ClassName_>::transition;\
using FContextSwitchVisitor<ClassName_>::loop;