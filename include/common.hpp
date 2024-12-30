#ifndef COMMON_HPP
#define COMMON_HPP

#include <stdio.h>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <charconv>

using std::vector;
using std::string;
using std::string_view;
using std::unordered_map;
using std::pair;

using Instruction = uint64_t;
using MacroMap = unordered_map<string, string>;

constexpr int divceil(int a, int b) { return a / b + (a % b != 0); }
constexpr int bitmask(int bits_) { return ~(~0U << bits_); }

constexpr size_t compiledInstructionLengthBits = 16U;

enum BytePart : bool { Low, High };

enum ErrorCode {
	Success, NoError = Success,
	UnknownInstruction,
	UnexpectedToken,
	InvalidParameterCount,
	LabelUsedButNotDefined,
	MultipleLabelDefinitions,
	FileNotFound,
	InvalidValue,
	ErrorDirective,
	ErrorCode_End
};

struct Result {
	ErrorCode code = NoError;
	string str = "";

	string getErrorMessage() {
		switch (code) {
		case UnknownInstruction:
			return "Unknown instruction: '" + str + "'.\n";

		case UnexpectedToken:
			return "Unexpected token: '" + str + "'";

		case InvalidParameterCount:
			return "Invalid parameter count. Expected number: " + str + ".\n";

		case LabelUsedButNotDefined:
			return "Label '" + str + "' used but not defined.\n";

		case MultipleLabelDefinitions:
			return "Label '" + str + "' defined multiple times.\n";

		case FileNotFound:
			return "File '" + str + "' not found.\n";

		case InvalidValue:
			if (!str.empty())
				return "Invalid range. Expected values: " + str + ".\n";
			else
				return "Value is too large.\n";

		case ErrorDirective:
			return str;

		default:
			return "";
		}
	}
};

#endif