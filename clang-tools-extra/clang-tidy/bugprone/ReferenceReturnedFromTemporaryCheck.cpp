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
AST_MATCHER(Expr, isLValue) { return Node.isLValue(); }
}

namespace {
std::vector<const Stmt *> getChildren(const Stmt *stmt) {
  if (isa<CallExpr>(stmt) && !isa<CXXOperatorCallExpr>(stmt)) {
    const auto *callExpr = dyn_cast<CallExpr>(stmt);
    llvm::SetVector<const Stmt *> all_args(callExpr->arg_begin(),
                                           callExpr->arg_end());
    llvm::SetVector<const Stmt *> non_arg_children(callExpr->child_begin(),
                                                   callExpr->child_end());

    auto size1 = non_arg_children.size();
    non_arg_children.set_subtract(all_args);
    auto size2 = non_arg_children.size();

    // temporary... should be assert, checking like below as I am only doing
    // release builds now...
    if (size1 != size2 + all_args.size())
      llvm::report_fatal_error(
          "Unexpected. Set subtraction produced wrong result.");

    return non_arg_children.takeVector();
  }

  return {stmt->child_begin(), stmt->child_end()};
}

const MaterializeTemporaryExpr *
GetTemporaryWithSdFullExpression(const Stmt *stmt) {
  if (isa<MaterializeTemporaryExpr>(stmt)) {
    const auto *temp = dyn_cast<MaterializeTemporaryExpr>(stmt);
    if (temp->getStorageDuration() == SD_FullExpression)
      return temp;
  }

  for (const auto *child : getChildren(stmt)) {
    const auto *res = GetTemporaryWithSdFullExpression(child);
    if (res)
      return res;
  }

  return nullptr;
}

} // namespace

void ReferenceReturnedFromTemporaryCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      varDecl(
          hasType(lValueReferenceType()), unless(parmVarDecl()),
          hasInitializer(ignoringParenImpCasts(expr(isLValue()).bind("theInitializer"))))
          .bind("theVarDecl"),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedInitExpr = Result.Nodes.getNodeAs<Expr>("theInitializer");

  if (isa<CallExpr>(MatchedInitExpr)) {
    const auto *callExpr = dyn_cast<CallExpr>(MatchedInitExpr);
    const auto &RetType = callExpr->getCallReturnType(*Result.Context);
    if (!RetType->isPointerType() && !RetType->isReferenceType())
      return;
  }

  const auto *TempOb = GetTemporaryWithSdFullExpression(MatchedInitExpr);
  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("theVarDecl");

  if (!TempOb)
    return;

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
