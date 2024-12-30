#include "common.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include "files.hpp"
#include "compiler_commands.hpp"

struct Label {
	size_t address = 0;

	struct CompletionAddress {
		size_t instruction = ~0U;
		size_t srcOffset = 0;
		size_t destOffset = 0;
		size_t destWidth = 0;
	};

	vector<CompletionAddress> completionAddresses;
	bool defined = false;
};

#define returnError(code_, str_) do { result.code = code_; result.str = str_; goto end; } while (false)

static bool getArgument(int argc, char* argv[], size_t offset_, const string &prefix_, int *pVal_ = nullptr) { // TODO // It works, but make it work better.
	if (prefix_.empty()) return false;	
	const bool expectValue = pVal_ != nullptr;
	
	for (int i = offset_; i < argc; i++) {
		if (strStartsWith(argv[i], prefix_.c_str())) {
			if (expectValue) {
				if (argv[i][prefix_.size()] != '=' /*the string is null-terminated, so no UB*/) continue;
				if (!strToNum(argv[i] + prefix_.size() + expectValue, *pVal_)) continue;
			}
			
			return true;
		}
	}

	return false;
}

int main(int argc, char* argv[]) {

	if (argc < 3) {
		fputs(
			"Usage:\n"
			".exe <src> <dest> [--groups=8] [-w] [-m]\n"
			"src    - Source file\n"
			"dest   - Destination file\n"
			"groups - Number of instructions per line\n"
			"w      - Don't split instructions into bytes\n"
			"m      - Don't add markers to the outputted code\n"
			"f      - Disable error forwarding\n",
			stdout
		);
		return -1;
	}
	
	int instructionGroups = 8;
	bool splitInstructions = true;
	bool removeMarkers = false;
	bool enableErrorForwarding = true;

	if (getArgument(argc, argv, 3, "-w")) { splitInstructions = false; }
	if (getArgument(argc, argv, 3, "-m")) { removeMarkers = true; }
	if (getArgument(argc, argv, 3, "-f")) { enableErrorForwarding = false; }
	if (getArgument(argc, argv, 3, "--groups", &instructionGroups)) {}

	size_t compiledInstructionLengthBytes = divceil(compiledInstructionLengthBits, 8);

	vector<string> script;
	vector<Instruction> code;
	unordered_map<string, Label> labels;
	MacroMap globalMacros;
	vector<Marker> markers;
	vector<ProcessedFile> files;
	Result result = {};

	if (!readFile(script, argv[1])) {
		fputs("Error: Unable to open file.\n", stdout);
		return -2;
	}

	{
		ProcessedFile mainFile;
		mainFile.location = (string)argv[1];
		mainFile.line = 0;
		files.push_back(mainFile);
	}
	
	
	prepareScript(script);
	
	for (int l = 0; l < script.size(); l++, files.back().line++) {
		if (script[l].empty()) continue;

		if (script[l].front() == '.') continue; // preprocessor label

		if (script[l].front() != '%') {
			replaceMacros(script[l], globalMacros, files.back().macros);
		}
		else {
			replaceMacrosProperties(script[l], globalMacros, files.back().macros);
		}
		
		vector<string> line = split(script[l].data(), script[l].data() + script[l].size());
		if (line.empty()) continue; // some lines may include only empty tokens (e.g.    "  " {} $$ comment   or sth like that)

		if (line[0].front() == '%') {

			if (line[0] == "%define") { // define [global] <macro> [value]
				result = defineMacro(line, globalMacros, files.back().macros);
			}
			else if (line[0] == "%undef") { // undef [global] <macro>
				result = undefMacro(line, globalMacros, files.back().macros);
			}
			else if (line[0] == "%include") { // include <file> [params...]
				result = includeFile(line, script, l, files);
			}
			else if (line[0] == "%jump") { // jump <label> [cond]
				result = jumpToLabelIf(line, script, l, files.back().line, globalMacros, files.back().macros);
			}
			else if (line[0] == "%file_push") { // file_push <path> [params...]
				if (line.size() < 2) returnError(InvalidParameterCount, "1 or more");
				
				ProcessedFile newFile;
				newFile.line = -1; // We do l++ at the beginning of the next loop iteration
				newFile.location = line[1];

				for (size_t i = 2; i < line.size(); i++)
					newFile.macros["%param" + numToStr(i - 2)] = line[i];
				newFile.macros["%paramcnt"] = numToStr(line.size() - 2);
				newFile.macros["%path"] = newFile.location.path;

				files.push_back(newFile);
			}
			else if (line[0] == "%file_pop") { // file_pop
				if (line.size() != 1) returnError(InvalidParameterCount, "0");
				if (!files.empty()) files.pop_back();
			}
			else if (line[0] == "%marker") { // marker <text>
				if (line.size() != 2) returnError(InvalidParameterCount, "1");
				markers.push_back({ line[1], code.size() });
			}
			else if (line[0] == "%error") { // error [code]
				if (line.size() == 2) {
					returnError(ErrorDirective, line[1]);
				}
				else if (line.size() == 3) {
					int err;
					if (!strToNum(line[1], err)) returnError(UnexpectedToken, line[1]);
					if (err < 0 || err >= ErrorCode_End) err = ErrorDirective;
					returnError((ErrorCode)err, line[2]);
				}
				else returnError(InvalidParameterCount, "1 or 2");
				
			}
			else returnError(UnknownInstruction, line[0]);

			if (result.code != NoError) goto end;
			continue;
		}
		
		if (line[0].back() == ':') {
			size_t labelAddress = code.size() * compiledInstructionLengthBytes;
			string labelName = line[0].substr(0, line[0].size() - 1);
			if (!isNameValid(labelName)) returnError(UnexpectedToken, line[0]);

			Label &label = labels[labelName];
			if (label.defined)
				returnError(MultipleLabelDefinitions, labelName);

			label.defined = true;
			label.address = labelAddress;

			for (auto &cmpl : label.completionAddresses)
				code[cmpl.instruction] |= (Instruction)((label.address >> cmpl.srcOffset) & bitmask(cmpl.destWidth)) << (sizeof(Instruction) * 8 - cmpl.destOffset);
			
			continue;
		}

		for (int i = 1; i < line.size(); i++) {
			if (line[i].size() < 4) continue;
			
			size_t srcOffset;
			if (strStartsWith(line[i], "lo:")) srcOffset = 0;
			else if (strStartsWith(line[i], "hi:")) srcOffset = 8;
			else continue;

			Label &label = labels[line[i].substr(3)];
			if (label.defined) {
				line[i] = numToStr((label.address >> srcOffset) & bitmask(8));
			}
			else {
				line[i] = "0";

				Label::CompletionAddress cmpl;
				cmpl.destWidth = 8;  // TODO // Make this work for other architectures
				cmpl.destOffset = 16;
				cmpl.srcOffset = srcOffset;
				cmpl.instruction = code.size();
				label.completionAddresses.push_back(cmpl);
			}
		}

		Instruction newInst;
		result = assembleInstruction(line, newInst);
		if (result.code != Success) goto end;
		code.push_back(newInst);

	}
	
	for (auto &[name, data] : labels)
		if (!data.defined) returnError(LabelUsedButNotDefined, name);
end:

	if (enableErrorForwarding)
	while (files.back().macros.find("forward_internal_errors") != files.back().macros.end() && files.size() > 1) files.pop_back();	

	if (result.code == Success) {
		string outStr =
			"Compilation complete!\n"
			"Program takes " + numToStr(code.size() * compiledInstructionLengthBytes) + " bytes (" + numToStr(code.size()) + " instructions) of memory.\n";

		fputs(outStr.c_str(), stdout);
		saveCode(code, argv[2], instructionGroups, splitInstructions, markers);
	}
	else {

		string errStr = 
			"file: " + files.back().location.name + ", "
			"line: " + numToStr(files.back().line + 1) + ", "
			"error: (" + numToStr((int)result.code) + ") "
			+ result.getErrorMessage();

		fputs(errStr.c_str(), stdout);
	}

	return result.code;
}