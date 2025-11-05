#include "nova/Frontend/AST.h"

namespace nova {

// Implement accept methods for all AST nodes
void NumberLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StringLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BooleanLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NullLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UndefinedLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Identifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BinaryExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UpdateExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MemberExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ConditionalExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArrayExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ObjectExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArrowFunctionExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ClassExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NewExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ThisExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SuperExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SpreadExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TemplateLiteralExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AwaitExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void YieldExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AsExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SatisfiesExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NonNullExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TaggedTemplateExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SequenceExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AssignmentExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ParenthesizedExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MetaProperty::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ImportExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// JSX/TSX
void JSXElement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void JSXFragment::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void JSXText::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void JSXExpressionContainer::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void JSXAttribute::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void JSXSpreadAttribute::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// Patterns
void ObjectPattern::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArrayPattern::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AssignmentPattern::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void RestElement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IdentifierPattern::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void Decorator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void BlockStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ExprStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void VarDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void DeclStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IfStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WhileStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void DoWhileStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForInStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForOfStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BreakStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ContinueStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ThrowStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TryStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SwitchStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void LabeledStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WithStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void DebuggerStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void EmptyStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void FunctionDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ClassDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void InterfaceDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TypeAliasDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void EnumDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ImportDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ExportDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Program::accept(ASTVisitor& visitor) { visitor.visit(*this); }

} // namespace nova
