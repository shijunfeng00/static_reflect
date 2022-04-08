#ifndef __REFLECTABLE_WRAPPER_H__
#define __REFLECTABLE_WRAPPER_H__
#include"static_reflect.h"
/**
 * һ�������л�����İ�װ�࣬���һ��Ԫ�ز���class�����Ǳ���std::tuple,����int[5]
 * �����ṩ���л���֧��
 * �ͽ������װΪһ��Reflectable�����Ȼ��������л����ٰ�value�ֶν�ȡ����
 */
template<typename T>
struct reflectable_wrapper
{
	const T value;
	reflectable_wrapper(const T&value):value{value}{}
	static consteval auto get_config()
	{
		return refl::Reflection<reflectable_wrapper>::regist_class(
			refl::Reflection<reflectable_wrapper>::regist_field(
				std::pair{&reflectable_wrapper<T>::value,"value"_ss}
				),
			refl::Reflection<reflectable_wrapper>::regist_method(
				)
			);
	}
};
#endif
