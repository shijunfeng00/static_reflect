#ifndef __STATIC_SERIALIZET_H__
#define __STATICSERIALIZE_H__
#include"static_reflect.h"
#include"reflectable_wrapper.h"
#include"type_traits.h"
#include<sstream>
#include<string_view>
#include<bits/refwrap.h>
namespace seri
{
	using namespace refl;
	using namespace type_traits;
	/**
	 * ����tuple��������ÿһ��Ԫ�ش���callback�����д���
	 * callback��������:
	 * void callback(auto&&index,auto element); index�����±�elementΪtuple��index��Ԫ��
	 */
	template<size_t idx=0>
	void for_each(auto&&tp,auto&&callback)
	{
		constexpr auto size=std::tuple_size<std::remove_reference_t<std::remove_const_t<decltype(tp)>>>::value;
		if constexpr(idx<size)
		{
			callback(idx,std::get<idx>(tp));
			for_each<idx+1>(tp,callback);
		}
	}
	/**
	 * �õ�JSON�ַ����ĸ�ʽ�ַ�,����{"x":{},"y":{}}
	 * Ȼ�����������ϱ���std::format��Runtime��䣬���������
	 * ����GCC��ʱ��֧�֣������Ҳ�����std::ostreamstream������JSON�ַ�����������dumps����
	 * �����һ�������л����࣬���ᱻ���л�Ϊ{"field_name1":{},"field_name2":{}}
	 * ���������std::tuple,std::pair,�ᱻ���Ϊ[{},{},{}]����ʽ����Ϊ������������֪�ģ����Ա����ڴ���
	 * std::arrayҲ�ᱻ��ʾΪ[1,2,3,4]����ʽ�����ǲ��ᱻ��⣬ֻ��дΪռλ��{},��һ������std::array<int,10000>,��������ը��
	 */
	template<typename T,std::size_t ith=0>
	consteval auto get_json_format_no_end()
	{
		constexpr auto get_size=[]() //T�����Ϳ����ǿ����л�����,Ҳ������tuple/pair,���������Ҫ�������ж�
		{
			if constexpr(is_reflectable_class_v<T>)
			{
				return std::tuple_size<decltype(static_reflect<T>().get_fields().info)>::value;
			}
			else if constexpr(is_tuple_or_pair_v<T>)
			{
				return std::tuple_size<T>::value;
			}
			else
			{
				static_assert(!(is_tuple_or_pair_v<T>||is_reflectable_class_v<T>),"not supported type.");
			}
		};
		constexpr auto size=get_size();
		if constexpr(ith<size)                //���ڵ�ith������,�������л�
		{
			if constexpr(is_tuple_or_pair_v<T>) //T��tuple or pair
			{
				using element_type=decltype(std::get<ith>(std::declval<T>()));
				using value=decltype(
					[](){
						if constexpr(is_reflectable_class_v<element_type>) //��Ԫ��������һ�����Է���Ķ���
						{
							return get_json_format_no_end<std::remove_reference_t<element_type>,0>();
						}
						else if constexpr(is_tuple_or_pair_v<element_type>)
						{
							return get_json_format_no_end<element_type,0>(); //��������һ��tuple����pair
						}
						else
						{
							return static_string<'{','}'>();
						}
					}()
					);
				if constexpr(ith==0)//����[
				{
					return static_string<'['>()+value()+static_string<','>()+get_json_format_no_end<T,ith+1>();
				}
				else if constexpr(ith==size-1) //����]
				{
					return value()+static_string<']'>();
				}
				else
				{
					return value()+static_string<','>()+get_json_format_no_end<T,ith+1>();//���ϵ�ǰ����������л���Ľṹ
				}

			}
			else //T��Reflectable����
			{
				constexpr auto refl_info=static_reflect<T>();

				using refl_info_type=remove_const_refertnce_t<decltype( //��õ�ith�����Զ�Ӧ��������Ϣ
					std::get<1>(
						std::get<ith>(
							std::declval<decltype(refl_info)>().get_fields().info
							).info
						)
					)>;
				using field_type=decltype(
					std::declval<decltype(refl_info)>().get_fields().get_field(refl_info_type())
					)::type;

				using field_name=decltype(//ȥ��ĩβ��'\0'
					[]<size_t...index,char...args>(std::index_sequence<index...>,static_string<args...>){
						return static_string<static_string<args...>::str[index]...>();
					}(
						std::make_index_sequence<sizeof(refl_info_type::str)/sizeof(char)-1>(),
						refl_info_type()
						)
					);
				using value=decltype(
					[](){
						if constexpr(is_reflectable_class_v<field_type>) //������������һ�����Է���Ķ���
						{
							return get_json_format_no_end<field_type,0>();
						}
						else if constexpr(is_tuple_or_pair_v<field_type>)
						{
							return get_json_format_no_end<field_type,0>(); //��������һ��tuple����pair
						}
						else
						{
							return static_string<'{','}'>();
						}
					}()
					);
				constexpr auto field_pair=static_string<'\"'>()+field_name()+static_string<'\"',':'>()+value();
				if constexpr(size==1)
				{
					constexpr auto json=static_string<'{'>()+field_pair+static_string<'}'>();
					return json;
				}
				else if constexpr(ith==size-1)
				{
					constexpr auto json=field_pair+static_string<'}'>();
					return json;
				}
				else if constexpr(ith==0)
				{
					constexpr auto json=static_string<'{'>()+field_pair+static_string<','>()+get_json_format_no_end<T,ith+1>();
					return json;
				}
				else
				{
					constexpr auto json=field_pair+static_string<','>()+get_json_format_no_end<T,ith+1>();
					return json;
				}

			}
		}
		else
		{
			return static_string<>();
		}
	}
	template<typename T>
	consteval auto get_json_format()
	{
		return get_json_format_no_end<T,0>()+""_ss;
	}
	/**
	 * �õ�JSON�ַ���ÿһ��ռλ����Ӧ�ı���������Ϊһ��std::tuple��ʽ
	 * �������get_json_formatռλ��һһ��Ӧ��ϵ
	 * std::tuple,std::pair,�����л����඼�ᱻ�ֽ�ΪԪ���ж�Ӧλ�õ�Ԫ��
	 */
	template<std::size_t ith=0>
	constexpr auto get_json_params(auto&&p)
	{
		using T=std::remove_const_t<std::remove_reference_t<decltype(p)>>;

		constexpr auto get_size=[]() //T�����Ϳ����ǿ����л�����,Ҳ������tuple/pair,���������Ҫ�������ж�
		{
			if constexpr(is_reflectable_class_v<T>)
			{
				return std::tuple_size<decltype(static_reflect<T>().get_fields().info)>::value;
			}
			else if constexpr(is_tuple_or_pair_v<T>)
			{
				return std::tuple_size<T>::value;
			}
			else
			{
				static_assert(!(is_tuple_or_pair_v<T>||is_reflectable_class_v<T>),"not supported type.");
				return 0;
			}
		};
		constexpr auto size=get_size();
		if constexpr(ith<size)
		{
			if constexpr(is_reflectable_class_v<T>) //T��һ�������л����࣬��˳������л�����ÿһ������(�ھ�̬����get_config��ע���)
			{
				constexpr auto refl_info=static_reflect<T>();
				constexpr auto field=std::get<ith>(refl_info.get_fields().info);
				using field_type_name=remove_const_refertnce_t<decltype(
					std::get<1>(
						std::get<ith>(
							std::declval<decltype(refl_info)>().get_fields().info
							).info
						)
					)>;

				using field_type=decltype(
					std::declval<decltype(refl_info)>().get_fields().get_field(field_type_name())
					)::type;

				if constexpr(is_reflectable_class_v<field_type>)
				{
					const field_type&value=field.get_value(p);
					auto field_values=get_json_params(value);
					return std::tuple_cat(
						field_values,
						get_json_params<ith+1>(p)
						);
				}
				else if constexpr(is_tuple_or_pair_v<field_type>)
				{
					const field_type&value=field.get_value(p);
					auto field_values=get_json_params(value);
					return std::tuple_cat(
						field_values,
						get_json_params<ith+1>(p)
						);
				}
				else
				{
					return std::tuple_cat(
						std::make_tuple(field.get_value(p)),
						get_json_params<ith+1>(p)
						);
				}
			}
			else if(is_tuple_or_pair_v<T>)
			{
				using element_type=std::remove_reference_t<decltype(std::get<ith>(std::declval<T>()))>;
				if constexpr(is_reflectable_class_v<element_type>)
				{
					const element_type&value=std::get<ith>(p);
					auto field_values=get_json_params(value);
					return std::tuple_cat(
						field_values,
						get_json_params<ith+1>(p)
						);

				}
				else if constexpr(is_tuple_or_pair_v<element_type>)
				{
					const element_type&value=std::get<ith>(p);
					auto field_values=get_json_params(value);
					return std::tuple_cat(
						field_values,
						get_json_params<ith+1>(p)
						);

				}
				else
				{
					const element_type&value=std::get<ith>(p);
					return std::tuple_cat(
						std::make_tuple(std::ref(value)),
						get_json_params<ith+1>(p)
						);

				}

			}
		}
		else
		{
			return std::make_tuple();
		}
	}
	template<typename Object>
	constexpr auto dumps(const Object&object)
	{
		if constexpr(!is_reflectable_class_v<Object>)
			static_assert(!is_reflectable_class_v<Object>,"Not a reflectable class.");
		constexpr auto format=get_json_format_no_end<Object>();
		constexpr auto format_view=std::string_view{(format+static_string<'\0'>()).str};
		auto params=get_json_params(object);
		std::ostringstream oss;
		int begin_idx=0;
		int end_idx=0;
		auto to_string=[&]<typename T>(T&&param)
		{
			using param_type=std::remove_reference_t<decltype(param)>;

			if constexpr(std::is_pointer_v<std::remove_reference_t<param_type>>)
			{
				if(param==nullptr||param==NULL)
				{
					oss<<"null";
				}
				else
				{
					oss<<*param;
				}
			}
			else if constexpr(is_string_v<param_type>)
			{
				oss<<"\""<<param<<"\"";
			}
			else if constexpr(is_raw_array_v<param_type>)
			{
				using element_type=std::remove_reference_t<decltype(param[0])>;
				if constexpr(std::is_fundamental_v<element_type>)
				{
					oss<<"[";
					bool begin=true;
					for(auto&it:param)
					{

						if(!begin)
							oss<<",";
						begin=false;
						oss<<it;
					}
					oss<<"]";
				}
				else
				{
					oss<<"<unsupport type:"<<get_type_name<element_type>()<<">";
				}
			}
			else if constexpr(is_list_type_v<param_type>)
			{
				using element_type=std::remove_pointer_t<decltype(param.begin())>;
				if constexpr(std::is_fundamental_v<element_type>)
				{
					oss<<"[";
					bool begin=true;
					for(auto&it:param)
					{

						if(!begin)
							oss<<",";
						begin=false;
						oss<<it;
					}
					oss<<"]";
				}
				else
				{
					oss<<"<unsupport type:"<<get_type_name<element_type>()<<">";
				}
			}
			else
			{
				oss<<param;
			}
		};
		for_each(params,[&](auto&&index,auto&param)
			{
				end_idx=format_view.find("{}",begin_idx);
				if(begin_idx==0)
				{
					oss<<format_view.substr(begin_idx,end_idx-begin_idx);
				}
				else
				{
					oss<<format_view.substr(begin_idx+1,end_idx-begin_idx-1);
				}
				to_string(param);
				begin_idx=end_idx+1;
			}
			);
		oss<<format_view.substr(end_idx+2);
		return oss.str();
	}
	template<typename Object>
	constexpr auto get_json_format_v=get_json_format<Object>();
	template<typename Object>
	constexpr auto describe_v=describe<Object>();
	template<typename Object>
	using describe_t=std::remove_reference_t<decltype(describe<Object>())>;
};
#endif

