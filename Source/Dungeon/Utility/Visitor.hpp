
// Visitor template declaration
template<typename... Types>
class Visitor;

template<typename ...T>
class Visitor<TVariant<T...>> : public Visitor<T...>{
};

// specialization for single type    
template<typename T>
class Visitor<T> {
public:
  virtual ~Visitor() = default;
  
  virtual void operator()(T & visitable) = 0;
};

// specialization for multiple types
template<typename T, typename... Types>
class Visitor<T, Types...> : public Visitor<Types...> {
public:
    // promote the function(s) from the base class
    using Visitor<Types...>::operator();

    virtual void operator()(T & visitable) = 0;
};

