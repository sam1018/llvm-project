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
      varDecl(hasType(lValueReferenceType()), unless(parmVarDecl()),
              hasInitializer(cxxMemberCallExpr().bind("theMemberCall")))
          .bind("theVarDecl"),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedMemberCallExpr =
      Result.Nodes.getNodeAs<CXXMemberCallExpr>("theMemberCall");

  // the member function's return type must be a reference
  if (!MatchedMemberCallExpr->getCallReturnType(*Result.Context)
           ->isReferenceType())
    return;

  // walk back through callers as long as member caller itself is a member function whose return type is also a reference
  auto *CurrentCallExpr = MatchedMemberCallExpr;
  const auto *Caller =
      dyn_cast<CXXMemberCallExpr>(CurrentCallExpr->getImplicitObjectArgument());
  while (Caller &&
         Caller->getCallReturnType(*Result.Context)->isReferenceType()) {
    CurrentCallExpr = Caller;
    Caller = dyn_cast<CXXMemberCallExpr>(
        CurrentCallExpr->getImplicitObjectArgument());
  }

  // after finishing the previous loop, now we want to check if current MemberCaller is a temporary
  const auto *TempOb = dyn_cast<MaterializeTemporaryExpr>(
      CurrentCallExpr->getImplicitObjectArgument());
  if (!TempOb)
    return;

  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("theVarDecl");
  const auto *TempCXXDecl = TempOb->getType()->getAsCXXRecordDecl();

  if (!TempCXXDecl) {
    diag(TempOb->getBeginLoc(),
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
