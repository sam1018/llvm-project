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
#include <iostream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace bugprone {

namespace {
AST_MATCHER(Expr, isLValue) { return Node.isLValue(); }
} // namespace

namespace {
std::vector<const Stmt *> getChildren(const Stmt *TheStmt) {
  // we don't want to check function arguments for temporaries, as it is quite
  // common to pass temporaries as function parameters... this means, we won't
  // be able to check cases like below unfortunately...
  //
  // int& get(const X& x) { return x.val; }
  // const int& val = get(X{});
  if (isa<CallExpr>(TheStmt)) {
    const auto *TheCallExpr = dyn_cast<CallExpr>(TheStmt);
    llvm::SetVector<const Stmt *> AllArgs(TheCallExpr->arg_begin(),
                                          TheCallExpr->arg_end());
    llvm::SetVector<const Stmt *> NonArgChildren(TheCallExpr->child_begin(),
                                                 TheCallExpr->child_end());

    // std::cout << TheCallExpr->getStmtClassName() << "," << AllArgs.size() <<
    // "\n";

    auto Size1 = NonArgChildren.size();
    NonArgChildren.set_subtract(AllArgs);
    auto Size2 = NonArgChildren.size();

    if (Size1 != Size2 + AllArgs.size())
      llvm::report_fatal_error("Unexpected. Removing arg children failed.");

    if (isa<CXXOperatorCallExpr>(TheStmt)) {
      // this is the one case where we definitely want to check the function
      // argument for temporaries, so we don't miss out on operators like: ->,
      // *, or []... and arg 0 is the caller of the operator...
      if (NonArgChildren.size() < 1)
        llvm::report_fatal_error(
            "Unexpected. CXXOperatorCallExpr has no children.");
      NonArgChildren.insert(TheCallExpr->getArg(0));
    }

    return NonArgChildren.takeVector();
  }

  return {TheStmt->child_begin(), TheStmt->child_end()};
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
BranchState GetBranchState(const Stmt *TheStmt, const ASTContext &Context) {
  if (isa<MemberExpr>(TheStmt)) {
    const auto *MemExpr = dyn_cast<MemberExpr>(TheStmt);
    const auto *MemberDecl = dyn_cast<ValueDecl>(MemExpr->getMemberDecl());
    if (!isa<CXXMethodDecl>(MemberDecl))
      return BranchState::MayDangle;
  }
  if (isa<CXXMemberCallExpr>(TheStmt) || isa<CXXOperatorCallExpr>(TheStmt)) {
    const auto *MemberCall = dyn_cast<CallExpr>(TheStmt);
    const auto &RetType = MemberCall->getCallReturnType(Context);
    if (!RetType.isNull() &&
        (RetType->isReferenceType() || RetType->isPointerType()))
      return BranchState::MayDangle;
    return BranchState::WontDangle;
  }
  return BranchState::Undecided;
}

const MaterializeTemporaryExpr *GetTemporary(const Stmt *TheStmt,
                                             const std::string &TempWhiteListRE,
                                             const ASTContext &Context,
                                             BranchState CurrentBranchState) {

  if (!TheStmt)
    return nullptr;

  const auto *TempOb = dyn_cast<MaterializeTemporaryExpr>(TheStmt);

  // if this is a temporary whose lifetime is getting extended, then return the
  // temporary as it won't dangle
  if (TempOb && TempOb->getExtendingDecl())
    return TempOb;

  if (CurrentBranchState == BranchState::Undecided)
    CurrentBranchState = GetBranchState(TheStmt, Context);

  if (CurrentBranchState == BranchState::WontDangle)
    return nullptr;

  if (CurrentBranchState == BranchState::MayDangle && TempOb) {
    if (IsTempSatisfiesCondition(TempOb, TempWhiteListRE)) {
      return TempOb;
    }
  }

  for (const auto *Child : getChildren(TheStmt)) {
    const auto *Res =
        GetTemporary(Child, TempWhiteListRE, Context, CurrentBranchState);
    if (Res)
      return Res;
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
  // checking expr(isLValue()) saves us from an inconsistent crash with template
  // dependant expressions, when we try to check if a function's return type is
  // ref or pointer... happens with QualType member function call...
  Finder->addMatcher(
      varDecl(unless(isExpansionInSystemHeader()),
              hasType(lValueReferenceType().bind("theVarType")),
              unless(parmVarDecl()),
              hasInitializer(
                  traverse(TK_AsIs, expr(isLValue()).bind("theInitializer"))))
          .bind("theVarDecl"),
      this);
}

void ReferenceReturnedFromTemporaryCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("theVarDecl");
  // MatchedDecl->dump();

  const auto *TempOb =
      GetTemporary(Result.Nodes.getNodeAs<Expr>("theInitializer"),
                   TempWhiteListRE, *Result.Context, BranchState::Undecided);

  if (!TempOb || TempOb->getExtendingDecl())
    return;

  const auto *TempCXXDecl = TempOb->getType()->getAsCXXRecordDecl();

  if (!TempCXXDecl) {
    diag(TempOb->getEndLoc(), "Matched: %0, `Temporary is not CXXRecordDecl`")
        << MatchedDecl;
    return;
  }

  const auto &TempDeclName = TempCXXDecl->getQualifiedNameAsString();

  const auto *MatchedVarType =
      Result.Nodes.getNodeAs<LValueReferenceType>("theVarType");

  diag(
      TempOb->getEndLoc(),
      "Matched. Variable name: `%0`, Variable type: `%1`, Temporary type: `%2`")
      << MatchedDecl
      << MatchedVarType->getPointeeType().getUnqualifiedType().getAsString() << TempDeclName;
}

} // namespace bugprone
} // namespace tidy
} // namespace clang
