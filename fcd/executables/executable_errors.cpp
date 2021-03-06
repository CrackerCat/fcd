//
// executable_errors.cpp
// Copyright (C) 2015 Félix Cloutier.
// All Rights Reserved.
//
// This file is distributed under the University of Illinois Open Source
// license. See LICENSE.md for details.
//

#include "executable_errors.h"

using namespace std;

namespace
{
	template<typename T, size_t N>
	constexpr size_t countof(T (&)[N])
	{
		return N;
	}
	
	ExecutableParsingErrorCategory instance;
	
#define ERROR_MESSAGE(code, message) [static_cast<size_t>(ExecutableParsingError::code)] = message
	const char* messageTable[] = {
		ERROR_MESSAGE(Generic_NoError, "no error"),
		ERROR_MESSAGE(Generic_UnknownFormat, "unknown executable format (see --help for supported formats)"),
		
		ERROR_MESSAGE(Elf_Corrupted, "ELF file fatally corrupted"),
		ERROR_MESSAGE(Elf_EndianMismatch, "fixme: ELF parser requires executable to use host endianness"),
		
		ERROR_MESSAGE(FlatBin_EntryPointOutOfRange, "entry address points outside of program memory"),
	};
	
	static_assert(countof(messageTable) == static_cast<size_t>(ExecutableParsingError::Generic_ErrorMax), "missing error strings");
}

ExecutableParsingErrorCategory& ExecutableParsingErrorCategory::instance()
{
	return ::instance;
}

error_code make_error_code(ExecutableParsingError error)
{
	return error_code((int)error, ExecutableParsingErrorCategory::instance());
}

const char* ExecutableParsingErrorCategory::name() const noexcept
{
	return "Executable Parsing Error";
}

string ExecutableParsingErrorCategory::message(int condition) const
{
	if (condition >= static_cast<int>(ExecutableParsingError::Generic_ErrorMax))
	{
		return "unknown error";
	}
	
	return messageTable[condition];
}
