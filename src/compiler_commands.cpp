#include "compiler_commands.hpp"

#include <iostream>

void getMacroValue(string &out, const string &str_, const MacroMap &rGlobalMacros_, const fMacroArgMap &rGlobalFMacros_, const MacroMap &rLocalMacros_, const fMacroArgMap &rLocalFMacros_, bool *pDefined_) {
	string macroName = str_;
	const size_t argBegin = str_.find('(');
	const size_t argEnd = str_.find(')');
	do {
		if (argBegin == string::npos || argEnd == string::npos) break;
		const auto argTextLen = argEnd - argBegin;
		if (argTextLen < 0) break;
		macroName = str_.substr(0, argBegin);
		const string argText = str_.substr(argBegin + 1, argTextLen - 1);
		vector<string> argVals = splitArgs(argText);
		auto fmacro = rLocalFMacros_.find(macroName);
		if (fmacro != rLocalFMacros_.end()) {
			auto fcount = argVals.size();
			auto ecount = fmacro->second.size();
			if (fcount!=ecount) {
				std::cout << "incorrect argument count (found " << fcount << ", expected " << ecount << "): ignoring all arguments\n";
				out = macroName;
				return;
			}
			string mval;
			getMacroValue(mval,macroName, rGlobalMacros_, rGlobalFMacros_, rLocalMacros_, rLocalFMacros_, pDefined_);
			for (int i = 0; i < ecount;i++) {
				replaceAllText(mval, "@" + fmacro->second.at(i), argVals.at(i));
			}
			out=mval;
			return;
		}
		fmacro = rGlobalFMacros_.find(macroName);
		if (fmacro != rGlobalFMacros_.end()) {
			auto fcount = argVals.size();
			auto ecount = fmacro->second.size();
			if (fcount != ecount) {
				std::cout << "incorrect argument count (found " << fcount << ", expected " << ecount << "): ignoring all arguments\n";
				out=str_;
				return;
			}
			string mval;
			getMacroValue(mval,macroName, rGlobalMacros_, rGlobalFMacros_, std::unordered_map<string, string>(), std::unordered_map<string, vector<string>>(), pDefined_);
			for (int i = 0; i < ecount; i++) {
				replaceAllText(mval, "$" + fmacro->second.at(i), argVals.at(i));
			}
			out=mval;
			return;
		}
	} while (0);
	
	auto macro = rLocalMacros_.find(macroName);
	if (macro != rLocalMacros_.end()) {
		if (pDefined_ != nullptr) *pDefined_ = true;
		out=macro->second;
		return;
	}

	macro = rGlobalMacros_.find(macroName);
	if (macro != rGlobalMacros_.end()) {
		if (pDefined_ != nullptr) *pDefined_ = true;
		out= macro->second;
		return;
	}

	if (pDefined_ != nullptr) *pDefined_ = false;
	out=macroName;
	return;
}

Result defineMacro(const vector<string> &line_, MacroMap &rGlobalMacros_, fMacroArgMap &rGlobalFMacros_, MacroMap &rLocalMacros_, fMacroArgMap& rLocalFMacros_) {

	if (line_.size() < 2) return { InvalidParameterCount, "1, 2 or 3" }; // We don't count the command

	bool global = line_[1] == "global";
	if (global ? (line_.size() != 3 && line_.size() != 4) : (line_.size() != 2 && line_.size() != 3))
		return { InvalidParameterCount, global ? "2 or 3" : "1 or 2" };

	const string &macroName = line_[global ? 2 : 1];

	const size_t argBegin = macroName.find('(');
	const size_t argEnd = macroName.find(')');
	do {
		if (argBegin==string::npos||argEnd==string::npos) break;
		const auto argTextLen = argEnd-argBegin;
		if (argTextLen<0) break;
		const string fmacroName = macroName.substr(0,argBegin);
		const string argText = macroName.substr(argBegin+1,argTextLen-1);
		vector<string> argNames = splitArgs(argText);

		string macroValue;
		if (global)
			macroValue = (line_.size() == 4 ? line_[3] : "");
		else
			macroValue = (line_.size() == 3 ? line_[2] : "");

		if (macroName.front() == '%') return { UnexpectedToken, fmacroName };

		(global ? rGlobalMacros_ : rLocalMacros_)[fmacroName] = macroValue;
		(global ? rGlobalFMacros_ : rLocalFMacros_)[fmacroName] = argNames;
		
		return {};
	} while(0);

	string macroValue;
	if (global)
		macroValue = (line_.size() == 4 ? line_[3] : "");
	else
		macroValue = (line_.size() == 3 ? line_[2] : "");

	if (macroName.front() == '%') return { UnexpectedToken, macroName };

	(global ? rGlobalMacros_ : rLocalMacros_)[macroName] = macroValue;

	return {};
}

