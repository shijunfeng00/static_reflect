#ifndef __STATIC_REFLECT_H__
#define __STATIC_REFLECT_H__
#include<tuple>
#include<string_view>
#include<bits/refwrap.h>
#include"type_traits.h"
namespace refl
{
	using type_traits::type_wrapper;          
	using type_traits::integer_wrapper;
	template<typename Object>
	consteval auto static_reflect()
	{
		return Object::get_config();
	}
	template<char...args>
	struct static_string //编译期静态字符串
	{
		static constexpr const char str[]={args...};
		operator const char*()const{return static_string::str;}
		static constexpr std::size_t length=std::tuple_size<decltype(std::make_tuple(args...))>::value;
		template<char...args2>
		consteval auto operator+(static_string<args2...>s2)
		{
			return static_string<args...,args2...>();
		}
		template<char...args2>
		consteval auto operator+(const static_string<args2...>s2)const
		{
			return static_string<args...,args2...>();
		}
	};
/*
#define string_view_to_static_string(str)\
	[&]<std::size_t...index__>(std::integer_sequence<std::size_t,index__...>) \
	{\
		return static_string<str[index__]...>();\
	}(std::make_index_sequence<str.size()>());
*/
	/**
	 * 编译期求static_string字符串的子串
	 */
	template<int begin_pos,int end_pos>
	consteval auto slice(auto&&str)
	{
		using str_type=std::remove_reference_t<decltype(str)>;
		if constexpr(end_pos<0) /*模仿python，负数代表倒数的下表*/
		{
			return decltype(slice<begin_pos,str_type::length+end_pos>(std::declval<str_type>()))();
		}
		else
		{
			constexpr auto static_string_slice=[]<char...args>(static_string<args...>&&)
			{  /*真正的slice逻辑在这里*/
				constexpr std::size_t begin=begin_pos;
				constexpr std::size_t end=end_pos;
				if constexpr(end>=static_string<args...>::length)
				{
					static_assert(end>=static_string<args...>::length,"Index out of range.");
				}
				else if constexpr(end<=begin) /*s[4:3]*/
				{
					return static_string<>();
				}
				else
				{
					constexpr auto length=end-begin;
					constexpr auto indices=std::make_index_sequence<length>();
					constexpr auto slice_func=[]<size_t...index>(const std::integer_sequence<std::size_t,index...>&)
					{
						constexpr auto str=static_string<args...>::str;
						return static_string<str[begin+index]...>();
					}(indices);
					constexpr auto slice_str=decltype(slice_func)();
					return slice_str;
				}
			};
			return decltype(static_string_slice(std::declval<str_type>()))();
		}
	}
	template<typename T,T...ch>
	consteval auto operator""_ss()
	{
		return static_string<ch...,'\0'>();
	}
	template<typename T,T...ch>
	consteval auto operator""_ss_no_end() //没有末尾的'\0'
	{
		return static_string<ch...>();
	}
	/**
	 * 编译期获取任意类型名称
	 */
	template<typename Object>
	constexpr auto get_type_name() 
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
	/**
	 * 编译期获取任意类型名称对应的哈希值
	 */
	template<typename Object>
	constexpr auto get_type_name_hash()  
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
		return integer_wrapper<hash_value>();
	}
	/********************************************************************************/
	/**
	 * 记录一个属性的编译期反射信息
	 * FieldTypeName:属性的类型名称
	 * FieldName:属性名称
	 * FieldNameHash:属性类型名称的哈希(deprecated)
	 * GetFunc:getter,获得属性值的Lambda函数的类型
	 * SetFunc:setter,设置属性的Lambda函数的类型
	 * FieldType:属性的类型
	 */
	template<typename FieldTypeName,typename FieldName,typename FieldNameHash,typename GetFunc,typename SetFunc,typename FieldType>
	struct Field
	{
		
		const std::tuple<FieldTypeName,FieldName,FieldNameHash,GetFunc,SetFunc,FieldType>info;
		consteval Field(std::tuple<FieldTypeName,FieldName,FieldNameHash,GetFunc,SetFunc,FieldType>info):info{info}{}
		/**
		 * 属性的类型，FieldType为type_wrapper<属性名称>，因此通过::type获得真实的类型信息
		 */
		consteval auto get_type_name()const
		{
			return std::string_view{std::get<0>(info).str};
		}
		consteval auto get_name()const
		{
			return std::string_view{std::get<1>(info).str};
		}
		[[deprecated]]
		consteval auto get_type_id()const
		{
			return std::get<2>(info).value;
		}
		/**
		 * 传入一个对象，得到他这个属性的值
		 */
		template<typename Object>
		constexpr auto get_value(Object&object)const
		{
			return std::get<3>(info)(object);
		}
		/**
		 * 传入一个对象，修改他这个属性的值
		 */
		template<typename Object>
		constexpr void set_value(Object&object,const auto&value)const
		{
			std::get<4>(info)(object,value);
		}
		using type=typename FieldType::type;
		
		constexpr static auto name_v=std::string_view{
			std::remove_reference_t<decltype(std::get<1>(std::declval<Field>().info))>::str
		};
		
	};
	/**
	 * 记录一个类所有属性的编译器反射信息，如果该类为派生类，可能还会包括它的父类属性信息
	 * FieldType:一个记录多个Field的tuple:std::tuple<Field<auto>...>
	 * fields是对其的封装
	 */
	template<typename FieldType>
	struct Fields
	{
		const FieldType info;
		consteval Fields(FieldType info):info{info}{}
		/**
		 * @brief  属性的数量
		 * 
		 * @return 
		 */
		consteval auto size()const
		{
			return std::tuple_size<decltype(info)>::value;
		}
		/**
		 * 根据属性的名称获得记录该属性的反射信息的编译期对象
		 * 编译期进行for循环匹配 
		 */
		template<typename FieldName,std::size_t index=0>
		consteval auto get_field(FieldName field_name)const
		{
			if constexpr(index>=std::tuple_size<decltype(info)>::value)     //编译期遍历所有属性，找到符合条件的那个属性
			{                                                               
				static_assert(index<std::tuple_size<decltype(info)>::value,"No such field.");
			}
			else
			{
				using searched_field_name_type=std::add_lvalue_reference_t<std::add_const_t<decltype(field_name)>>;
				using field_name_type=decltype(std::get<1>(std::get<index>(info).info));
				if constexpr(std::is_same<searched_field_name_type,field_name_type>::value)
				{
					return std::get<index>(info);
				}
				else
				{
					return get_field<FieldName,index+1>(field_name);
				}
			}
		}
        /**
		 * 编译期查询类中是否又该属性存在，其实也是编译期for循环匹配名字是否存在
		 */
		template<typename FieldName,std::size_t index=0>
		consteval auto has_field(FieldName field_name)const
		{
			if constexpr(index>=std::tuple_size<decltype(info)>::value) //遍历完一遍后都没找到
			{
				return std::false_type();
			}
			else
			{
				using searched_field_name_type=std::add_lvalue_reference_t<std::add_const_t<decltype(field_name)>>;
				using field_name_type=decltype(std::get<1>(std::get<index>(info).info));
				if constexpr(std::is_same<searched_field_name_type,field_name_type>::value)
				{
					return std::true_type();
				}
				else
				{
					return has_field<FieldName,index+1>(field_name);
				}
			}
		}
		template<std::size_t ith=0>
		constexpr auto for_each(auto&&callback)const
		{
			constexpr auto size=std::tuple_size<decltype(info)>::value;

			if constexpr(ith<size)
			{
				callback(ith,std::get<ith>(info));
				for_each<ith+1>(callback);
			}
		}
		/**
		 * 类型萃取
		 */
		template<std::size_t ith>
		using field_t=std::remove_reference_t<decltype(std::get<ith>(info))>;
		template<typename StaticString>
		using field_ss_t=std::remove_reference_t<decltype(std::declval<Fields>().get_field(StaticString()))>;
		template<typename StaticString>
		static constexpr auto has_field_v=std::is_same_v<
			std::true_type,
			decltype(std::declval<Fields>().has_field(StaticString()))
		>;
		static constexpr auto size_v=std::tuple_size_v<decltype(std::declval<Fields>().info)>;
	};
	/**
	 * 记录一个方法的编译期反射信息
	 * MethodTypeName:方法的类型名称
	 * MethodName:方法名称
	 * MethodNameHash:方法类型名称的哈希(deprecated)
	 * Function:对于类方法的包装，调用它等同于调用属性本身func1(a,b,c)->func2(对象,a,b,c)
	 * Parmas:方法的返回值类型和各个参数的类型tuple<返回值类型,tuple<type_wrapper<形参类型>...>>
	 */
	template<typename MethodTypeName,typename MethodName,typename MethodNameHash,typename Function,typename Parmas>
	struct Method
	{
		template<typename T>
		using remove_const_ref=typename std::remove_const<typename std::remove_reference<T>::type>::type;
		template<typename T>
		using remove_rvalue_ref=typename std::remove_reference<typename std::remove_reference<T>::type>::type;
		
		const std::tuple<MethodTypeName,MethodName,MethodNameHash,Function,Parmas>info;

		consteval Method(std::tuple<MethodTypeName,MethodName,MethodNameHash,Function,Parmas>info):info{info}{}
		/**
		 * 得到每个形参的类型
		 */
		template<std::size_t i>
		using parameter_t=remove_rvalue_ref<
		decltype(std::get<i>(std::declval<
			remove_const_ref<
			decltype(std::get<1>(std::get<4>(info)))
			>
			>()))
		>::type;
		/**
		 * 得到返回值的类型
		 */
		using return_t=remove_const_ref<
		decltype(std::get<0>(std::get<4>(info)))
		>::type;
		static constexpr auto name_v= std::string_view{std::remove_const_t<
			std::remove_reference_t<decltype(std::get<1>(std::declval<Method>().info))>
		>::str};
		
		/**
		 * 获得形参的数量
		 */
		static constexpr auto size_v=std::tuple_size<Parmas>::value;
		
		consteval auto get_type_name()const
		{
			return std::string_view{std::get<0>(info).str};
		}
		consteval auto get_name()const
		{
			return std::string_view{std::get<1>(info).str};
		}
		[[deprecated]]
		consteval auto get_type_id()const
		{
			return std::get<2>(info).value;
		}
		consteval auto get_return_type_name()const
		{
			return std::string_view{refl::get_type_name<return_t>().str};
		}
		template<std::size_t index>
		consteval auto get_args_type_name()const
		{
			return std::string_view{refl::get_type_name<parameter_t<index>>().str};
		}
		/**
		 * @brief 得到由形参类型的字符串名称组成的数组(std::array<string_view>,同样也是编译期构建的
		 * 
		 * @return 
		 */
		consteval auto get_args_type_name_list()const
		{
			using types=remove_const_ref<decltype(std::get<1>(std::get<4>(info)))>;
			constexpr auto num_args=std::tuple_size<types>::value;

			return []<std::size_t...i>(std::integer_sequence<std::size_t,i...>)
			{
				return std::array{
					std::string_view{refl::get_type_name<
						typename remove_rvalue_ref<
						decltype(std::get<i>(std::declval<types>()))
						>::type
						>().str}...
				};
			}(std::make_index_sequence<num_args>());
		}
		/**
		 * 通过反射调用属性
		 */
		template<typename ObjectType,typename...Args>
		auto invoke(ObjectType&node,const Args&...args)const
		{
			return std::get<3>(info)(node,args...);
		}
		/**
		 * 如果是constexpr对象
		 * 而且方法也是constexpr(consteval)
		 * 那么调用方法的返回值也是可以在编译期计算出
		 * 这个时候采用invoke_constexpr将该计算也放在编译期进行
		 */
		template<typename ObjectType,typename...Args>
		consteval auto constexpr_invoke(ObjectType node,const Args&...args)const
		{
			return std::get<3>(info)(node,args...);
		}
		
	};
	/**
	 * 记录一个类中所有方法的反射信息的类
	 * MethodType:为一个std::tuple,包含若干Method<>,也就是std::tuple<Method<auto>...>
	 */
	template<typename MethodType>
	struct Methods
	{
		const MethodType info;
		consteval Methods(MethodType info):info{info}{}
		/**
		 * @brief 得到方法的数量
		 * 
		 * @return 
		 */
		consteval auto size()const
		{
			return std::tuple_size<decltype(info)>::value;
		}
		/**
		 * 通过字符串名称来获得对应的记录方法的反射信息对象
		 */
		template<typename MethodName,std::size_t index=0>
		consteval auto get_method(MethodName field_name)const
		{
			if constexpr(index>=std::tuple_size<decltype(info)>::value)
			{
				static_assert(index<std::tuple_size<decltype(info)>::value,"No such method.");
			}
			else
			{
				using searched_field_name_type=std::add_lvalue_reference_t<std::add_const_t<decltype(field_name)>>;
				using field_name_type=decltype(std::get<1>(std::get<index>(info).info));
				if constexpr(std::is_same<searched_field_name_type,field_name_type>::value)
				{
					return std::get<index>(info);
				}
				else
				{
					return get_method<MethodName,index+1>(field_name);
				}
			}
		}
		/**
		 * 查询某一个名称为method_name的方法是否存在
		 */
		template<typename MethodName,std::size_t index=0>
		consteval auto has_method(MethodName method_name)const
		{
			if constexpr(index>=std::tuple_size<decltype(info)>::value)
			{
				return std::false_type();
			}
			else
			{
				using searched_method_name_type=std::add_lvalue_reference_t<std::add_const_t<decltype(method_name)>>;
				using method_name_type=decltype(std::get<1>(std::get<index>(info).info));
				if constexpr(std::is_same<searched_method_name_type,method_name_type>::value)
				{
					return std::true_type();
				}
				else
				{
					return has_method<MethodName,index+1>(method_name);
				}
			}
		}
		template<std::size_t ith=0>
		constexpr auto for_each(auto&&callback)const
		{
			constexpr auto size=std::tuple_size<decltype(info)>::value;

			if constexpr(ith<size)
			{
				callback(ith,std::get<ith>(info));
				for_each<ith+1>(callback);
			}
		}
		/**
		 * 没有对应提供method_t是因为我暂时通不过编译
		 */
		template<typename StaticString>
		using method_ss_t=decltype(std::declval<Methods>().get_method(StaticString()));
		template<typename StaticString>
		static constexpr auto has_method_v=std::is_same_v<std::true_type,
			decltype( std::declval<Methods>().has_method(StaticString()) )
		>;
		static constexpr auto size_v=std::tuple_size<decltype(std::declval<Methods>().info)>::value;
	};
	/**
	 * 记录一个class的所有编译期反射信息的类
	 * ObjectWrapper:对象的类型包装,type_wrapper<Object>
	 * FieldsType:Fields<auto>,记录属性编译期反射信息的类
	 * MethodsType:Methods<auto>,记录方法编译期反射的类
	 * ParentClass:父类，C++允许多继承，但是目前只实现了单继承
	 */
	template<typename ObjectWrapper,typename FieldsType,typename MethodsType,typename...ParentClass>
	struct Class
	{
		std::tuple<ObjectWrapper,FieldsType,MethodsType>info;
		consteval Class(ObjectWrapper wrapper,FieldsType fields_info,MethodsType methods_info):info{
			std::make_tuple(wrapper,fields_info,methods_info)
		}{}
		consteval auto type()const->ObjectWrapper::type;

		consteval auto get_name()const
		{
			using Object=type_traits::remove_const_refertnce_t<decltype(std::get<0>(info))>::type;
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
			using Object=type_traits::remove_const_refertnce_t<decltype(std::get<0>(info))>::type;
			return Object{args...};
		}
		
		template<std::size_t ith>
		using parent_t=std::remove_reference_t<decltype(
			std::get<ith>(std::declval<std::tuple<ParentClass...>>())
		)>;		//父类类型，通过下标遍历每一个
		using parents_t=std::tuple<ParentClass...>;   //父类类型，所有父类类型组合为一个Tuple的类型列表
		using fields_t=std::remove_reference_t<decltype(std::get<1>(info))>;   //类型萃取，记录属性反射信息的类
		using methods_t=std::remove_reference_t<decltype(std::get<2>(info))>;  //记录方法反射信息的类	
	};
	/**
	 * 主要用于在编译期注册反射信息，使得编译期可以获得类的反射信息
	 */
	template<typename Object=void>
	struct Reflection
	{
		/**
		 * 用于派生类的反射注册，可以记录继承关系SubClassInfo为子类继承信息
		 * 将父类反射信息整合进子类中
		 */
		template<typename FatherClass>
		struct Inherit
		{
			template<typename SubClassFields,typename SubClassMedhods>
			static consteval auto regist_class(SubClassFields&&fields,SubClassMedhods&&methods)
			{
				auto sub_cls_fs_info=fields.info;
				auto sub_cls_ms_info=methods.info;
				auto fa_cls_fs_info=FatherClass::get_config().get_fields().info;
				auto fa_cls_ms_info=FatherClass::get_config().get_methods().info;
				auto fields_info=Fields{std::tuple_cat(fa_cls_fs_info,sub_cls_fs_info)};
				auto methods_info=Methods{std::tuple_cat(fa_cls_ms_info,sub_cls_ms_info)};
				return Class<
					type_wrapper<Object>,
					decltype(fields_info),
					decltype(methods_info),
					FatherClass
				>{type_wrapper<Object>{},fields_info,methods_info};
			}
		};
		template<typename FieldsType,typename MethodsType>
		static consteval auto regist_class(FieldsType fields_info,MethodsType methods_info)
		{
			return Class{type_wrapper<Object>{},fields_info,methods_info};
		}
		static consteval auto regist_field(auto...field)
		{
			return Fields{std::make_tuple(
				Field{std::make_tuple(
					get_type_name<type_traits::remove_class_t<decltype(field.first)>>(), //属性的类型名称
					field.second,                                                        //属性名称
					get_type_name_hash<type_traits::remove_class_t<decltype(field.first)>>(),//属性类型名的哈希
					[=](const Object&node)->const auto{return std::ref(node.*(field.first));}, //getter
					[=](Object&node,const auto&value){node.*(field.first)=value;}, //setter
					type_wrapper<type_traits::remove_class_t<decltype(field.first)>>{} 
					)}...  //之所以type_wrapper包装一下，是因为可能遇到原生数组int[N]的问题，函数没法返回一个数组
				)};
		}
		static consteval auto regist_method(auto...method)
		{
			return Methods{std::make_tuple(
				Method{std::make_tuple(
					get_type_name<decltype(method.first)>(),                                             //方法的类型名称
					method.second,                                                                       //方法名称
					get_type_name_hash<decltype(method.first)>(),                                        //方法名称的哈希值
					[=]<typename ReturnType,typename...Args>(ReturnType(Object::*method)(Args...))       //调用方法的函数指针
					{
						return [=](Object&node,const Args&...args){return (node.*method)(args...);};
					}(method.first),
					[=]<typename ReturnType,typename...Args>(ReturnType(Object::*method)(Args...))       //用于获取各个形参类型
					{
						return std::make_pair(
							type_wrapper<ReturnType>{},
							std::make_tuple(type_wrapper<Args>{}...)
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
			if constexpr(type_traits::is_class_method_v<type>||delimiter>=num_elements)
			{
				auto info=std::make_tuple(infos...);
				auto fields_info=get_field_info(info,std::make_index_sequence<delimiter>());
				auto methods_info=get_method_info<integer_wrapper<delimiter>>(info,std::make_index_sequence<num_elements-delimiter>());
				return Class{type_wrapper<Object>{},fields_info,methods_info};
			}
			else
			{
				return reflect<delimiter+1>(infos...);
			}
		}
		
		template<std::size_t...index>
		static consteval auto get_field_info(auto&&info,std::integer_sequence<std::size_t,index...>)
		{
			return regist_field(std::get<index>(info)...);
		}
		template<typename Offset,std::size_t...index>
		static consteval auto get_method_info(auto&&info,std::integer_sequence<std::size_t,index...>)
		{
			if constexpr(std::tuple_size<decltype(info)>::value==0)
			{
				return regist_method();
			}
			else
			{
				return regist_method(std::get<index+Offset::value>(info)...);
			}
		}
	};

	template<typename T,std::size_t stage=0,std::size_t index=0>
	consteval auto describe()
	{
		if constexpr(stage==0)
		{
			using parents=decltype(refl::static_reflect<T>())::parents_t;//父类,可能有继承关系
			constexpr auto parents_name="\nparents class:\t"_ss_no_end+slice<10,-1>(get_type_name<parents>());
			constexpr auto class_name="Class Name:\t"_ss_no_end+slice<0,-1>(get_type_name<T>());
			constexpr auto size="\nsize:\t\t"_ss_no_end+slice<29,-2>(get_type_name<integer_wrapper<sizeof(T)>>())+" Bytes"_ss_no_end;
			constexpr auto cls_result=class_name+parents_name+size+"\n"_ss_no_end;			
			constexpr auto res=cls_result+"Fields:\n"_ss_no_end+describe<T,1,0>();
			return res;
		}
		else if constexpr(stage==1)
		{
			constexpr auto size=static_reflect<T>().get_fields().size();
			if constexpr(index<size)
			{
				constexpr auto fields=static_reflect<T>().get_fields();
				constexpr auto field=std::get<index>(fields.info);
				constexpr auto name="name:\t"_ss_no_end+slice<0,-1>(
					std::remove_const_t<std::remove_reference_t<
					decltype(
						std::get<1>(field.info)
						)
					>>());
				constexpr auto type="\ttype:\t"_ss_no_end+slice<0,-1>(
					std::remove_const_t<
					std::remove_reference_t<
					decltype(
						std::get<0>(field.info)
						)>>()
					);
				return name+type+"\n"_ss_no_end+describe<T,1,index+1>();
			}
			else
			{
				return "Methods:\n"_ss_no_end+describe<T,2,0>();
			}
		}
		else if constexpr(stage==2)
		{
			constexpr auto size=static_reflect<T>().get_methods().size();
			if constexpr(index<size)
			{
				constexpr auto methods=static_reflect<T>().get_methods();
				constexpr auto method=std::get<index>(methods.info);
				constexpr auto name="name:\t"_ss_no_end+slice<0,-1>(
					std::remove_const_t<
					std::remove_reference_t<
					decltype(
						std::get<1>(method.info))>>()
					);
				constexpr auto type="\ttype:\t"_ss_no_end+slice<0,-1>(
					std::remove_const_t<
					std::remove_reference_t<
					decltype(
						std::get<0>(method.info))>>()
					);
				return name+type+"\n"_ss_no_end+describe<T,2,index+1>();;
			}
			else
			{
				return ""_ss;
			}
		}
	}
	
	template<typename Object>
	using get_type_name_t=decltype(refl::get_type_name<Object>());
	template<typename Object>
	constexpr auto get_type_name_v=refl::get_type_name<Object>();
	template<typename Object>
	constexpr auto get_type_name_view=std::string_view{refl::get_type_name<Object>().str};
	template<typename Object>
	constexpr auto static_reflect_v=static_reflect<Object>();
	template<typename Object>
	using static_reflect_t=std::remove_reference_t<decltype(static_reflect<Object>())>;
	
}
using refl::operator""_ss;
using refl::operator""_ss_no_end;
#endif
