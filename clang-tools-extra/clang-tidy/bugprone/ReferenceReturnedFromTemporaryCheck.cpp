//===--- ReferenceReturnedFromTemporaryCheck.cpp - clang-tidy -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ReferenceReturnedFromTemporaryCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace bugprone {

void ReferenceReturnedFromTemporaryCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      traverse(TK_AsIs,
               varDecl(hasType(lValueReferenceType()), unless(parmVarDecl()),
                       hasInitializer(
                           expr(hasDescendant(
                                    materializeTemporaryExpr().bind("theTemp")))
                               .bind("theInitializer")))
                   .bind("theVarDecl")),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("theVarDecl");
  const auto *Matchedtemporary =
      Result.Nodes.getNodeAs<MaterializeTemporaryExpr>("theTemp");

  if (Matchedtemporary->getStorageDuration() != SD_FullExpression)
    return;

  diag(MatchedDecl->getLocation(), "Matched %0")
      << MatchedDecl;
}

} // namespace bugprone
} // namespace tidy
} // namespace clang
