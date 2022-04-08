# static_reflect
This is a fully compiling time static reflection lightweight framework for C++.

It provides a very rich compile-time reflection function.

# environment
gcc10.3.0 & -std=c++20


# Below is a demo of all APIs.
```cpp
#include"static_reflect.h"
#include"static_serialize.h"
#include<iostream>
#include<cstdio>
#include<sstream>
#include<vector>
#include<array>
using namespace refl;
using namespace seri;
using namespace std;
struct Node
{
	constexpr Node(){}
	constexpr Node(int x,float y):x{x},y{y}{} //不过构造函数还是得保持public...
	int x=3;
	float y=2;
	string_view z="default";
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
		return Reflection<Node>::regist_class(
			Reflection<Node>::regist_field(
				pair{&Node::x,"x"_ss},
				pair{&Node::y,"y"_ss},
				pair{&Node::z,"z"_ss}
				),
			Reflection<Node>::regist_method(
				pair{&Node::add,"add"_ss},
				pair{&Node::mul,"mul"_ss}
				)
			);
	}
};
struct Test
{
	Node a;
	Node b;
	int c=2333;
	std::tuple<int,float,Node>tp;
	int*d=nullptr;
	std::array<int,5>e={1,2,3,4,5};
	int f[5]={5,4,3,2,1};
	constexpr Test()
	{
		a.x=23;
		a.y=43;
		a.z="abcd";
		d=new int(-5);
		std::get<0>(tp)=12;
		std::get<1>(tp)=24;
		std::get<2>(tp).z="sjf";
	}
	constexpr auto func(int,float,double){}
	static consteval auto get_config()
	{
		return Reflection<Test>::regist_class(
			Reflection<Test>::regist_field(
				pair{&Test::a,"a"_ss},
				pair{&Test::b,"b"_ss},				
  			    pair{&Test::c,"c"_ss},
				pair{&Test::tp,"tp"_ss},
				pair{&Test::d,"d"_ss},
				pair{&Test::e,"e"_ss},
				pair{&Test::f,"f"_ss}
				),
			Reflection<Test>::regist_method(
				pair{&Test::func,"func"_ss}
				)
			);
	}
};
struct Son:public Test
{
	string_view g="son";
	consteval static auto get_config()
	{
		return
			Reflection<Son>::Inherit<Test>::regist_class(
				Reflection<Son>::regist_field(
					pair{&Son::g,"g"_ss}
				),
				Reflection<Son>::regist_method(
					)
			);

	}
};

int main()
{

	constexpr auto reflect=static_reflect_v<Node>;  //获得记录反射信息的对象
	constexpr auto class_name=reflect.get_name();   //获得类名称
	static_assert(class_name=="Node");	
	
	constexpr auto fields=reflect.get_fields();      //所有属性的反射信息
	constexpr auto field=fields.get_field("x"_ss);   //名称为x的属性
	static_assert(fields.size()==3);                 //这个类有3个属性
	static_assert(field.get_name()=="x");            //属性名称
	static_assert(field.get_type_name()=="int");     //属性类型名称
	static_assert(fields.get_field("y"_ss).get_type_name()=="float");
	static_assert(fields.get_field("z"_ss).get_type_name()=="std::basic_string_view<char>");
	
	constexpr auto methods=reflect.get_methods();  //获得记录方法的反射信息的对象
	constexpr auto method=methods.get_method("add"_ss);
	
	static_assert(methods.size()==2);              //方法的数量
	static_assert(method.get_name()=="add");       //方法名称为add
	
	static_assert(method.get_type_name()=="int (Node::*)(int, float)"); //函数指针类型
	static_assert(method.get_return_type_name()=="int");                //返回值
	static_assert(method.get_args_type_name_list()==std::array<std::string_view,2>{"int","float"});	//函数形参
	static_assert(method.get_args_type_name<0>()=="int");
	static_assert(method.get_args_type_name<1>()=="float");

	constexpr auto object=reflect.get_instance(1,2.f);    //调用构造函数
	constexpr auto object2=reflect.get_instance();        //调用构造函数
	static_assert(field.get_value(object)==1);
	static_assert(method.constexpr_invoke(object,2,3.f)==8);
	static_assert(method.constexpr_invoke(object2,2,3.f)==10);

	fields.for_each([](auto&&index,auto field){
		cout<<field.get_type_name()<<" "<<field.get_name()<<endl;
	});
	
	methods.for_each([](auto&&index,auto method){
		cout<<method.get_type_name()<<" "<<method.get_name()<<endl;
	});
	

	auto object1=reflect.get_instance(1,2.f);    
	cout<<field.get_value(object1)<<endl;
	cout<<object1.x<<endl;
	field.set_value(object1,72);
	cout<<object1.x<<endl;
	cout<<method.invoke(object1,2,3.f)<<endl;

	
	using reflect_t=static_reflect_t<Node>;                //反射信息类型
	using fields_t=reflect_t::fields_t;                    //所有的属性信息类
	constexpr auto fsize_v=fields_t::size_v;               //属性数量
	using field_t=fields_t::field_ss_t<decltype("x"_ss)>;  //名称为x的属性
	using type=field_t::type;
	static_assert(std::is_same_v<type,int>);               //属性的类型为int
	static_assert(fsize_v==3);                            
	static_assert(field_t::name_v=="x");
	
	static_assert(static_reflect_t<Node>::fields_t::field_t<1>::name_v=="y");           //按照下标来访问元素
	static_assert(is_same_v<static_reflect_t<Node>::fields_t::field_t<1>::type,float>); //按照下标来访问元素
	
	static_assert(1==fields_t::has_field_v<decltype("x"_ss)>);   //检测名称为x的属性是否存在
	static_assert(1==fields_t::has_field_v<decltype("y"_ss)>);
	static_assert(0==fields_t::has_field_v<decltype("xxx"_ss)>);

	
	using reflect_t=static_reflect_t<Node>;
	using methods_t=reflect_t::methods_t;                      //所有的方法的编译期反射信息类
	using method_t=methods_t::method_ss_t<decltype("add"_ss)>; //名称为add的属性
	using return_t=method_t::return_t;                         //返回值类型
	constexpr auto num_params=method_t::size_v;                //参数个数
	
	using param0_t=method_t::parameter_t<0>;                   //具体的每个形参类型
	using param1_t=method_t::parameter_t<1>;

	static_assert(method_t::name_v=="add");              //该属性名称为add
	static_assert(methods_t::size_v==2);                 //属性个数为2
	static_assert(num_params==2);                        //add属性的形参数量为2
	static_assert(is_same_v<param0_t,int>);
	static_assert(is_same_v<param1_t,float>);            //每个形参具体类型
	static_assert(is_same_v<return_t,int>);              //返回值类型
	
	static_assert(1==methods_t::has_method_v<decltype("add"_ss)>); //检测名称为add的方法是否存在
	static_assert(1==methods_t::has_method_v<decltype("mul"_ss)>);
	static_assert(0==methods_t::has_method_v<decltype("func"_ss)>);
	
	
	constexpr auto d1=describe_v<Node>;
	constexpr auto d2=describe_v<Test>;
	cout<<d1<<endl<<d2<<endl;
	
	
	constexpr auto d3=describe_v<Son>;
	cout<<d3<<endl;
	
	
	constexpr auto name=get_type_name_v<
		static_reflect_t<Son>::methods_t::method_ss_t<decltype("func"_ss)>::return_t
	>;
	cout<<name;
	
	
	Son obj;
	cout<<seri::dumps(obj); 
	
}

```

