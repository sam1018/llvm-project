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
} // namespace

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

const MaterializeTemporaryExpr *GetTemporary(const Stmt *stmt) {
  if (isa<MaterializeTemporaryExpr>(stmt))
    return dyn_cast<MaterializeTemporaryExpr>(stmt);

  for (const auto *child : getChildren(stmt)) {
    const auto *res = GetTemporary(child);
    if (res)
      return res;
  }

  return nullptr;
}

} // namespace

void ReferenceReturnedFromTemporaryCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      varDecl(hasType(lValueReferenceType()), unless(parmVarDecl()),
              hasInitializer(
                  traverse(TK_AsIs, expr(isLValue()).bind("theInitializer"))))
          .bind("theVarDecl"),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *TempOb =
      GetTemporary(Result.Nodes.getNodeAs<Expr>("theInitializer"));

  if (!TempOb || TempOb->getStorageDuration() != SD_FullExpression)
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
  // skip proxy... came from boost::fusion library... and it's probably safe to
  // skip something called proxy
  if (llvm::Regex(".*iterator.*|.*proxy.*", llvm::Regex::IgnoreCase)
          .match(TempDeclName))
    return;

  diag(MatchedDecl->getLocation(), "Matched: %0, Temporary Name: %1")
      << MatchedDecl << TempDeclName;
}

} // namespace bugprone
} // namespace tidy
} // namespace clang
