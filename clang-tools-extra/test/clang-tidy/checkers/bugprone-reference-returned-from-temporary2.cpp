// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t

#include <optional>
#include <string>
#include <map>

std::optional<std::pair<std::string, std::string>> f() { return std::make_optional(std::make_pair("abc", "def")); }

const auto &match1 = f()->first;
// CHECK-MESSAGES: :[[@LINE-1]]:24: warning: Matched: 'match1', Temporary Name: `std::optional`, Decl Type: `class std::basic_string<char, struct std::char_traits<char>, class std::allocator<char> >`, Is Builtin: `0` [bugprone-reference-returned-from-temporary]

const auto &no_match1{std::to_string(10)};
std::map<int, std::string> m;
const auto &no_match2 = m[10];
