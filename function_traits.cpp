// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <array>

// Type your code here, or load an example.

#include <type_traits>

#define DECL(X) decl_ptr<decltype(X), X>
typedef void* Argument;
typedef Argument* ArgumentPack;


//#define OPERATION_TARGET targetPtr
//#define OPERATION_ARG(INDEX) pack[INDEX+1]
//#define OPERATION_RETURN pack[0]
//#define OPERATION_PARAMS void * OPERATION_TARGET, ArgumentPack pack
//#define OPERATION_ARGS OPERATION_TARGET, pack


#define OPERATION_TARGET targetPtr
#define OPERATION_ARG(INDEX) pack[INDEX]
#define OPERATION_RETURN returnPtr
#define OPERATION_PARAMS void * OPERATION_TARGET, void * returnPtr, ArgumentPack pack
#define OPERATION_ARGS OPERATION_TARGET, OPERATION_TARGET, pack

typedef void(*Operation)(OPERATION_PARAMS);


template <typename T>
constexpr T& op_target(OPERATION_PARAMS)
{
  return *static_cast<T*>(OPERATION_TARGET);
}

template <typename T>
constexpr T& op_result(OPERATION_PARAMS)
{
  return *static_cast<T*>(OPERATION_RETURN);
}



template <typename T, size_t Index>
constexpr std::enable_if_t<std::is_rvalue_reference_v<T>, T> op_arg(OPERATION_PARAMS)
{
  return std::move(*static_cast<std::remove_reference_t<T>*>(OPERATION_ARG(Index)));
}
template <typename T, size_t Index>
constexpr std::enable_if_t<!std::is_rvalue_reference_v<T>, T> op_arg(OPERATION_PARAMS)
{
  return *static_cast<std::remove_reference_t<T>*>(OPERATION_ARG(Index));
}


;
template <class F>
struct function_traits;


struct no_target_tag {};

// function signature
template <class T, class R, class... Args>
struct function_traits<R(T, Args...)>
{
  using return_type = R;
  using target_parameter_type = std::conditional_t<std::is_same_v<no_target_tag, T>, void, std::remove_reference_t<T>>;
  using target_type = std::remove_cv_t<target_parameter_type >;


  static constexpr bool is_free = std::is_same_v<no_target_tag, T>;
  static constexpr bool is_member = !is_free;
  static constexpr std::size_t arity = sizeof...(Args);

  template <std::size_t N>
  struct argument
  {
    static_assert(N < arity, "error: invalid parameter index.");
    using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    static constexpr std::size_t index = N;
  };
  static constexpr Operation call_op = nullptr;


};

// function pointer
template <class R, class... Args>
struct function_traits<R(*)(Args...)> : function_traits<R(no_target_tag, Args...)>
{
  using callable_type = R(*)(Args...);



};
// member function pointer
template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)> : function_traits<R(C&, Args...)>
{
  using callable_type = R(C::*)(Args...);

};

// const member function pointer
template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const> : function_traits<R(const C&, Args...)>
{
  using callable_type = R(C::*)(Args...)const;


};

// volatile member function pointer
template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) volatile> : function_traits<R(volatile C&, Args...)>
{
  using callable_type = R(C::*)(Args...)volatile;
};

// cv
template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)const volatile> : function_traits<R(const volatile C&, Args...)>
{
  using callable_type = R(C::*)(Args...)const volatile;
};
// rvalue
template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) &&> : function_traits<R(C&&, Args...)>
{
  using callable_type = R(C::*)(Args...)&&;
};


template<class T, T Pointer>
struct decl_ptr
{
  static constexpr T pointer = Pointer;
};


template<class T, T Ptr>
struct function_traits<decl_ptr<T, Ptr>> : function_traits<T>
{
  static constexpr T pointer = decl_ptr<T, Ptr>::pointer;
};



// member object pointer
template <class C, class R>
struct function_traits<R(C::*)> : function_traits<R(C&)> {};
//functor
template <class F>
struct function_traits : function_traits<decl_ptr<decltype(&F::operator()), &F::operator()>>
{

};

template <class F>
struct function_traits<F &> : function_traits<F>
{
};

template <class F>
struct function_traits<F &&> : function_traits<F>
{
};


template <typename Function, size_t ParameterIndex>
using parameter_t = typename function_traits<Function>::template argument<ParameterIndex>::type;



