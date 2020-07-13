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

bool IsTempSatisfiesCondition(const MaterializeTemporaryExpr *TempOb,
                              const std::string &TempWhiteListRE) {
  if (TempOb->getStorageDuration() != SD_FullExpression)
    return false;

  const auto *TempCXXDecl = TempOb->getType()->getAsCXXRecordDecl();
  // this is weird... returning true for this so we can further investigate
  // what's going on
  if (!TempCXXDecl)
    return true;

  const auto &TempDeclName = TempCXXDecl->getQualifiedNameAsString();

  return llvm::Regex(TempWhiteListRE, llvm::Regex::IgnoreCase)
             .match(TempDeclName) == false;
}

// branch state remains undecided until we see the first member variable or
// member function call. branch state is MayDangle if it is a member variable or
// member function call that returns reference or pointer. otherwise it's
// WontDangle.
enum class BranchState { Undecided, MayDangle, WontDangle };

// here we are looking for the expr on right side of temporary that will dangle
// when temporary goes out of scope... that is,
// create_temp_ob().get_ref() // the get_ref() part
// or
// create_temp_ob().ptr_var->val // and val here... lifetime extension won't
// kick in in this case since val is not a direct member of temp_ob
BranchState GetBranchState(const Stmt *stmt, const ASTContext &Context) {
  if (isa<MemberExpr>(stmt)) {
    const auto *MemExpr = dyn_cast<MemberExpr>(stmt);
    const auto *MemberDecl = dyn_cast<ValueDecl>(MemExpr->getMemberDecl());
    if (!isa<CXXMethodDecl>(MemberDecl))
      return BranchState::MayDangle;
  }
  if (isa<CXXMemberCallExpr>(stmt) || isa<CXXOperatorCallExpr>(stmt)) {
    const auto *MemberCall = dyn_cast<CallExpr>(stmt);
    const auto &RetType = MemberCall->getCallReturnType(Context);
    if (RetType->isReferenceType() || RetType->isPointerType())
      return BranchState::MayDangle;
    return BranchState::WontDangle;
  }
  return BranchState::Undecided;
}

const MaterializeTemporaryExpr *GetTemporary(const Stmt *stmt,
                                             const std::string &TempWhiteListRE,
                                             const ASTContext &Context,
                                             BranchState bs) {

  const auto *TempOb = dyn_cast<MaterializeTemporaryExpr>(stmt);

  // if this is a temporary whose lifetime is getting extended, then return the
  // temporary as it won't dangle
  if (TempOb && TempOb->getExtendingDecl())
    return TempOb;

  if (bs == BranchState::Undecided)
    bs = GetBranchState(stmt, Context);

  if (bs == BranchState::WontDangle)
    return nullptr;

  if (bs == BranchState::MayDangle && TempOb) {
    if (IsTempSatisfiesCondition(TempOb, TempWhiteListRE)) {
      return TempOb;
    }
  }

  for (const auto *child : getChildren(stmt)) {
    const auto *res = GetTemporary(child, TempWhiteListRE, Context, bs);
    if (res)
      return res;
  }

  return nullptr;
}

} // namespace

ReferenceReturnedFromTemporaryCheck::ReferenceReturnedFromTemporaryCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      TempWhiteListRE(Options.get("TempWhiteListRE", "")) {}

void ReferenceReturnedFromTemporaryCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "TempWhiteListRE", TempWhiteListRE);
}

void ReferenceReturnedFromTemporaryCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      varDecl(hasType(lValueReferenceType()), unless(parmVarDecl()),
              hasInitializer(traverse(TK_AsIs, expr().bind("theInitializer"))))
          .bind("theVarDecl"),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("theVarDecl");
  //MatchedDecl->dump();

  const auto *TempOb =
      GetTemporary(Result.Nodes.getNodeAs<Expr>("theInitializer"),
                   TempWhiteListRE, *Result.Context, BranchState::Undecided);

  if (!TempOb || TempOb->getExtendingDecl())
    return;

  const auto *TempCXXDecl = TempOb->getType()->getAsCXXRecordDecl();

  if (!TempCXXDecl) {
    diag(TempOb->getEndLoc(), "Matched: %0, Temporary is not CXXRecordDecl")
        << MatchedDecl;
    return;
  }

  const auto &TempDeclName = TempCXXDecl->getQualifiedNameAsString();

  diag(TempOb->getEndLoc(), "Matched: %0, Temporary Name: %1")
      << MatchedDecl << TempDeclName;
}

} // namespace bugprone
} // namespace tidy
} // namespace clang
