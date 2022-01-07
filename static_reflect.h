#ifndef __STATIC_REFLECT_H__
#define __STATIC_REFLECT_H__
#include<tuple>
#include<string_view>
#define static_reflect(Class) Class::get_config()
template<char...args>
struct static_string //编译期静态字符串
{
	static constexpr const char str[]={args...};
	operator const char*()const{return static_string::str;}
	static constexpr std::size_t length=std::tuple_size<decltype(std::make_tuple(args...))>::value;
};
template<typename T,T...ch>
consteval auto operator""_ss() 
{
	return static_string<ch...,'\0'>();
}
/***************************************************/
template<std::size_t integer>
struct IntegerWrapper //对于整数的包装类，比如IntegerWrapper<1234>，用于计算类型哈希值，目前没有太大的作用
{
	static constexpr auto value=integer;
};
template<typename Object>
constexpr auto get_type_name() //编译期获取任意类型名称
{
	constexpr std::string_view fully_name=__PRETTY_FUNCTION__;
	constexpr std::size_t begin=fully_name.find("=")+2;
	constexpr std::size_t end=fully_name.find_last_of("]");
	constexpr auto type_name_view=fully_name.substr(begin,end-begin);
	constexpr auto indices=std::make_index_sequence<type_name_view.size()>();
	constexpr auto type_name=[&]<std::size_t...indices>(std::integer_sequence<std::size_t,indices...>)
	{
		constexpr auto str=static_string<type_name_view[indices]...,'\0'>();
		return str;
	}(indices);
	return type_name;
}
template<typename Object>
constexpr auto get_type_name_hash()  //编译期获取任意类型名称对应的哈希值
{
	constexpr std::string_view fully_name=__PRETTY_FUNCTION__;
	constexpr std::size_t begin=fully_name.find("=")+2;
	constexpr std::size_t end=fully_name.find("]");
	constexpr auto type_name_view=fully_name.substr(begin,end-begin);
	constexpr auto hash_value=[&]()
	{
		std::size_t hash_value=0;
		for(auto&it:type_name_view)
			hash_value=hash_value*37+it;
		return hash_value;
	}();
	return IntegerWrapper<hash_value>();
}
/********************************************************************************/
template<typename Object> //类的包装器，具体是和decltype结合用来自动推断类型
struct TypeWrapper
{
	using type=Object;
};
template<typename FieldTypeName,typename FieldName,typename FieldNameHash,typename GetFunc,typename SetFunc,typename FieldType>
struct Field
{
	const std::tuple<FieldTypeName,FieldName,FieldNameHash,GetFunc,SetFunc,FieldType>info;
	consteval Field(std::tuple<FieldTypeName,FieldName,FieldNameHash,GetFunc,SetFunc,FieldType>info):
		info{info}{}
		consteval auto type()const->FieldType::type;
		consteval auto get_type_name()const
		{
			return std::string_view{std::get<0>(info).str};
		}
		consteval auto get_name()const
		{
			return std::string_view{std::get<1>(info).str};
		}
		consteval auto get_type_id()const
		{
			return std::get<2>(info).value;
		}
		template<typename Object>
		constexpr auto get_value(Object&object)const
		{
			return std::get<3>(info)(object);
		}
		template<typename Object>
		constexpr void set_value(Object&object,const auto&value)const
		{
			std::get<4>(info)(object,value);
		}
};
template<typename FieldType>
struct Fields
{
	const FieldType metadata;
	consteval Fields(FieldType info):
		metadata{info}{}
		consteval auto size()const{return std::tuple_size<decltype(metadata)>::value;}
		template<typename FieldName,std::size_t index=0>
		consteval auto get_field(FieldName field_name)const
		{
			if constexpr(index>=std::tuple_size<decltype(metadata)>::value) //编译期遍历所有属性，找到符合条件的那个属性
			{                                                               //本来是打算写一个编译期哈希表，后来放弃了
				static_assert(index<std::tuple_size<decltype(metadata)>::value,"No such field.");
			}
			else
			{
				using field_name_type=std::add_lvalue_reference_t<std::add_const_t<decltype(field_name)>>;
				using this_field_name_type=decltype(std::get<1>(std::get<index>(metadata).info));
				if constexpr(std::is_same<field_name_type,this_field_name_type>::value)
				{
					return std::get<index>(metadata);
				}
				else
				{
					return get_method<FieldName,index+1>(field_name);
				} 
			}
		}
};
template<typename MethodTypeName,typename MethodName,typename MethodNameHash,typename Function,typename Parmas>
struct Method 
{
	template<typename T>
	using remove_const_ref=typename std::remove_const<typename std::remove_reference<T>::type>::type;
	template<typename T>
	using remove_rvalue_ref=typename std::remove_reference<typename std::remove_reference<T>::type>::type;
	const std::tuple<MethodTypeName,MethodName,MethodNameHash,Function,Parmas>info;
	