template<typename Function>
using return_t = typename function_traits<Function>::return_type;
template<typename Function>
using target_t = typename function_traits<Function>::target_type;

template<typename Function, std::size_t Index>
using argument_t = typename function_traits<Function>::template argument<Index>::type;

template<typename Function>
constexpr bool is_free = function_traits<Function>::is_free;

template<typename Function>
constexpr bool returns_void_v = std::is_same_v<typename function_traits<Function>::return_type, void>;

template<typename Function>
constexpr std::size_t arity = function_traits<Function>::arity;

template<typename Function>
using callable_t = typename function_traits<Function>::callable_type;

template<typename Function>
constexpr callable_t<Function> function_ptr = function_traits<Function>::pointer;

template<typename Function, bool Free, bool Void>
using caller_enabled_t = std::enable_if_t < is_free < Function > == Free && returns_void_v<Function> == Void>;

template< typename R, typename T, std::size_t...Is >
static caller_enabled_t<T, false, false> caller(OPERATION_PARAMS)
{
  auto & result = *static_cast<R*>(OPERATION_RETURN);
  auto & target = *static_cast<target_t<T>*>(OPERATION_TARGET);
  result = (target.*function_ptr<T>)(static_cast<argument_t<T, Is>>(*static_cast<std::remove_reference_t<argument_t<T, Is>>*>(OPERATION_ARG(Is)))...);
}
template< typename R, typename T, std::size_t...Is >
static caller_enabled_t<T, true, false> caller(OPERATION_PARAMS)
{
  auto & result = *static_cast<R*>(OPERATION_RETURN);
  result = (*function_ptr<T>)(static_cast<argument_t<T, Is>>(*static_cast<std::remove_reference_t<argument_t<T, Is>>*>(OPERATION_ARG(Is)))...);
}
template< typename R, typename T, std::size_t...Is >
static caller_enabled_t<T, false, true> caller(OPERATION_PARAMS)
{
  auto & target = *static_cast<target_t<T>*>(OPERATION_TARGET);
  (target.*function_ptr<T>)(static_cast<argument_t<T, Is>>(*static_cast<std::remove_reference_t<argument_t<T, Is>>*>(OPERATION_ARG(Is)))...);
}
template< typename R, typename T, std::size_t...Is >
static caller_enabled_t<T, true, true> caller(OPERATION_PARAMS)
{
  (*function_ptr<T>)(static_cast<argument_t<T, Is>>(*static_cast<std::remove_reference_t<argument_t<T, Is>>*>(OPERATION_ARG(Is)))...);
}

template< typename R, typename T, std::size_t...Is>
static constexpr Operation make_call_operation(std::index_sequence<Is...>)
{
  return &caller< R, T, Is...>;
}
template<typename R, typename T>
static constexpr Operation make_call_operation()
{
  return make_call_operation<R, T>(std::make_index_sequence<arity<T>>());
}
template<typename R, typename T>
static constexpr Operation make_call_operation(const T&)
{
  return make_call_operation<R, T>();
}


template<typename T>
static constexpr Operation make_call_operation()
{
  return make_call_operation<return_t<T>, T>(std::make_index_sequence<arity<T>>());
}
template<typename T>
static constexpr Operation make_call_operation(const T&)
{
  return make_call_operation<return_t<T>, T>();
}






template<typename Function>
constexpr Operation call_op = function_traits<Function>::call_op;





template<typename T>
constexpr std::enable_if_t<std::is_rvalue_reference_v<T>, T>  arg2(void * ptr)
{
  return std::move(*static_cast<std::remove_reference_t<T>*>(ptr));
}
template<typename T>
constexpr std::enable_if_t<!std::is_rvalue_reference_v<T>, T>  arg2(void * ptr)
{
  return *static_cast<std::remove_reference_t<T>*>(ptr);
}


int m1(int a, int && b, int& c)
{
  return 0;
}
struct MyType
{
  int m1(int a, int && b, int& c)
  {
    return a + b + c;
  }
  int m2(int a, int && b, int& c)const
  {
    return 0;
  }
  int m3(int a, int && b, int& c)volatile
  {
    return 0;
  }
  int value = 1;
};
auto lambda1 = [](int a, int && b, int & c) {return a + b + c; };
auto lambda2 = [](int a, int && b, int & c)mutable {return 0; };


