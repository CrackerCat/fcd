//
// pass_reguse.h
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

#ifndef pass_reguse_h
#define pass_reguse_h

SILENCE_LLVM_WARNINGS_BEGIN()
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Pass.h>
SILENCE_LLVM_WARNINGS_END()

class RegisterUse : public llvm::ImmutablePass, public llvm::AliasAnalysis
{
	std::unordered_map<const llvm::Function*, std::unordered_map<const char*, ModRefResult>> registerUse;
	
public:
	static char ID;
	
	RegisterUse();
	RegisterUse(const RegisterUse& that);
	
	virtual bool doInitialization(llvm::Module& m) override;
	virtual const char* getPassName() const override;
	virtual void getAnalysisUsage(llvm::AnalysisUsage& au) const override;
	virtual void* getAdjustedAnalysisPointer(llvm::AnalysisID PI) override;
	virtual ModRefResult getModRefInfo(llvm::ImmutableCallSite cs, const llvm::MemoryLocation& location) override;
	
	std::unordered_map<const char*, ModRefResult>& getOrCreateModRefInfo(llvm::Function* fn);
	std::unordered_map<const char*, ModRefResult>* getModRefInfo(llvm::Function* fn);
	const std::unordered_map<const char*, ModRefResult>* getModRefInfo(llvm::Function* fn) const;
	ModRefResult getModRefInfo(llvm::Function* fn, const char* registerName) const;
	
	void dumpFn(const llvm::Function* fn) const;
};

namespace llvm
{
	void initializeRegisterUsePass(PassRegistry& PR);
}

#endif /* pass_reguse_h */