	consteval Method(std::tuple<MethodTypeName,MethodName,MethodNameHash,Function,Parmas>info):info{info}{}
	
	template<std::size_t i>
	consteval auto arg_type()const->remove_rvalue_ref<
		decltype(std::get<i>(std::declval<
			remove_const_ref<
				decltype(std::get<1>(std::get<4>(info)))
			>
		>()))
	>::type;
	consteval auto return_type()const->remove_const_ref<
		decltype(std::get<0>(std::get<4>(info)))
	>::type;
	
	consteval auto get_type_name()const
	{
		return std::string_view{std::get<0>(info).str};
	}
	consteval auto get_name()const
	{
		return std::string_view{std::get<1>(info).str};
	}
	consteval auto get_type_id()const
	{
		return std::get<2>(info).value;
	}
	consteval auto get_return_type_name()const
	{
		return std::string_view{::get_type_name<decltype(return_type())>().str};
	}
	 //如果为-1,返回一个tuple,否则返回具体的某个参数类型名称
	consteval auto get_args_type_name()const
	{
		using types=remove_const_ref<decltype(std::get<1>(std::get<4>(info)))>;
		constexpr auto num_args=std::tuple_size<types>::value;
	
			return []<std::size_t...i>(std::integer_sequence<std::size_t,i...>)
			{
				return std::array{
					std::string_view{::get_type_name<
						typename remove_rvalue_ref<
							decltype(std::get<i>(std::declval<types>()))
						>::type
					>().str}...
				};
			}(std::make_index_sequence<num_args>());
	}
	template<typename ObjectType,typename...Args>
	auto invoke(ObjectType&node,const Args&...args)const
	{
		return std::get<3>(info)(node,args...);
	}
	template<typename ObjectType,typename...Args>
	consteval auto constexpr_invoke(ObjectType node,const Args&...args)const
	{
		return std::get<3>(info)(node,args...);
	}
};
template<typename MethodType>
struct Methods
{
	const MethodType metadata;
	consteval Methods(MethodType info):
		metadata{info}{}
	consteval auto size()const{return std::tuple_size<decltype(metadata)>::value;}
	template<typename MethodName,std::size_t index=0>
	consteval auto get_method(MethodName field_name)const
	{
		if constexpr(index>=std::tuple_size<decltype(metadata)>::value)
		{
			static_assert(index<std::tuple_size<decltype(metadata)>::value,"No such method.");
		}
		else
		{
			using field_name_type=std::add_lvalue_reference_t<std::add_const_t<decltype(field_name)>>;
			using this_field_name_type=decltype(std::get<1>(std::get<index>(metadata).info));
			if constexpr(std::is_same<field_name_type,this_field_name_type>::value)
			{
				return std::get<index>(metadata);
			}
			else
			{
				return get_method<MethodName,index+1>(field_name);
			}
		}
	}
};
template<typename ObjectTypeWrapper,typename FieldsType,typename MethodsType>
struct Class
{
	std::tuple<ObjectTypeWrapper,FieldsType,MethodsType>info;
	consteval Class(ObjectTypeWrapper wrapper,FieldsType fields_info,MethodsType methods_info):
		info{std::make_tuple(wrapper,fields_info,methods_info)}{}
	consteval auto type()const->ObjectTypeWrapper::type;
	consteval auto get_name()const
	{
		using ConstRefWrappedType=decltype(std::get<0>(info));
		using ConstWrappedType=typename std::remove_reference<ConstRefWrappedType>::type;
		using WrappedType=typename std::remove_const<ConstWrappedType>::type;
		using Object=WrappedType::type;
		return std::string_view{get_type_name<Object>().str};
	}
	consteval auto get_fields()const
	{
		return std::get<1>(info);
	}
	consteval auto get_methods()const
	{
		return std::get<2>(info);
	}
	template<typename...Args>
	constexpr auto get_instance(const Args&...args)const
	{
		using ConstRefWrappedType=decltype(std::get<0>(info));
		using ConstWrappedType=typename std::remove_reference<ConstRefWrappedType>::type;
		using WrappedType=typename std::remove_const<ConstWrappedType>::type;
		using Object=WrappedType::type;
		return Object{args...};
	}
};
template<typename Object>
struct Reflection
{
	template<typename FieldsType,typename MethodsType>
	static consteval auto regist_class(FieldsType fields_info,MethodsType methods_info)
	{
		return Class{TypeWrapper<Object>{},fields_info,methods_info};
	}
	template<typename Field> //判断是不是成员函数，如果不是的话，那就是成员变量
	struct IsClassMethod
	{
		template<typename ReturnType,typename ObjectType,typename...Args>
		static consteval int match(ReturnType(ObjectType::*)(Args...));
		template<typename F>
		static consteval auto check(int)->decltype(match(std::declval<F>()),std::true_type());
		template<typename F>
		static consteval auto check(...)->std::false_type;
		static constexpr auto value=std::is_same<decltype(check<Field>(0)),std::true_type>::value;
	};
	static consteval auto regist_field(auto...field)
	{
		constexpr auto field_type_func=[]<typename Type>(Type Object::*)->Type{};
		return Fields{std::make_tuple(
			Field{std::make_tuple(
				get_type_name<decltype(field_type_func(field.first))>(),
				field.second,                
				get_type_name_hash<decltype(field_type_func(field.first))>(),                                              
				[=](const Object&node)->const auto{return node.*(field.first);},                
				[=](Object&node,const auto&value){return node.*(field.first)=value;},
				TypeWrapper<decltype(field_type_func(field.first))>{}
				)}...
			)};
	}
	static consteval auto regist_method(auto...method)
	{
		return Methods{std::make_tuple(
			Method{std::make_tuple(
				get_type_name<decltype(method.first)>(),                                             //方法的类型名称
				method.second,                                                                       //方法名称
				get_type_name_hash<decltype(method.first)>(),                                        //方法名称的哈希值
				[=]<typename ReturnType,typename...Args>(ReturnType(Object::*method)(Args...))   //调用方法的函数指针
				{
					return [=](Object&node,const Args&...args){return (node.*method)(args...);};
				}(method.first),
				[=]<typename ReturnType,typename...Args>(ReturnType(Object::*method)(Args...))   //调用方法的函数指针
				{
					return std::make_pair(
						TypeWrapper<ReturnType>{},
						std::make_tuple(TypeWrapper<Args>{}...)
					);
				}(method.first)
			)}...
		)};
	}
	template<std::size_t delimiter=0>
	static consteval auto reflect(auto&&...infos)
	{
		using type=decltype(std::get<0>(std::get<delimiter>(std::make_tuple(infos...))));
		constexpr auto num_elements=std::tuple_size<decltype(std::make_tuple(infos...))>::value;
		if constexpr(IsClassMethod<type>::value||delimiter>=num_elements)
		{
			auto metadata=std::make_tuple(infos...);
			auto fields_info=get_field_info(metadata,std::make_index_sequence<delimiter>());
			auto methods_info=get_method_info<IntegerWrapper<delimiter>>(metadata,std::make_index_sequence<num_elements-delimiter>());
			return Class{TypeWrapper<Object>{},fields_info,methods_info};
		}
		else
		{
			return reflect<delimiter+1>(infos...);
		}
	}
	template<std::size_t...index>
	static consteval auto get_field_info(auto&&metadata,std::integer_sequence<std::size_t,index...>)
	{
		return regist_field(std::get<index>(metadata)...);
	}
	template<typename Offset,std::size_t...index>
	static consteval auto get_method_info(auto&&metadata,std::integer_sequence<std::size_t,index...>)
	{
		return regist_method(std::get<index+Offset::value>(metadata)...);
	}
};
#endif
