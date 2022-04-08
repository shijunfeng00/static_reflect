#ifndef __REFLECTABLE_WRAPPER_H__
#define __REFLECTABLE_WRAPPER_H__
#include"static_reflect.h"
/**
 * 一个可序列化的类的包装类，如果一个元素不是class，而是比如std::tuple,或者int[5]
 * 对其提供序列化的支持
 * 就将将其包装为一个Reflectable的类别，然后进行序列化，再把value字段截取出来
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
