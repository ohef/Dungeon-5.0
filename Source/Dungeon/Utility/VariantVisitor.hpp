#pragma once

template <typename TRet,typename... T>
class TVariantVisitor;

template <typename TRet,typename... T>
class TVariantVisitor<TRet,TVariant<T...>> : public TVariantVisitor<TRet, T...>
{
};

template <typename TRet, typename TArg>
class TVariantVisitor<TRet,TArg>
{
public:
  virtual ~TVariantVisitor() = default;

  virtual TRet operator()(TArg& visitable)
  {
    return TRet();
  };
};

template <typename TRet, typename TArg, typename... TTypes>
class TVariantVisitor<TRet,TArg, TTypes...> : public TVariantVisitor<TRet, TTypes...>
{
public:
  using TVariantVisitor<TRet,TTypes...>::operator();

  virtual TRet operator()(TArg& visitable)
  {
    return TRet();
  };
};
