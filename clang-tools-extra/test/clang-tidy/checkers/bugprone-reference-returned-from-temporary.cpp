// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t

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
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Matched: 'match1', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]
const int &match2 = create_my_struct().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'match2', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]
const int &match3 = create_my_struct().get_this_ref().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'match3', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]
const int &match4 = create_my_struct(create_my_struct().ref_get()).get_this_ref().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'match4', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]
const int &match5 = my_struct().get_this_ptr()->val;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'match5', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]
const int &match6 = my_struct().get_this_ptr()->ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'match6', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]
const int &match7 = my_struct()->val2;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'match7', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]

// No match tests:
// non-reference var decls do not match
int no_match_non_ref;
// initializer is temporary, but binding to const& promotes initializer to lvalue... so the following is ok
const int &no_match_init_promoted_to_lvalue_1 = {};
int some_func();
const int &no_match_init_promoted_to_lvalue_2 = some_func();
const int &no_match_init_promoted_to_lvalue_3 = create_my_struct().get_this_ref().get();
const int &no_match_init_promoted_to_lvalue_4{some_func()};
// initializer has no temporary object
my_struct ob1;
const int &no_match_init_has_no_temporary = ob1.ref_get();
// function parameters do not match
void some_func(const int &no_match_function_params = {});
// do not match if the temporary object's decl name contains *iterator*
struct test_Iterator_ : public my_struct {};
int &no_match_temp_is_iterator = test_Iterator_().ref_get();
// do not match lambda
const auto &no_match_lambda = []() { arg_cr_my_struct(my_struct()); };
// do not match temporary function args
const auto &no_match_arg_1 = arg_cr_my_struct_returns_r_my_struct(my_struct());
const auto &no_match_arg_2 = arg_cr_my_struct_returns_r_my_struct(create_my_struct());
const auto &no_match_arg_3 = arg_rvalue_ref_my_struct(create_my_struct());
const auto &no_match_arg_4 = arg_cr_int_returns_r_my_struct(create_my_struct().ref_get());
// if function returns non-ref, that's ok
const int &no_match_function_returns_non_ref = my_struct().get();
