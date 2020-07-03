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

namespace {
AST_MATCHER(MaterializeTemporaryExpr, isSD_FullExpression) {
  return Node.getStorageDuration() == SD_FullExpression;
}

AST_MATCHER(Expr, isLValue) { return Node.isLValue(); }
} // namespace

void ReferenceReturnedFromTemporaryCheck::registerMatchers(
    MatchFinder *Finder) {
  const auto &initExprCheck =
      expr(unless(lambdaExpr()),
           traverse(TK_AsIs,
                    hasDescendant(materializeTemporaryExpr(
                                      isSD_FullExpression(), unless(isLValue()))
                                      .bind("theTemp"))));

  Finder->addMatcher(
      varDecl(hasType(lValueReferenceType()), unless(parmVarDecl()),
              hasInitializer(initExprCheck.bind("theInitializer")))
          .bind("theVarDecl"),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedInitExpr = Result.Nodes.getNodeAs<Expr>("theInitializer");
  const auto *InitCallExpr = dyn_cast<CallExpr>(MatchedInitExpr);
  if (!InitCallExpr ||
      !InitCallExpr->getCallReturnType(*Result.Context)->isReferenceType())
    return;

  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("theVarDecl");
  const auto *Matchedtemporary =
      Result.Nodes.getNodeAs<MaterializeTemporaryExpr>("theTemp");

  const auto *TempCXXDecl = Matchedtemporary->getType()->getAsCXXRecordDecl();

  if (!TempCXXDecl) {
    diag(Matchedtemporary->getBeginLoc(),
         "Matched: %0, Temporary is not CXXRecordDecl")
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
