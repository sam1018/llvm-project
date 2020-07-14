// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t -- -config="{CheckOptions: [{key: bugprone-reference-returned-from-temporary.TempWhiteListRE, value: '.*iterator.*|.*proxy.*|ns::match_exact_name_in_ns'}]}" --

namespace ns {
struct match_exact_name_in_ns {
  int &ref_get();
};
} // namespace ns

struct my_struct {
  int &ref_get();
};

struct iterator {
  int &operator[](int ind);
};
struct vector {
  iterator begin();
};
const int &match1 = vector().begin()[10];
// CHECK-MESSAGES: :[[@LINE-1]]:28: warning: Matched: 'match1', Temporary Name: `vector`, Decl Type: `int`, Is Builtin: `1` [bugprone-reference-returned-from-temporary]

// do not match if the temporary object's decl name contains *iterator*
struct test_Iterator_ : public my_struct {};
struct test_Proxy_ : public my_struct {};
int &no_match1 = test_Iterator_().ref_get();
int &no_match2 = test_Proxy_().ref_get();
int &no_match3 = ns::match_exact_name_in_ns().ref_get();
