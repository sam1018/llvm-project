// RUN: %clang_cc1 %s -triple i686-pc-win32 -fsyntax-only -Wmicrosoft -verify -fms-extensions -fexceptions -fcxx-exceptions


// ::type_info is predeclared with forward class declartion
void f(const type_info &a);


// Microsoft doesn't validate exception specification.
void foo(); // expected-note {{previous declaration}}
void foo() throw(); // expected-warning {{exception specification in declaration does not match previous declaration}}

void r6() throw(...); // expected-note {{previous declaration}}
void r6() throw(int); // expected-warning {{exception specification in declaration does not match previous declaration}}

struct Base {
  virtual void f2();
  virtual void f3() throw(...);
};

struct Derived : Base {
  virtual void f2() throw(...);
  virtual void f3();
};


// MSVC allows type definition in anonymous union and struct
struct A
{
  union 
  {
    int a;
    struct B  // expected-warning {{types declared in an anonymous union are a Microsoft extension}}
    { 
      int c;
    } d;

    union C   // expected-warning {{types declared in an anonymous union are a Microsoft extension}}
    {
      int e;
      int ee;
    } f;

    typedef int D;  // expected-warning {{types declared in an anonymous union are a Microsoft extension}}
    struct F;  // expected-warning {{types declared in an anonymous union are a Microsoft extension}}
  };

  struct
  {
    int a2;

    struct B2  // expected-warning {{types declared in an anonymous struct are a Microsoft extension}}
    {
      int c2;
    } d2;
    
	union C2  // expected-warning {{types declared in an anonymous struct are a Microsoft extension}}
    {
      int e2;
      int ee2;
    } f2;

    typedef int D2;  // expected-warning {{types declared in an anonymous struct are a Microsoft extension}}
    struct F2;  // expected-warning {{types declared in an anonymous struct are a Microsoft extension}}
  };
};

// __stdcall handling
struct M {
    int __stdcall addP();
    float __stdcall subtractP(); 
};

template<typename T> void h1(T (__stdcall M::* const )()) { }

void m1() {
  h1<int>(&M::addP);
  h1(&M::subtractP);
} 

//MSVC allows forward enum declaration
enum ENUM; // expected-warning {{forward references to 'enum' types are a Microsoft extension}}
ENUM *var = 0;     
ENUM var2 = (ENUM)3;
enum ENUM1* var3 = 0;// expected-warning {{forward references to 'enum' types are a Microsoft extension}}


enum ENUM2 {
	ENUM2_a = (enum ENUM2) 4,
	ENUM2_b = 0x9FFFFFFF, // expected-warning {{enumerator value is not representable in the underlying type 'int'}}
	ENUM2_c = 0x100000000 // expected-warning {{enumerator value is not representable in the underlying type 'int'}}
};


void f(long long);
void f(int);
 
int main()
{
  // This is an ambiguous call in standard C++.
  // This calls f(long long) in Microsoft mode because LL is always signed.
  f(0xffffffffffffffffLL);
  f(0xffffffffffffffffi64);
}

// Enumeration types with a fixed underlying type.
const int seventeen = 17;
typedef int Int;

struct X0 {
  enum E1 : Int { SomeOtherValue } field; // expected-warning{{enumeration types with a fixed underlying type are a Microsoft extension}}
  enum E1 : seventeen;
};

enum : long long {  // expected-warning{{enumeration types with a fixed underlying type are a Microsoft extension}}
  SomeValue = 0x100000000
};


class AAA {
__declspec(dllimport) void f(void) { }
void f2(void);
};

__declspec(dllimport) void AAA::f2(void) { // expected-error {{dllimport attribute can be applied only to symbol}}

}



template <class T>
class BB {
public:
   void f(int g = 10 ); // expected-note {{previous definition is here}}
};

template <class T>
void BB<T>::f(int g = 0) { } // expected-warning {{redefinition of default argument}}


namespace MissingTypename {

template<class T> class A {
public:
	 typedef int TYPE;
};

template<class T> class B {
public:
	 typedef int TYPE;
};


template<class T, class U>
class C : private A<T>, public B<U> {
public:
   typedef A<T> Base1;
   typedef B<U> Base2;
   typedef A<U> Base3;

   A<T>::TYPE a1; // expected-warning {{missing 'typename' prior to dependent type name}}
   Base1::TYPE a2; // expected-warning {{missing 'typename' prior to dependent type name}}

   B<U>::TYPE a3; // expected-warning {{missing 'typename' prior to dependent type name}}
   Base2::TYPE a4; // expected-warning {{missing 'typename' prior to dependent type name}}

   A<U>::TYPE a5; // expected-error {{missing 'typename' prior to dependent type name}}
   Base3::TYPE a6; // expected-error {{missing 'typename' prior to dependent type name}}
 };

}




extern void static_func();
void static_func(); // expected-note {{previous declaration is here}}


static void static_func() // expected-warning {{static declaration of 'static_func' follows non-static declaration}}
{

}

long function_prototype(int a);
long (*function_ptr)(int a);

void function_to_voidptr_conv() {
   void *a1 = function_prototype;
   void *a2 = &function_prototype;
   void *a3 = function_ptr;
}


void pointer_to_integral_type_conv(char* ptr) {
   char ch = (char)ptr;
   short sh = (short)ptr;
   ch = (char)ptr;
   sh = (short)ptr;
} 


struct X1 {
  template<typename T> void f(T);

  template<> void f(int) { } // expected-warning{{Allowing explicit specialization of 'f' in class scope is a Microsoft extension}}
};
