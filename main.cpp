#include"static_reflect.h"
#include<cstdio>
using namespace std;
struct Node
{
	constexpr Node(){}
	constexpr Node(int x,float y):x{x},y{y}{} //不过构造函数还是得保持public...
private:
	int x=3;
	float y=2;
	constexpr int add(int dx,float dy)
	{
		return x+dx+y+dy;
	}
	constexpr int mul(float dx)
	{
		return x*dx*y;
	}
public:
	static consteval auto get_config() //注册反射所需的meta data
	{
		/*
		return Reflection<Node>::reflect(
			make_pair(&Node::x,"x"_ss),
			make_pair(&Node::y,"y"_ss),
			make_pair(&Node::add,"add"_ss),
			make_pair(&Node::mul,"mul"_ss)
		);
		*/
		return Reflection<Node>::regist_class(
			Reflection<Node>::regist_field(
				make_pair(&Node::x,"x"_ss),
				make_pair(&Node::y,"y"_ss)
			),
			Reflection<Node>::regist_method(
				make_pair(&Node::add,"add"_ss),
				make_pair(&Node::mul,"mul"_ss)
			)
		);
	}
};
int main()
{
	
	constexpr auto refl_info=static_reflect(Node);                   //得到类的反射信息
	constexpr auto methods=refl_info.get_methods();                  //得到类的所有方法的tuple
	constexpr auto method=methods.get_method("add"_ss);              //得到名称为"add"的方法
	constexpr auto fields=refl_info.get_fields();                    //得到类的所有属性的tuple
	constexpr auto field=fields.get_field("x"_ss);                   //得到名称为"x"的方法
	
	constexpr auto node=refl_info.get_instance(2,3.f);               //实例化一个对象,调用对应的构造函数
	
	constexpr auto method_name=method.get_name();                    //获取方法名称
	constexpr auto method_type=method.get_type_name();               //获取方法类型名称
	constexpr auto method_type_id=method.get_type_id();              //获取方法类型名称的哈希值
	constexpr auto method_value=method.constexpr_invoke(node,2,3.f); //调用方法
	
	constexpr auto args_types=method.get_args_type_name();           //得到该方法的参数类型名称列表
	constexpr auto return_type=method.get_return_type_name();       //得到该方法的返回值类型名称
	
	constexpr auto field_name=field.get_name();                      //得到该属性的名称
	constexpr auto field_type=field.get_type_name();                 //得到该属性的类型名称
	constexpr auto value=field.get_value(node);                      //得到该属性的值
	//field.set_value(node,2333);                                    //设置该属性的值,constexpr对象不可用.
	
	static_assert(std::is_same<decltype(refl_info.type()),Node>::value);
	static_assert(refl_info.get_name()=="Node");
	static_assert(methods.size()==2);                                         //成员函数的数量
	static_assert(fields.size()==2);                                          //成员变量的数量
	

	static_assert(std::is_same<decltype(method.return_type()),int>::value);   //得到成员函数的返回值类型(非字符串)
	static_assert(std::is_same<decltype(method.arg_type<0>()),int>::value);   //得到成员函数第0个参数类型
	static_assert(std::is_same<decltype(method.arg_type<1>()),float>::value); //得到成员函数第1个参数类型
	static_assert(args_types.size()==2);                                      //该成员函数参数个数
	static_assert(method.constexpr_invoke(node,2,3.f)==10);                   //调用构造函数构造编译期静态对象
	static_assert(method_name=="add");
	static_assert(method_type=="int (Node::*)(int, float)");
	static_assert(method_type_id==4425629840105553482ll);
	static_assert(method_value==10);
	static_assert(args_types[0]=="int");
	static_assert(args_types[1]=="float");
	static_assert(return_type=="int");
	
	static_assert(std::is_same<decltype(field.type()),int>::value);	
	static_assert(field_name=="x");
	static_assert(field_type=="int");
	static_assert(value==2);
	
	/*
	constexpr auto refl_info=static_reflect(Node);                   //得到类的反射信息
	constexpr auto fields=refl_info.get_fields();                    //得到类的所有属性的tuple
	constexpr auto field=fields.get_field("x"_ss);                   //得到名称为"x"的方法
	constexpr auto methods=refl_info.get_methods();                  //得到类的所有方法的tuple
	constexpr auto method=methods.get_method("add"_ss);              //得到名称为"add"的方法
	Node node=refl_info.get_instance(2,3.f);                         //实例化一个对象,调用对应的构造函数
	volatile int x=2333;
	field.set_value(node,x);                                         //修改值
	printf("%d,",field.get_value(node));                            //得到值
	printf("%d\n",method.invoke(node,2,3.f));
	
	constexpr auto args_types=method.get_args_type_name();           //得到该方法的参数类型名称列表
	constexpr auto return_type=method.get_return_type_name();       //得到该方法的返回值类型名称
	static_assert(std::is_same<decltype(method.return_type()),int>::value);   //得到成员函数的返回值类型(非字符串)
	static_assert(std::is_same<decltype(method.arg_type<0>()),int>::value);   //得到成员函数第0个参数类型
	static_assert(std::is_same<decltype(method.arg_type<1>()),float>::value); //得到成员函数第1个参数类型
	static_assert(args_types.size()==2);                                      //该成员函数参数个数
	static_assert(method.get_name()=="add");
	static_assert(method.get_type_name()=="int (Node::*)(int, float)");
	static_assert(method.get_type_id()==4425629840105553482ll);
	static_assert(args_types[0]=="int");
	static_assert(args_types[1]=="float");
	static_assert(return_type=="int");
	*/
}
