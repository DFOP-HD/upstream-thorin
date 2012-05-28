#ifndef IMPALA_AST_H
#define IMPALA_AST_H

#include <vector>

#include "anydsl/util/box.h"
#include "anydsl/util/location.h"

#include "impala/token.h"

namespace impala {

class ASTNode {
public:

    virtual ~ASTNode() {}

    anydsl::Location loc;
};

class Prg : public ASTNode {
public :
};

class Type;

class Decl : public ASTNode {
public:

    Decl(const Token& id, const Type* type);

    anydsl::Symbol id() const { return id_; }
    const Type* type() const { return type_; }

private:

    anydsl::Symbol id_;
    const Type* type_;
};

//------------------------------------------------------------------------------

class Type : public ASTNode {
public:
};

class PrimType : public Type {
public:

    enum Kind {
#define IMPALA_TYPE(itype, atype) TYPE_##itype = Token:: TYPE_##itype,
#include "impala/tokenlist.h"
    };

    PrimType(const anydsl::Location& loc, Kind kind);

    Kind kind() const { return kind_; }

private:

    Kind kind_;
};

//------------------------------------------------------------------------------

class Expr : public ASTNode {
protected:

    std::vector<const Expr*> args_;
};

class EmptyExpr : public Expr {
public:

    EmptyExpr(const anydsl::Location& loc) {
        this->loc = loc;
    }
};

class Literal : public Expr {
public:

    enum Kind {
#define IMPALA_LIT(tok, t) tok = Token:: tok,
#include "impala/tokenlist.h"
        BOOL
    };

    Literal(const anydsl::Location& loc, Kind kind, anydsl::Box value);

    Kind kind() const { return kind_; }
    anydsl::Box value() const { return value_; }

private:

    Kind kind_;
    anydsl::Box value_;
};

class PrefixExpr : public Expr {
public:

    enum Kind {
#define IMPALA_PREFIX(tok, str, prec) tok = Token:: tok,
#include "impala/tokenlist.h"
    };

    PrefixExpr(const anydsl::Position& pos1, Kind kind, const Expr* rexpr);

    const Expr* rexpr() const { return args_[0]; }

    Kind kind() const { return kind_; }

private:

    Kind kind_;
};

class InfixExpr : public Expr {
public:

    enum Kind {
#define IMPALA_INFIX_ASGN(tok, str, lprec, rprec) tok = Token:: tok,
#define IMPALA_INFIX(     tok, str, lprec, rprec) tok = Token:: tok,
#include "impala/tokenlist.h"
    };

    InfixExpr(const Expr* lexpr, Kind kind, const Expr* rexpr);

    const Expr* lexpr() const { return args_[0]; }
    const Expr* rexpr() const { return args_[1]; }

    Kind kind() const { return kind_; }

private:

    Kind kind_;
};

/**
 * Just for expr++ and expr--.
 * For indexing and function calls use IndexExpr or CallExpr, respectively.
 */
class PostfixExpr : public Expr {
public:

    enum Kind {
        INC = Token::INC,
        DEC = Token::DEC
    };

    PostfixExpr(const Expr* lexpr, Kind kind, const anydsl::Position& pos2);

    const Expr* lexpr() const { return args_[0]; }

    Kind kind() const { return kind_; }

private:

    Kind kind_;
};

//------------------------------------------------------------------------------

class Stmt : public ASTNode {
};

class EmptyStmt : public Stmt {
public:

    EmptyStmt(const anydsl::Location& loc) {
        this->loc = loc;
    }
};

class ExprStmt : public Stmt {
public:

    ExprStmt(const Expr* expr, const anydsl::Position& pos2);

    const Expr* expr() const { return expr_; }

private:

    const Expr* expr_;
};

class DeclStmt : public Stmt {
public:

    DeclStmt(const Decl* decl, const Expr* init, const anydsl::Position& pos2);

    const Decl* decl_;
    const Expr* init_;
};

class IfElseStmt: public Stmt {
public:

    IfElseStmt(const anydsl::Position& pos1, const Expr* cond, const Stmt* ifStmt, const Stmt* elseStmt);

    const Expr* cond() const { return cond_; }
    const Stmt* ifStmt() const { return ifStmt_; }
    const Stmt* elseStmt() const { return elseStmt_; }

private:

    const Expr* cond_;
    const Stmt* ifStmt_;
    const Stmt* elseStmt_;
};

class Loop : public Stmt {
public:

    Loop() {}

    const Expr* cond() const { return cond_; }
    const Stmt* body() const { return body_; }

protected:

    void set(const Expr* cond, const Stmt* body) {
        cond_ = cond;
        body_ = body;
    }

private:

    const Expr* cond_;
    const Stmt* body_;
};

class WhileStmt : public Loop {
public:

    WhileStmt() {}
        
    void set(const anydsl::Position& pos1, const Expr* cond, const Stmt* body);
};

class DoWhileStmt : public Loop {
public:

    DoWhileStmt() {}

    void set(const anydsl::Position& pos1, const Stmt* body, const Expr* cond, const anydsl::Position& pos2);
};

class ForStmt : public Loop {
public:

    ForStmt(const anydsl::Position& pos1, const Expr* cond, const Expr* inc, const Stmt* body);

private:

    const Expr* inc_;
};

class BreakStmt : public Stmt {
public:

    BreakStmt(const anydsl::Position& pos1, const anydsl::Position& pos2, const Loop* loop);

private:

    const Loop* loop_;
};

class ContinueStmt : public Stmt {
public:

    ContinueStmt(const anydsl::Position& pos1, const anydsl::Position& pos2, const Loop* loop);

private:

    const Loop* loop_;
};

class ReturnStmt : public Stmt {
public:

    ReturnStmt(const anydsl::Position& pos1, const Expr* expr, const anydsl::Position& pos2);

    const Expr* expr() const { return expr_; }
private:

    const Expr* expr_;
};

class ScopeStmt : public Stmt {
public:

    ScopeStmt() {}

private:

    std::vector<const Stmt*> stmts_;

    friend class Parser;
};

//------------------------------------------------------------------------------

} // namespace impala

#endif // IMPALA_AST_H
