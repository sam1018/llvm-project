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
bool satisfiesCondition(const Expr *expr, const ASTContext &context) {
  if (isa<CXXMemberCallExpr>(expr)) {
    const auto *MemberCallExpr = dyn_cast<CXXMemberCallExpr>(expr);
    const auto &RetType = MemberCallExpr->getCallReturnType(context);
    return RetType->isReferenceType() || RetType->isPointerType();
  }

  if (isa<MemberExpr>(expr)) {
    const auto *MemberVarExpr = dyn_cast<MemberExpr>(expr);
    return MemberVarExpr->isLValue();
  }

  return false;
}

Expr *CallerExpr(const Expr *expr) {
  if (isa<CXXMemberCallExpr>(expr)) {
    const auto *MemberCallExpr = dyn_cast<CXXMemberCallExpr>(expr);
    return MemberCallExpr->getImplicitObjectArgument();
  }

  if (isa<MemberExpr>(expr)) {
    const auto *MemberVarExpr = dyn_cast<MemberExpr>(expr);
    return MemberVarExpr->getBase();
  }

  return nullptr;
}
} // namespace

void ReferenceReturnedFromTemporaryCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      varDecl(hasType(lValueReferenceType()), unless(parmVarDecl()),
              hasInitializer(expr(anyOf(cxxMemberCallExpr(), memberExpr()))
                                 .bind("theInitializer")))
          .bind("theVarDecl"),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *CurrentExpr = Result.Nodes.getNodeAs<Expr>("theInitializer");
  if (!satisfiesCondition(CurrentExpr, *Result.Context))
    return;

  const auto *PrevExpr = CurrentExpr;
  CurrentExpr = CallerExpr(CurrentExpr);
  while (satisfiesCondition(CurrentExpr, *Result.Context)) {
    PrevExpr = CurrentExpr;
    CurrentExpr = CallerExpr(CurrentExpr);
  }

  const auto *TempOb = dyn_cast<MaterializeTemporaryExpr>(CurrentExpr);
  if (!TempOb)
    return;

  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("theVarDecl");
  const auto *TempCXXDecl = TempOb->getType()->getAsCXXRecordDecl();

  if (!TempCXXDecl) {
    diag(TempOb->getBeginLoc(), "Matched: %0, Temporary is not CXXRecordDecl")
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
