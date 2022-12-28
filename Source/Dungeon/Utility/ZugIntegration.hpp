#pragma once

namespace zug
{
	namespace unreal
	{
		//Ripped from push_back iterator in the std library; don't really know if this is even a good idea 
		template <class _Container>
		class TAddIterator
		{
		public:
			_CONSTEXPR20 explicit TAddIterator(_Container& _Cont) noexcept
				: container(_STD addressof(_Cont))
			{
			}

			_CONSTEXPR20 TAddIterator& operator=(const typename _Container::ElementType& _Val)
			{
				container->Add(_Val);
				return *this;
			}

			_CONSTEXPR20 TAddIterator& operator=(typename _Container::ElementType&& _Val)
			{
				container->Add(_STD move(_Val));
				return *this;
			}

			_NODISCARD _CONSTEXPR20 TAddIterator& operator*() noexcept
			{
				return *this;
			}

			_CONSTEXPR20 TAddIterator& operator++() noexcept
			{
				return *this;
			}

			_CONSTEXPR20 TAddIterator operator++(int) noexcept
			{
				return *this;
			}

		protected:
			_Container* container;
		};

		//TODO: This is a stateful add...not sure if it's gonna be a big deal with the other method (emplace)
		template <typename CollectionT, typename XformT, typename... InputRangeTs>
		auto into(CollectionT&& col, XformT&& xform, InputRangeTs&&... ranges)
		-> CollectionT&&
		{
			transduce(std::forward<XformT>(xform),
			          output,
			          TAddIterator(col),
			          std::forward<InputRangeTs>(ranges)...);
			return std::forward<CollectionT>(col);
		}
	}
}
