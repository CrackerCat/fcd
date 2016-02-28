//
// expressions.h
// Copyright (C) 2015 Félix Cloutier.
// All Rights Reserved.
//
// This file is part of fcd.
// 
// fcd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// fcd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with fcd.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef fcd__ast_expressions_h
#define fcd__ast_expressions_h

#include "dumb_allocator.h"
#include "expression_use.h"
#include "llvm_warnings.h"
#include "not_null.h"

SILENCE_LLVM_WARNINGS_BEGIN()
#include <llvm/Support/raw_ostream.h>
SILENCE_LLVM_WARNINGS_END()

#include <string>

class AstContext;

#define OPERAND_GET_SET_T(name, type, index) \
	NOT_NULL(type) get##name() { return llvm::cast<type>(getOperand(index)); } \
	NOT_NULL(const type) get##name() const { return llvm::cast<type>(getOperand(index)); } \
	void set##name(NOT_NULL(type) op) { getOperandUse(index).setUse(op); }

#define OPERAND_GET_SET(name, index) OPERAND_GET_SET_T(name, Expression, index)

class Expression : public ExpressionUser
{
	friend class ExpressionUse;
	
private:
	class ExpressionUse* firstUse;
	
protected:
	static bool defaultEqualityCheck(const Expression& a, const Expression& b);
	
public:
	static bool classof(ExpressionUser* user)
	{
		return user->getUserType() < ExpressionMax;
	}
	
	Expression(UserType type, AstContext& ctx, unsigned allocatedUses, unsigned usedUses)
	: ExpressionUser(type, allocatedUses, usedUses)
	{
		// The context parameter only forces subclasses to accept one, for uniformity purposes.
		(void)ctx;
	}
	
	Expression(UserType type, AstContext& ctx, unsigned uses)
	: Expression(type, ctx, uses, uses)
	{
	}
	
	void print(llvm::raw_ostream& os) const;
	void dump() const;
	
	virtual bool operator==(const Expression& that) const = 0;
	
	bool operator!=(const Expression& that) const
	{
		return !(*this == that);
	}
};

struct UnaryOperatorExpression : public Expression
{
	enum UnaryOperatorType : unsigned
	{
		Min = 0,
		
		// The SSA form ensures that we will never need a distinction between prefix and postfix increment/decrement.
		// That's why there's only one of each. We will, however, prefer the prefix version because postfix ++ and --
		// are the only postfix unary operators.
		Increment = Min, Decrement,
		AddressOf, Dereference,
		LogicalNegate,
		Max
	};
	
	UnaryOperatorType type;
	
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == UnaryOperator;
	}
	
	UnaryOperatorExpression(AstContext& ctx, unsigned uses, UnaryOperatorType type, NOT_NULL(Expression) operand)
	: Expression(UnaryOperator, ctx, uses), type(type)
	{
		assert(uses == 1);
		setOperand(operand);
	}
	
	using ExpressionUser::getOperand;
	OPERAND_GET_SET(Operand, 0)
	
	virtual bool operator==(const Expression& that) const override;
};

// Represents a chain of the same binary operator. For instance, +(a, b, c) would be a + b + c.
class NAryOperatorExpression : public Expression
{
	AstContext& ctx;
	
	template<typename... TExpressionType>
	void setOperands(unsigned index, NOT_NULL(Expression) expr, TExpressionType&&... exprs)
	{
		setOperand(index, expr);
		setOperand(index + 1, std::forward<TExpressionType>(exprs)...);
	}
	
public:
	enum NAryOperatorType : unsigned
	{
		Min = UnaryOperatorExpression::Max,
		
		Assign = Min,
		Multiply, Divide, Modulus,
		Add, Subtract,
		ShiftLeft, ShiftRight,
		SmallerThan, SmallerOrEqualTo, GreaterThan, GreaterOrEqualTo,
		Equal, NotEqual,
		BitwiseAnd,
		BitwiseXor,
		BitwiseOr,
		ShortCircuitAnd,
		ShortCircuitOr,
		
		MemberAccess, PointerAccess,
		Max
	};
	
private:
	NAryOperatorType type;
	
public:
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == NAryOperator;
	}
	
	NAryOperatorExpression(AstContext& ctx, unsigned uses, NAryOperatorType type)
	: Expression(NAryOperator, ctx, uses), ctx(ctx), type(type)
	{
	}
	
	template<typename... TExpressionType>
	NAryOperatorExpression(AstContext& ctx, unsigned uses, NAryOperatorType type, TExpressionType... expressions)
	: Expression(NAryOperator, ctx, uses, sizeof...(TExpressionType)), type(type)
	{
		assert(uses >= sizeof...(TExpressionType));
		setOperand(0, expressions...);
	}
	
	using ExpressionUser::setOperand;
	using ExpressionUser::erase;
	
	template<typename TIter>
	void addOperands(TIter begin, TIter end)
	{
		for (auto iter = begin; iter != end; ++iter)
		{
			addOperand(*iter);
		}
	}
	
	template<typename... TExpressionType>
	void addOperand(NOT_NULL(Expression) expression, TExpressionType... expressions)
	{
		addOperand(expression);
		addOperand(expressions...);
	}
	
	void addOperand(NOT_NULL(Expression) expression) { insertUseAtEnd().setUse(expression); }
	
	virtual bool operator==(const Expression& that) const override;
	
private:
	void print(llvm::raw_ostream& os, Expression* expression) const;
};