MyType t;
int res = 33;
int a = 1;
int b = 2;
int c = 3;
std::array<void*, 4> arr{ &res,&a,&b,&c };

void test1()
{

//  call_op<DECL(&MyType::m1)>(&t, arr.data());
}

void test2()
{
 // call_op<DECL(&m1)>(nullptr, arr.data());

}
void test3()
{

  //call_op<decltype(lambda1)>(&lambda1, arr.data);

}

struct holder
{
  holder() {}
  template<typename T>
  holder(T&& t)
  {
    printf("lol");
  }
};




int main()


{

  auto op2 = make_call_operation<holder>([](int a, int b)
  {
    printf("haha");
  });
  holder h;
  //arr[0] = &h;
  //op2(nullptr, arr.data());
  
  op2(nullptr, &h,arr.data());

  static_assert(is_free < DECL(&m1)>, "expected m1 to be free");
  static_assert(!is_free < DECL(&MyType::m1)>, "expected m1 to be free");

  static_assert(std::is_same_v<return_t<decltype(&m1)>, int>, "expected int return value");
  static_assert(std::is_same_v<return_t<decltype(&MyType::m1)>, int>, "expected int return value");
  static_assert(std::is_same_v<return_t<decltype(&MyType::value)>, int>, "expected int return value");
  static_assert(std::is_same_v<return_t<decltype(lambda1)>, int>, "expected int return value");
  static_assert(std::is_same_v<return_t<decltype(lambda2)>, int>, "expected int return value");


  static_assert(std::is_same_v<target_t<decltype(&m1)>, void>, "expected void target value");
  static_assert(std::is_same_v<target_t<decltype(&MyType::m1)>, MyType>, "expected MyType target value");
  static_assert(std::is_same_v<target_t<decltype(&MyType::m2)>, MyType>, "expected const MyType target value");
  static_assert(std::is_same_v<target_t<decltype(&MyType::value)>, MyType>, "expected MyType target value");
  static_assert(std::is_same_v<target_t<decltype(lambda1)>, decltype(lambda1)>, "expected lambda target value");
  static_assert(std::is_same_v<target_t<decltype(lambda2)>, decltype(lambda2)>, "expected lambda target value");

  static_assert(std::is_same_v<target_t<decltype(&m1)>, void>, "expected void target value");
  static_assert(std::is_same_v<target_t<decltype(&MyType::m1)>, MyType>, "expected MyType target value");
  static_assert(std::is_same_v<target_t<decltype(&MyType::m2)>, MyType>, "expected const MyType target value");
  static_assert(std::is_same_v<target_t<decltype(&MyType::value)>, MyType>, "expected MyType target value");
  static_assert(std::is_same_v<target_t<decltype(lambda1)>, decltype(lambda1)>, "expected lambda target value");
  static_assert(std::is_same_v<target_t<decltype(lambda2)>, decltype(lambda2)>, "expected lambda target value");


  static_assert(std::is_same_v<parameter_t<decltype(&m1), 0>, int>, "expected int param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(&m1), 1>, int&&>, "expected int&& param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(&m1), 2>, int&>, "expected int& param1 type");


  static_assert(std::is_same_v<parameter_t<decltype(&m1), 0>, int>, "expected int param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(&m1), 1>, int&&>, "expected int&& param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(&m1), 2>, int&>, "expected int& param1 type");

  static_assert(std::is_same_v<parameter_t<decltype(&MyType::m1), 0>, int>, "expected int param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(&MyType::m1), 1>, int&&>, "expected int&& param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(&MyType::m1), 2>, int&>, "expected int& param1 type");


  static_assert(std::is_same_v<parameter_t<decltype(lambda1), 0>, int>, "expected int param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(lambda1), 1>, int&&>, "expected int&& param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(lambda1), 2>, int&>, "expected int& param1 type");


  static_assert(std::is_same_v<parameter_t<decltype(lambda2), 0>, int>, "expected int param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(lambda2), 1>, int&&>, "expected int&& param1 type");
  static_assert(std::is_same_v<parameter_t<decltype(lambda2), 2>, int&>, "expected int& param1 type");





  printf("%d", std::is_same_v<parameter_t<decltype(lambda2), 0>, int>);
  return 0;
}
