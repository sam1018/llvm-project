// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t

#include <optional>
#include <string>

std::optional<std::pair<std::string, std::string>> f() { return std::make_optional(std::make_pair("abc", "def")); }

const auto &match1 = f()->first;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: Matched: 'match1', Temporary Name: std::optional [bugprone-reference-returned-from-temporary]

const auto &no_match1{std::to_string(10)};