struct TernaryExpression : public Expression
{
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Ternary;
	}
	
	TernaryExpression(AstContext& ctx, unsigned uses, NOT_NULL(Expression) condition, NOT_NULL(Expression) ifTrue, NOT_NULL(Expression) ifFalse)
	: Expression(Ternary, ctx, uses)
	{
		assert(uses == 3);
		setCondition(condition);
		setTrueValue(ifTrue);
		setFalseValue(ifFalse);
	}
	
	OPERAND_GET_SET(Condition, 0)
	OPERAND_GET_SET(TrueValue, 1)
	OPERAND_GET_SET(FalseValue, 2)
	
	virtual bool operator==(const Expression& that) const override;
};

struct NumericExpression : public Expression
{
	union
	{
		int64_t si64;
		uint64_t ui64;
	};
	
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Numeric;
	}
	
	NumericExpression(AstContext& ctx, unsigned uses, uint64_t ui)
	: Expression(Numeric, ctx, uses), ui64(ui)
	{
		assert(uses == 0);
	}
	
	NumericExpression(AstContext& ctx, unsigned uses, int64_t si)
	: Expression(Numeric, ctx, uses), si64(si)
	{
		assert(uses == 0);
	}
	
	virtual bool operator==(const Expression& that) const override;
};

struct TokenExpression : public Expression
{
	NOT_NULL(const char) token;
	
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Token;
	}
	
	TokenExpression(AstContext& ctx, unsigned uses, llvm::StringRef token);
	
	virtual bool operator==(const Expression& that) const override;
};

class CallExpression : public Expression
{
public:
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Call;
	}
	
	explicit CallExpression(AstContext& ctx, unsigned uses, NOT_NULL(Expression) callee)
	: Expression(Call, ctx, uses)
	{
		assert(uses > 0);
		setCallee(callee);
	}
	
	OPERAND_GET_SET(Callee, 0);
	
	ExpressionUse& getParameter(unsigned index) { return getOperandUse(index + 1); }
	const ExpressionUse& getParameter(unsigned index) const { return getOperandUse(index + 1); }
	void setParameter(unsigned index, NOT_NULL(Expression) param) { setOperand(index + 1, param); }
	
	size_t params_size() const { return operands_size() - 1; }
	iterator params_begin();
	iterator params_end() { return operands_end(); }
	llvm::iterator_range<iterator> params() { return llvm::make_range(params_begin(), params_end()); }
	iterator erase(iterator param);
	
	template<typename TIter>
	void addParameter(TIter begin, TIter end)
	{
		for (auto iter = begin; iter != end; ++iter)
		{
			addOperand(*iter);
		}
	}
	
	template<typename... TExpressionType>
	void addParameter(Expression* expression, TExpressionType... expressions)
	{
		addParameter(expression);
		addParameter(expressions...);
	}
	
	void addParameter(Expression* expression);
	
	virtual bool operator==(const Expression& that) const override;
};

struct CastExpression : public Expression
{
	enum CastSign
	{
		Irrelevant,
		SignExtend,
		ZeroExtend,
	};
	
	CastSign sign;
	
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Cast;
	}
	
	explicit CastExpression(AstContext& ctx, unsigned uses, NOT_NULL(TokenExpression) type, NOT_NULL(Expression) value, CastSign sign)
	: Expression(Cast, ctx, uses), sign(sign)
	{
		assert(uses == 2);
		setCastType(type);
		setCastValue(value);
	}
	
	OPERAND_GET_SET_T(CastType, TokenExpression, 0)
	OPERAND_GET_SET(CastValue, 1)
	
	virtual bool operator==(const Expression& that) const override;
};

class AggregateExpression : public Expression
{
	AstContext& ctx;
	
public:
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Aggregate;
	}
	
	explicit AggregateExpression(AstContext& ctx, unsigned numUses)
	: Expression(Aggregate, ctx, numUses), ctx(ctx)
	{
	}
	
	virtual bool operator==(const Expression& that) const override;
	
	AggregateExpression* copyWithNewItem(unsigned index, NOT_NULL(Expression) expression);
};

struct SubscriptExpression : public Expression
{
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Subscript;
	}
	
	SubscriptExpression(AstContext& ctx, unsigned uses, NOT_NULL(Expression) left, NOT_NULL(Expression) subscript)
	: Expression(Subscript, ctx, uses)
	{
		setPointer(left);
		setIndex(subscript);
	}
	
	OPERAND_GET_SET(Pointer, 0)
	OPERAND_GET_SET(Index, 1)
	
	virtual bool operator==(const Expression& that) const override;
};

struct AssemblyExpression : public Expression
{
	AstContext& ctx;
	PooledDeque<NOT_NULL(const char)> parameterNames;
	NOT_NULL(const char) assembly;
	
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Assembly;
	}
	
	AssemblyExpression(AstContext& ctx, unsigned uses, llvm::StringRef assembly);
	
	void addParameterName(llvm::StringRef parameterName);
	
	virtual bool operator==(const Expression& that) const override;
};

struct AssignableExpression : public Expression
{
	NOT_NULL(const char) prefix;
	
	static bool classof(const ExpressionUser* node)
	{
		return node->getUserType() == Assignable;
	}
	
	AssignableExpression(AstContext& ctx, unsigned uses, NOT_NULL(TokenExpression) type, llvm::StringRef assembly);
	
	OPERAND_GET_SET_T(Type, TokenExpression, 0);
	
	virtual bool operator==(const Expression& that) const override;
};

#undef OPERAND_GET_SET_T
#undef OPERAND_GET_SET

#endif /* fcd__ast_expressions_h */
