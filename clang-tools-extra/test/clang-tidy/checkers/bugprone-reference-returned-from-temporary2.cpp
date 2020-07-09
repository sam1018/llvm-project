// RUN: %check_clang_tidy %s bugprone-reference-returned-from-temporary %t

#include <optional>
#include <string>

std::optional<std::pair<std::string, std::string>> f() { return std::make_optional(std::make_pair("abc", "def")); }

const auto &x = f()->first;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: Matched: 'x', Temporary Name: optional [bugprone-reference-returned-from-temporary]
