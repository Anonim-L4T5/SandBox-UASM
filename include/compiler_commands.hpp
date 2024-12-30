#ifndef COMPILER_COMMANDS_HPP
#define COMPILER_COMMANDS_HPP

#include "common.hpp"
#include "files.hpp"
#include "parser.hpp"

inline bool isMacroDefined(const string &str_, const MacroMap &rGlobalMacros_, const MacroMap &rLocalMacros_) {
	return rLocalMacros_.find(str_) != rLocalMacros_.end() || rGlobalMacros_.find(str_) != rGlobalMacros_.end();
}
const string &getMacroValue(const string &str_, const MacroMap &rGlobalMacros_, const MacroMap &rLocalMacros_, bool *pDefined_ = nullptr);
Result defineMacro(const vector<string> &line, MacroMap &rGlobalMacros_, MacroMap &rLocalMacros_);
Result undefMacro(const vector<string> &line, MacroMap &rGlobalMacros_, MacroMap &rLocalMacros_);
Result jumpToLabelIf(const vector<string> &line_, const vector<string> &script_, int &rGlobalLineIdx_, int &rLocalLineIdx_, const MacroMap &rGlobalMacros_, const MacroMap &rLocalMacros_);
Result includeFile(vector<string> line_, vector<string> &rScript_, int &rGlobalLineIdx_, vector<ProcessedFile> &rFiles_);


#endif