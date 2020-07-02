// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t

struct some_struct {
  int val;
  int &get();
}; 

// Match tests:
some_struct f();

int &match1 = some_struct().get();
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Matched: 'match1', Temporary Name: some_struct [bugprone-reference-returned-from-temporary]
const int &match2 = f().get();
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'match2', Temporary Name: some_struct [bugprone-reference-returned-from-temporary]

// No match tests:
// non-reference var decls do not match
int no_match_non_ref;
// initializer is temporary, but binding to const& promotes initializer to lvalue... so the following is ok
const int &no_match_init_promoted_to_lvalue_1 = {};
int some_func();
const int &no_match_init_promoted_to_lvalue_2 = some_func();
// initializer has no temporary object
some_struct ob1;
const int &no_match_init_has_no_temporary = ob1.get();
// function parameters do not match
void some_func(const int &no_match_function_params = {});
// do not match if the temporary object's decl name contains *iterator*
struct test_Iterator_ : public some_struct {};
int &no_match_temp_is_iterator = test_Iterator_().get();