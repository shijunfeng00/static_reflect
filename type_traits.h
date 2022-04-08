#ifndef __TYPE_TRAITS_H__
#define __TYPE_TRAITS_H__
#include<type_traits>
#include<utility>
#include<string>
/**
 * 更多的类型萃取，实现编译期反射反射需要很多的类型匹配模板
 */
namespace type_traits
{
	/**
	 * 类的包装器，具体是和decltype结合用来自动推断类型
	 */
	template<typename Object>
	struct type_wrapper
	{
		using type=Object;
	};
	/**
	 *  对于整数的包装类，比如integer_wrapper<1234>，用于计算类型哈希值，该部分未来会废弃
	 */
	template<std::size_t integer>
	struct integer_wrapper
	{
		static constexpr auto value=integer;
		consteval operator std::size_t()
		{
			return integer;
		}
	};

	/**
	 * 判断是否是list类型,list包括：
	 * 原生数组:T a[N]
	 * std::array,std::pair,std::tuple
	 * std::vector等含有begin和end方法的容器
	 * 当然里面的静态函数提供了更精细的类型判断，也可以被调用
	 */
	template<typename T>
	struct is_list_type
	{
		template<typename U,std::size_t N>
		static consteval auto get_length(U(&&)[N])
		{
			return integer_wrapper<N>();
		}
		template<typename U,std::size_t N>
		static consteval auto get_length(U(&)[N])
		{
			return integer_wrapper<N>();
		}
		template<typename U,std::size_t N>
		static consteval auto get_length(U[N])
		{
			return integer_wrapper<N>();
		}
		template<typename U>
		static constexpr auto match_tuple_or_array(int)->decltype(std::get<0>(std::declval<U>()),std::true_type());
		template<typename U>
		static constexpr auto match_tuple_or_array(...)->std::false_type;
		template<typename U>
		static constexpr auto match_raw_array(int)->decltype(get_length(std::declval<U>()),std::true_type());
		template<typename U>
		static constexpr auto match_raw_array(...)->std::false_type;
		template<typename U>
		static constexpr auto match_has_method_begin(int)->decltype(std::declval<U>().begin(),std::true_type());
		template<typename U>
		static constexpr auto match_has_method_begin(...)->std::false_type;
		template<typename U>
		static constexpr auto match_has_method_end(int)->decltype(std::declval<U>().end(),std::true_type());
		template<typename U>
		static constexpr auto match_has_method_end(...)->std::false_type;
		template<typename T_first,typename T_second>
		static constexpr auto is_pair(std::pair<T_first,T_second>&&)->std::true_type;
		template<typename...Args>
		static constexpr auto is_tuple(std::tuple<Args...>&&)->std::true_type;
		template<typename U>
		static constexpr auto match_tuple_or_pair(int)->decltype(is_pair(std::declval<U>()),std::true_type());
		template<typename U>
		static constexpr auto match_tuple_or_pair(int)->decltype(is_tuple(std::declval<U>()),std::true_type());
		template<typename U>
		static constexpr auto match_tuple_or_pair(...)->std::false_type;
		static constexpr auto is_tuple_or_pair()
		{
			return std::is_same<decltype(match_tuple_or_pair<T>(0)),std::true_type>::value;
		}
		/**
		 * @brief 是否是可迭代的对象，或者说改类实现了begin()和end()方法
		 * 
		 * @return 
		 */
		static constexpr auto is_iterable()
		{
			constexpr bool has_begin_v=std::is_same<decltype(match_has_method_begin<T>(0)),std::true_type>::value;
			constexpr bool has_end_v=std::is_same<decltype(match_has_method_end<T>(0)),std::true_type>::value;
			return has_begin_v&has_end_v;
		}
		static constexpr auto is_tuple_or_array()
		{
			return std::is_same<decltype(match_tuple_or_array<T>(0)),std::true_type>::value;
		}
		/**
		 * @brief 是否是原生数组类型
		 */
		static constexpr auto is_raw_array()
		{
			return std::is_same<decltype(match_raw_array<T>(0)),std::true_type>::value;
		}
		static constexpr auto get_value()
		{
			if constexpr(is_tuple_or_array())
				return true;
			else if constexpr(is_raw_array())
				return true;
			else if constexpr(is_iterable())
				return true;
			else
				return false;
		}
		static constexpr auto value=get_value();
	};
	/**
	 * 判断是不是成员函数类型
	 */
	template<typename Field>
	struct is_class_method
	{
		template<typename ReturnType,typename ObjectType,typename...Args>
		static consteval int match(ReturnType(ObjectType::*)(Args...));
		template<typename F>
		static consteval auto check(int)->decltype(match(std::declval<F>()),std::true_type());
		template<typename F>
		static consteval auto check(...)->std::false_type;
		static constexpr auto value=std::is_same<decltype(check<Field>(0)),std::true_type>::value;
	};
	/**
	 * 判断是不是可以反射的类
	 */
	template<typename T>
	struct is_reflectable_class
	{
		template<typename F>
		static consteval auto check(int)->decltype(std::declval<F>().get_config(),std::true_type());
		template<typename F>
		static consteval auto check(...)->std::false_type;
		static constexpr auto value=std::is_same<decltype(check<T>(0)),std::true_type>::value;
	};

	/**
	 * 判断是不是字符串，是的话，需要加上双引号
	 */
	template<typename T>
	struct is_string
	{
		template<typename U>
		static auto match(int)->decltype(std::string{std::declval<std::remove_volatile_t<U>>()},std::true_type());
		template<typename U>
		static auto match(...)->std::false_type;
		constexpr static bool value=std::is_same<std::true_type,decltype(match<T>(0))>::value;
	};
	/**
	 * 提取出一个类的属性对应的类型信息
	 */
	template<typename Object>
	struct remove_class
	{
		template<typename Type,typename T,std::size_t N>
		static constexpr auto field_type_func(Type (T::*)[N])->type_wrapper<Type[5]>; //[error]function returning an array
		template<typename Type,typename T>
		static constexpr auto field_type_func(Type (T::*))->type_wrapper<Type>;
		using type=decltype(field_type_func(Object()))::type;
	};
	/**
	 * 模仿<type_traits>的_v和_t命名
	 */
	template<typename Object>
	using remove_class_t=remove_class<Object>::type;
	template<typename T>
	using remove_const_refertnce_t=std::remove_const_t<std::remove_reference_t<T>>;
	template<typename T>
	using add_const_lreference_t=std::add_lvalue_reference_t<std::add_const_t<T>>;

	template<typename Field>
	constexpr auto is_class_method_v=is_class_method<Field>::value;

	template<typename Object>
	constexpr bool is_string_v=is_string<Object>::value;

	template<typename T>
	constexpr auto is_reflectable_class_v=is_reflectable_class<T>::value;

	template<typename T>
	constexpr auto is_list_type_v=is_list_type<T>::value;

	template<typename T>
	constexpr auto is_raw_array_v=is_list_type<T>::is_raw_array();

	template<typename T>
	constexpr auto is_tuple_or_array_v=is_list_type<T>::is_tuple_or_array();

	template<typename T>
	constexpr auto is_tuple_or_pair_v=is_list_type<T>::is_tuple_or_pair();

	template<typename T>
	constexpr auto is_iterable_v=is_list_type<T>::is_iterable();
}
#endif
