// RUN: %check_clang_tidy -std=c++11-or-later %s google-readability-casting %t
// FIXME: Fix the checker to work in C++17 mode.

bool g() { return false; }

enum Enum { Enum1 };
struct X {};
struct Y : public X {};

void f(int a, double b, const char *cpc, const void *cpv, X *pX) {
  const char *cpc2 = cpc;
  //
  //

  typedef const char *Typedef1;
  typedef const char *Typedef2;
  Typedef1 t1;
  static_cast<Typedef2>(t1);
  //
  //
  static_cast<const char*>(t1);
  //
  //
  static_cast<Typedef1>(cpc);
  //
  //
  t1;
  //
  //

  char *pc = const_cast<char*>(cpc);
  //
  //
  typedef char Char;
  Char *pChar = static_cast<Char*>(pc);
  //
  //

  static_cast<Char>(*cpc);
  //
  //

  static_cast<char>(*pChar);
  //
  //

  static_cast<const char*>(cpv);
  //
  //

  char *pc2 = const_cast<char*>(cpc + 33);
  //
  //

  const char &crc = *cpc;
  char &rc = const_cast<char&>(crc);
  //
  //

  char &rc2 = const_cast<char&>(*cpc);
  //
  //

  char ** const* const* ppcpcpc;
  char ****ppppc = const_cast<char****>(ppcpcpc);
  //
  //

  char ***pppc = const_cast<char***>(*(ppcpcpc));
  //
  //

  char ***pppc2 = const_cast<char***>(*ppcpcpc);
  //
  //

  char *pc5 = const_cast<char*>(static_cast<const char*>(cpv));
  //
  //
  //

  int b1 = static_cast<int>(b);
  //
  //
  b1 = (const int&)b;
  //
  //

  b1 = static_cast<int>(b);
  //
  //

  b1 = static_cast<int>(b);
  //
  //

  b1 = static_cast<int>(b);
  //
  //

  b1 = static_cast<int>(b);
  //
  //

  Y *pB = (Y*)pX;
  //
  Y &rB = (Y&)*pX;
  //

  const char *pc3 = static_cast<const char*>(cpv);
  //
  //

  char *pc4 = (char*)cpv;
  //
  //

  b1 = static_cast<int>(Enum1);
  //
  //

  Enum e = static_cast<Enum>(b1);
  //
  //

  e = Enum1;
  //
  //

  e = e;
  //
  //

  e = e;
  //
  //

  e = (e);
  //
  //

  static const int kZero = 0;
  kZero;
  //
  //

  int b2 = int(b);
  int b3 = static_cast<double>(b);
  int b4 = b;
  double aa = a;
  (void)b2;
  return (void)g();
}

template <typename T>
void template_function(T t, int n) {
  int i = (int)t;
  //
  //
  int j = n;
  //
  //
}

template <typename T>
struct TemplateStruct {
  void f(T t, int n) {
    int k = (int)t;
    //
    //
    int l = n;
    //
    //
  }
};

void test_templates() {
  template_function(1, 42);
  template_function(1.0, 42);
  TemplateStruct<int>().f(1, 42);
  TemplateStruct<double>().f(1.0, 42);
}

extern "C" {
void extern_c_code(const char *cpc) {
  const char *cpc2 = cpc;
  //
  //
  char *pc = (char*)cpc;
}
}

#define CAST(type, value) (type)(value)
void macros(double d) {
  int i = CAST(int, d);
}

enum E { E1 = 1 };
template <E e>
struct A {
  // Usage of template argument e = E1 is represented as (E)1 in the AST for
  // some reason. We have a special treatment of this case to avoid warnings
  // here.
  static const E ee = e;
};
struct B : public A<E1> {};


void overloaded_function();
void overloaded_function(int);

template<typename Fn>
void g(Fn fn) {
  fn();
}

void function_casts() {
  typedef void (*FnPtrVoid)();
  typedef void (&FnRefVoid)();
  typedef void (&FnRefInt)(int);

  g(static_cast<void (*)()>(overloaded_function));
  //
  //
  g(static_cast<void (*)()>(&overloaded_function));
  //
  //
  g(static_cast<void (&)()>(overloaded_function));
  //
  //

  g(static_cast<FnPtrVoid>(overloaded_function));
  //
  //
  g(static_cast<FnPtrVoid>(&overloaded_function));
  //
  //
  g(static_cast<FnRefVoid>(overloaded_function));
  //
  //

  FnPtrVoid fn0 = static_cast<void (*)()>(&overloaded_function);
  //
  //
  FnPtrVoid fn1 = static_cast<void (*)()>(overloaded_function);
  //
  //
  FnPtrVoid fn1a = static_cast<FnPtrVoid>(overloaded_function);
  //
  //
  FnRefInt fn2 = static_cast<void (&)(int)>(overloaded_function);
  //
  //
  auto fn3 = static_cast<void (*)()>(&overloaded_function);
  //
  //
  auto fn4 = static_cast<void (*)()>(overloaded_function);
  //
  //
  auto fn5 = static_cast<void (&)(int)>(overloaded_function);
  //
  //

  void (*fn6)() = static_cast<void (*)()>(&overloaded_function);
  //
  //
  void (*fn7)() = static_cast<void (*)()>(overloaded_function);
  //
  //
  void (*fn8)() = static_cast<FnPtrVoid>(overloaded_function);
  //
  //
  void (&fn9)(int) = static_cast<void (&)(int)>(overloaded_function);
  //
  //

  void (*correct1)() = static_cast<void (*)()>(overloaded_function);
  FnPtrVoid correct2 = static_cast<void (*)()>(&overloaded_function);
  FnRefInt correct3 = static_cast<void (&)(int)>(overloaded_function);
}

struct S {
    S(const char *);
};
struct ConvertibleToS {
  operator S() const;
};
struct ConvertibleToSRef {
  operator const S&() const;
};

void conversions() {
  //auto s1 = (const S&)"";
  // C HECK-MESSAGES: :[[@LINE-1]]:10: warning: C-style casts are discouraged; use static_cast [
  // C HECK-FIXES: S s1 = static_cast<const S&>("");
  auto s2 = S("");
  //
  //
  auto s2a = static_cast<struct S>("");
  //
  //
  auto s2b = static_cast<const S>("");
  //
  // FIXME: This should be constructor call syntax: S("").
  //
  ConvertibleToS c;
  auto s3 = (const S&)c;
  //
  //
  // FIXME: This should be a static_cast.
  // C HECK-FIXES: auto s3 = static_cast<const S&>(c);
  auto s4 = S(c);
  //
  //
  ConvertibleToSRef cr;
  auto s5 = (const S&)cr;
  //
  //
  // FIXME: This should be a static_cast.
  // C HECK-FIXES: auto s5 = static_cast<const S&>(cr);
  auto s6 = S(cr);
  //
  //
}
