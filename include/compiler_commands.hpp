#ifndef COMPILER_COMMANDS_HPP
#define COMPILER_COMMANDS_HPP

#include "common.hpp"
#include "files.hpp"
#include "parser.hpp"

inline bool isMacroDefined(const string &str_, const MacroMap &rGlobalMacros_, const MacroMap &rLocalMacros_) {
	return rLocalMacros_.find(str_) != rLocalMacros_.end() || rGlobalMacros_.find(str_) != rGlobalMacros_.end();
}
void getMacroValue(string& out,const string& str_, const  MacroMap& rGlobalMacros_, const fMacroArgMap& rGlobalFMacros_, const  MacroMap& rLocalMacros_, const fMacroArgMap& rLocalFMacros_, bool* pDefined_);
Result defineMacro(const vector<string> &line, MacroMap &rGlobalMacros_, fMacroArgMap &rGlobalFMacros_, MacroMap &rLocalMacros_, fMacroArgMap& rLocalFMacros_);
Result undefMacro(const vector<string>& line_, MacroMap& rGlobalMacros_, fMacroArgMap& rGlobalFMacros_, MacroMap& rLocalMacros_, fMacroArgMap& rLocalFMacros_);
Result jumpToLabelIf(const vector<string> &line_, const vector<string> &script_, int &rGlobalLineIdx_, int &rLocalLineIdx_, const MacroMap &rGlobalMacros_, const MacroMap &rLocalMacros_);
Result includeFile(vector<string> line_, vector<string> &rScript_, int &rGlobalLineIdx_, vector<ProcessedFile> &rFiles_);


#endif