Result undefMacro(const vector<string>& line_, MacroMap& rGlobalMacros_, fMacroArgMap& rGlobalFMacros_, MacroMap& rLocalMacros_, fMacroArgMap& rLocalFMacros_) {

	if (line_.size() == 2) {
		if (line_[1].front() == '%') return { UnexpectedToken, line_[1] };
		rLocalMacros_.erase(line_[1]);
		rLocalFMacros_.erase(line_[1]);
	}
	else if (line_.size() == 3) {
		if (line_[1] != "global") return { UnexpectedToken, line_[1] };
		if (line_[2].front() == '%') return { UnexpectedToken, line_[2] };
		rGlobalMacros_.erase(line_[2]);
		rGlobalFMacros_.erase(line_[2]);
	}
	else return { InvalidParameterCount, "1 or 2" };

	return {};
}

Result jumpToLabelIf(const vector<string> &line_, const vector<string> &script_, int &rGlobalLineIdx_, int &rLocalLineIdx_, const MacroMap &rGlobalMacros_, const MacroMap &rLocalMacros_) {

	if (line_.size() == 3) {
		Expression cond = line_[2];
		if (cond.valid() && cond.zero()) return {};		
	}
	else if (line_.size() != 2) return { InvalidParameterCount, "1 or 2" };
	
	int fileBeginLineIdx = rGlobalLineIdx_ - rLocalLineIdx_;

	for (int newLineIdx = rLocalLineIdx_ + 1; newLineIdx != rLocalLineIdx_; newLineIdx++) {
		
		const string &thisLine = script_[fileBeginLineIdx + newLineIdx];

		const char *begin = thisLine.data(), *end;
		getNextToken(begin, end, thisLine.data() + thisLine.size());
		if (begin == end) continue;

		if (*begin == '.' && std::equal(begin + 1, end, line_[1].data())) {
			rGlobalLineIdx_ += newLineIdx - rLocalLineIdx_;
			rLocalLineIdx_ = newLineIdx;
			return {};
		}
		else if (std::equal(begin, end, "%file_pop")) { // label was not found after the command - go to the begin of file and search above
			newLineIdx = -1; // we increment newLineIdx in the next iteration
		} // %file_pop is always added by the compiler, so we won't reach the end of script
	}

	return { LabelUsedButNotDefined, line_[1] };
}

Result includeFile(vector<string> line_, vector<string> &rScript_, int &rGlobalLineIdx_, vector<ProcessedFile> &rFiles_) {
	if (line_.size() < 2) return { InvalidParameterCount, "1 or more" };

	bool wholePath = removeBrackets(line_[1], '<', '>');
	string pathStr = wholePath ? line_[1] : rFiles_.back().location.path + line_[1];

	vector<string> includedScript;
	if (!readFile(includedScript, pathStr))
		return { FileNotFound, line_[1] };

	prepareScript(includedScript);

	{
		string filePushCommandStr = "%file_push \"" + pathStr + "\"";
		for (int i = 2; i < line_.size(); i++)
			filePushCommandStr += ' ' + line_[i];

		includedScript.insert(includedScript.begin(), filePushCommandStr);
		includedScript.push_back("%file_pop");
	}

	rScript_.erase(rScript_.begin() + rGlobalLineIdx_);
	rScript_.insert(rScript_.begin() + rGlobalLineIdx_, includedScript.begin(), includedScript.end());
	rGlobalLineIdx_--; // We go back one line, because the %inlcude at script[l] got replaced

	return {};
}