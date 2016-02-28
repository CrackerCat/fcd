//
// expression_use.cpp
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

#include "expression_use.h"
#include "expressions.h"

using namespace llvm;
using namespace std;

namespace
{
	template<typename TAction>
	void iterateUseArrays(ExpressionUse* arrayEnd, unsigned used, unsigned allocated, TAction&& action)
	{
		auto arrayBegin = arrayEnd - allocated;
		auto nextHead = &reinterpret_cast<ExpressionUseArrayHead*>(arrayBegin)[-1];
		while (arrayEnd != nullptr && action(arrayEnd - used, arrayEnd))
		{
			used = nextHead->allocatedAndUsed.second;
			arrayBegin = nextHead->array;
			arrayEnd = arrayBegin == nullptr ? nullptr : arrayBegin + nextHead->allocatedAndUsed.first;
		}
	}
	
	template<typename TAction>
	void iterateUseArrays(const ExpressionUse* arrayEnd, unsigned used, unsigned allocated, TAction&& action)
	{
		auto arrayBegin = arrayEnd - allocated;
		auto nextHead = &reinterpret_cast<const ExpressionUseArrayHead*>(arrayBegin)[-1];
		while (arrayEnd != nullptr && action(arrayEnd - used, arrayEnd))
		{
			used = nextHead->allocatedAndUsed.second;
			arrayBegin = nextHead->array;
			arrayEnd = arrayBegin == nullptr ? nullptr : arrayBegin + nextHead->allocatedAndUsed.first;
		}
	}
}

void ExpressionUse::setNext(ExpressionUse* use)
{
	next = use;
	next->prev.setPointer(this);
}

pair<ExpressionUse*, size_t> ExpressionUse::walkWay()
{
	size_t total = 0;
	ExpressionUse* current = this;
	while (current->prev.getInt() < Stop)
	{
		total <<= 1;
		total |= current->prev.getInt();
	}
	return make_pair(current, total);
}

pair<ExpressionUse*, ExpressionUser*> ExpressionUse::walkToEndOfArray()
{
	ExpressionUse* stop;
	size_t skip;
	tie(stop, skip) = walkWay();
	
	if (stop->prev.getInt() == FullStop)
	{
		skip = 1;
	}
	else
	{
		tie(stop, skip) = stop->walkWay();
	}
	
	ExpressionUse* endPointer = &stop[skip];
	auto atLocation = reinterpret_cast<PointerIntPair<void*, 1>*>(endPointer);
	
	// Pointer heresy directly based off what LLVM does.
	// (http://llvm.org/docs/doxygen/html/Use_8cpp_source.html#l00041)
	if (atLocation->getInt() == 0)
	{
		// aligned pointer; this is not a tagged pointer
		// (we're reading the User vtable)
		return make_pair(nullptr, reinterpret_cast<ExpressionUser*>(atLocation));
	}
	else
	{
		// tagged pointer, hung-off uses; the pointer in atLocation points to the user
		return make_pair(nullptr, reinterpret_cast<ExpressionUser*>(atLocation->getPointer()));
	}
}

ExpressionUser* ExpressionUse::getUser()
{
	ExpressionUse* use = this;
	ExpressionUser* user = nullptr;
	while (user == nullptr)
	{
		tie(use, user) = use->walkToEndOfArray();
	}
	return user;
}

void ExpressionUse::setUse(Expression *target)
{
	if (expression == target)
	{
		return;
	}
	
	if (expression != nullptr)
	{
		prev.getPointer()->setNext(next);
	}
	
	prev.setPointer(nullptr);
	expression = target;
	if (expression != nullptr)
	{
		setNext(expression->firstUse);
		expression->firstUse = this;
	}
}

ExpressionUse& ExpressionUser::getOperandUse(unsigned int index)
{
	ExpressionUse* result = nullptr;
	iterateUseArrays(reinterpret_cast<ExpressionUse*>(this), allocatedAndUsed.second, allocatedAndUsed.first, [&](ExpressionUse* begin, ExpressionUse* end)
	{
		ptrdiff_t count = end - begin;
		if (count >= index)
		{
			result = end - index;
			return false;
		}
		else
		{
			index -= count;
			return true;
		}
	});
	
	return *result;
}

unsigned ExpressionUser::operands_size() const
{
	unsigned count = 0;
	iterateUseArrays(reinterpret_cast<const ExpressionUse*>(this), allocatedAndUsed.second, allocatedAndUsed.first, [&](const ExpressionUse* begin, const ExpressionUse* end)
	{
		count += end - begin;
		return true;
	});
	return count;
}
