// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t -- -config="{CheckOptions: [{key: bugprone-reference-returned-from-temporary.TempWhiteListRE, value: '.*iterator.*|.*proxy.*'}]}" --

struct my_struct {
  int &ref_get();
};

const int &val = my_struct().ref_get();
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Matched: 'val', Temporary Name: my_struct [bugprone-reference-returned-from-temporary]

// do not match if the temporary object's decl name contains *iterator*
struct test_Iterator_ : public my_struct {};
struct test_Proxy_ : public my_struct {};
int &no_match_temp_is_iterator = test_Iterator_().ref_get();
int &no_match_temp_is_proxy = test_Proxy_().ref_get();
