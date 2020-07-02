// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t

struct some_struct {
  int val;
  int &get();
}; 

// Match tests:
some_struct f();

int &match1 = some_struct().get();
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Matched 'match1' [bugprone-reference-returned-from-temporary]
const int &match2 = f().get();
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched 'match2' [bugprone-reference-returned-from-temporary]

// No match tests:
// non-reference var decls do not match
int no_match1;
// initializer is temporary, but binding to const& promotes initializer to lvalue... so the following is ok
int some_func();
const int &no_match2 = some_func();
// initializer has no temporary object
some_struct ob1;
const int &no_match3 = ob1.get();