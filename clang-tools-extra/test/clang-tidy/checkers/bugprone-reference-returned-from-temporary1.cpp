// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t -- -config="{CheckOptions: [{key: bugprone-reference-returned-from-temporary.TempWhiteListRE, value: '.*iterator.*|.*proxy.*|ns::match_exact_name_in_ns'}]}" --
// Check check_dangling_reference.py for updated config options to run tests

struct my_struct2 {
  int val2;
};

struct my_struct {
  int val;
  int &ref_get();
  int get();
  my_struct &get_this_ref();
  my_struct *get_this_ptr();
  my_struct2 *operator->();
};

my_struct create_my_struct();
my_struct create_my_struct(const int &);
void arg_cr_my_struct(const my_struct &);
my_struct& arg_rvalue_ref_my_struct(my_struct &&);
my_struct& arg_cr_my_struct_returns_r_my_struct(const my_struct &);
my_struct& arg_cr_int_returns_r_my_struct(const int &);
void use_int(const int&);

// Match tests:
int &match1 = my_struct().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:25: warning: Matched. Variable name: `'match1'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &match2 = create_my_struct().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:38: warning: Matched. Variable name: `'match2'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &match3 = create_my_struct().get_this_ref().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:38: warning: Matched. Variable name: `'match3'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &match4 = create_my_struct(create_my_struct().ref_get()).get_this_ref().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:66: warning: Matched. Variable name: `'match4'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &match5 = my_struct().get_this_ptr()->val;
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: Matched. Variable name: `'match5'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &match6 = my_struct().get_this_ptr()->ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: Matched. Variable name: `'match6'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &match7 = my_struct()->val2;
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: Matched. Variable name: `'match7'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]

// whitelist test - CastFunctionsWhiteList ('cast_func2' is whitelisted):
my_struct ob;
my_struct cast_func(const my_struct &);
my_struct cast_func2(const my_struct &);
const int &match8 = cast_func(ob).ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: Matched. Variable name: `'match8'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &match9 = cast_func(my_struct{}).ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:42:  warning: Matched. Variable name: `'match9'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]
const int &no_match8_1_casting_func_arg_is_non_temp = cast_func2(ob).ref_get();
const int &match9_1_casting_func_arg_is_temporary = cast_func2(my_struct{}).ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:75: warning: Matched. Variable name: `'match9_1_casting_func_arg_is_temporary'`, Variable type: `int`, Temporary type: `my_struct` [bugprone-reference-returned-from-temporary]

// No match tests:
// non-reference var decls do not match
int no_match_non_ref;
// initializer is temporary, but binding to const& promotes initializer to lvalue... so the following is ok
const int &no_match_init_promoted_to_lvalue_1 = {};
int some_func();
const int &no_match_init_promoted_to_lvalue_2 = some_func();
const int &no_match_init_promoted_to_lvalue_3{some_func()};
const my_struct &no_match_init_promoted_to_lvalue_4{true ? my_struct() : my_struct()};
// initializer has no temporary object
my_struct ob1;
const int &no_match_init_has_no_temporary = ob1.ref_get();
// function parameters do not match
void some_func(const int &no_match_function_params = {});
// do not match lambda
const auto &no_match_lambda = []() { arg_cr_my_struct(my_struct()); };
// do not match temporary function args
const auto &no_match_arg_1 = arg_cr_my_struct_returns_r_my_struct(my_struct());
const auto &no_match_arg_2 = arg_cr_my_struct_returns_r_my_struct(create_my_struct());
const auto &no_match_arg_3 = arg_rvalue_ref_my_struct(create_my_struct());
const auto &no_match_arg_4 = arg_cr_int_returns_r_my_struct(create_my_struct().ref_get());
// if function returns non-ref, that's ok
const int &no_match_function_returns_non_ref = my_struct().get();
const int &no_match_function_returns_non_ref2 = create_my_struct().get_this_ref().get();

// whitelist test - TempWhiteListRE('.*iterator.*|.*proxy.*|ns::match_exact_name_in_ns'):
struct test_Iterator_ : public my_struct {};
struct test_Proxy_ : public my_struct {};
int &no_match1 = test_Iterator_().ref_get();
int &no_match2 = test_Proxy_().ref_get();

namespace ns {
struct match_exact_name_in_ns {
  int &ref_get();
};
} // namespace ns

int &no_match3 = ns::match_exact_name_in_ns().ref_get();
