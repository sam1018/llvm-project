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
                           expr(unless(hasDescendant(lambdaExpr())),
                                hasDescendant(
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

  // skip if temporary object's lifetime is beyond the expression, for example
  // when binding to const&
  if (Matchedtemporary->getStorageDuration() != SD_FullExpression)
    return;

  const auto *TempCXXDecl = Matchedtemporary->getType()->getAsCXXRecordDecl();

  if (!TempCXXDecl) {
    diag(Matchedtemporary->getBeginLoc(),
         "Matched: %0, Temporary is not CXXDecl")
        << MatchedDecl;
    return;
  }

  const auto &TempDeclName = TempCXXDecl->getName();

  // skip if temporary is an iterator, as iterator's dereferenced object's
  // lifetime is not bound to the iterator object
  if (llvm::Regex(".*iterator.*", llvm::Regex::IgnoreCase).match(TempDeclName))
    return;

  diag(MatchedDecl->getLocation(), "Matched: %0, Temporary Name: %1")
      << MatchedDecl << TempDeclName;
}

} // namespace bugprone
} // namespace tidy
} // namespace clang
