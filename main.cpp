#include"static_reflect.h"
#include<cstdio>
using namespace std;
struct Node
{
	constexpr Node(){}
	constexpr Node(int x,float y):x{x},y{y}{} //�������캯�����ǵñ���public...
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
	static consteval auto get_config() //ע�ᷴ�������meta data
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
	
	constexpr auto refl_info=static_reflect(Node);                   //�õ���ķ�����Ϣ
	constexpr auto methods=refl_info.get_methods();                  //�õ�������з�����tuple
	constexpr auto method=methods.get_method("add"_ss);              //�õ�����Ϊ"add"�ķ���
	constexpr auto fields=refl_info.get_fields();                    //�õ�����������Ե�tuple
	constexpr auto field=fields.get_field("x"_ss);                   //�õ�����Ϊ"x"�ķ���
	
	constexpr auto node=refl_info.get_instance(2,3.f);               //ʵ����һ������,���ö�Ӧ�Ĺ��캯��
	
	constexpr auto method_name=method.get_name();                    //��ȡ��������
	constexpr auto method_type=method.get_type_name();               //��ȡ������������
	constexpr auto method_type_id=method.get_type_id();              //��ȡ�����������ƵĹ�ϣֵ
	constexpr auto method_value=method.constexpr_invoke(node,2,3.f); //���÷���
	
	constexpr auto args_types=method.get_args_type_name();           //�õ��÷����Ĳ������������б�
	constexpr auto return_type=method.get_return_type_name();       //�õ��÷����ķ���ֵ��������
	
	constexpr auto field_name=field.get_name();                      //�õ������Ե�����
	constexpr auto field_type=field.get_type_name();                 //�õ������Ե���������
	constexpr auto value=field.get_value(node);                      //�õ������Ե�ֵ
	//field.set_value(node,2333);                                    //���ø����Ե�ֵ,constexpr���󲻿���.
	
	static_assert(std::is_same<decltype(refl_info.type()),Node>::value);
	static_assert(refl_info.get_name()=="Node");
	static_assert(methods.size()==2);                                         //��Ա����������
	static_assert(fields.size()==2);                                          //��Ա����������
	

	static_assert(std::is_same<decltype(method.return_type()),int>::value);   //�õ���Ա�����ķ���ֵ����(���ַ���)
	static_assert(std::is_same<decltype(method.arg_type<0>()),int>::value);   //�õ���Ա������0����������
	static_assert(std::is_same<decltype(method.arg_type<1>()),float>::value); //�õ���Ա������1����������
	static_assert(args_types.size()==2);                                      //�ó�Ա������������
	static_assert(method.constexpr_invoke(node,2,3.f)==10);                   //���ù��캯����������ھ�̬����
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
	constexpr auto refl_info=static_reflect(Node);                   //�õ���ķ�����Ϣ
	constexpr auto fields=refl_info.get_fields();                    //�õ�����������Ե�tuple
	constexpr auto field=fields.get_field("x"_ss);                   //�õ�����Ϊ"x"�ķ���
	constexpr auto methods=refl_info.get_methods();                  //�õ�������з�����tuple
	constexpr auto method=methods.get_method("add"_ss);              //�õ�����Ϊ"add"�ķ���
	Node node=refl_info.get_instance(2,3.f);                         //ʵ����һ������,���ö�Ӧ�Ĺ��캯��
	volatile int x=2333;
	field.set_value(node,x);                                         //�޸�ֵ
	printf("%d,",field.get_value(node));                            //�õ�ֵ
	printf("%d\n",method.invoke(node,2,3.f));
	
	constexpr auto args_types=method.get_args_type_name();           //�õ��÷����Ĳ������������б�
	constexpr auto return_type=method.get_return_type_name();       //�õ��÷����ķ���ֵ��������
	static_assert(std::is_same<decltype(method.return_type()),int>::value);   //�õ���Ա�����ķ���ֵ����(���ַ���)
	static_assert(std::is_same<decltype(method.arg_type<0>()),int>::value);   //�õ���Ա������0����������
	static_assert(std::is_same<decltype(method.arg_type<1>()),float>::value); //�õ���Ա������1����������
	static_assert(args_types.size()==2);                                      //�ó�Ա������������
	static_assert(method.get_name()=="add");
	static_assert(method.get_type_name()=="int (Node::*)(int, float)");
	static_assert(method.get_type_id()==4425629840105553482ll);
	static_assert(args_types[0]=="int");
	static_assert(args_types[1]=="float");
	static_assert(return_type=="int");
	*/
}
