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
} // namespace

void ReferenceReturnedFromTemporaryCheck::registerMatchers(
    MatchFinder *Finder) {
  const auto &initExprCheck = expr(
      unless(lambdaExpr()),
      traverse(TK_AsIs,
               hasDescendant(materializeTemporaryExpr(isSD_FullExpression())
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
  const auto *MemberCallExpr = dyn_cast<CXXMemberCallExpr>(MatchedInitExpr);
  std::string temp;
  llvm::raw_string_ostream os(temp);
  if (MemberCallExpr) {
    MemberCallExpr->getImplicitObjectArgument()->dump(os);
  } else {
    temp = "Non member function call!!";
  }

  const auto *MatchedMemberCallExpr =
      Result.Nodes.getNodeAs<CXXMemberCallExpr>("theInitializer");

  // initializer must be a member call
  if (!MatchedMemberCallExpr)
    return;

  // the member function's return type must be a reference
  if (!MatchedMemberCallExpr->getCallReturnType(*Result.Context)
           ->isReferenceType())
    return;

  // walk back through callers as long as member caller itself is a member function whose return type is also a reference
  auto *CurrentCallExpr = MemberCallExpr;
  const auto *Caller =
      dyn_cast<CXXMemberCallExpr>(CurrentCallExpr->getImplicitObjectArgument());
  while (Caller &&
         Caller->getCallReturnType(*Result.Context)->isReferenceType()) {
    CurrentCallExpr = Caller;
    Caller = dyn_cast<CXXMemberCallExpr>(
        CurrentCallExpr->getImplicitObjectArgument());
  }

  // after finishing the previous loop, now we want to check if current MemberCaller is a temporary
  if (!isa<MaterializeTemporaryExpr>(
          CurrentCallExpr->getImplicitObjectArgument()))
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
