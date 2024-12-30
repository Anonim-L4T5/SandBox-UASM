#include "files.hpp"

bool readFile(vector<string> &rScript_, const string &fileName_, bool removeEmpty_) {

	rScript_.clear();

	std::ifstream ifs(fileName_);
	if (!ifs.is_open()) {
		return 0;
	}

	std::string line;
	while (std::getline(ifs, line)) {
		if (!line.empty() || !removeEmpty_)
			rScript_.push_back(line);
	}

	return 1;
}

bool saveCode(const vector<Instruction> &code_, const string &fileName_, size_t groups_, bool splitInstructions_, const vector<Marker> &markers_) {

	int compiledInstructionLengthBytes = divceil(compiledInstructionLengthBits, 8);
	int compiledInstructionLengthHexDigits = divceil(compiledInstructionLengthBits, 4);

	std::ofstream ofs(fileName_, std::ios::trunc | std::ios::binary);
	if (!ofs.is_open()) return 0;

	vector<char> outStr;
	outStr.reserve((compiledInstructionLengthBytes + splitInstructions_ + 1) * code_.size());

	size_t lastMarkerIdx = 0;

	for (size_t i = 0; i < code_.size(); i++) {

		constexpr char num4b[]{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

		if (lastMarkerIdx != markers_.size() && i == markers_[lastMarkerIdx].pos) {
			string str = "<" + markers_[lastMarkerIdx].str + "> ";
			outStr.insert(outStr.end(), str.begin(), str.end());
			lastMarkerIdx++;
		}

		Instruction inst = code_[i];
		for (int j = 0; j < compiledInstructionLengthHexDigits; j++) {
			outStr.push_back(num4b[inst >> (sizeof(inst) * 8 - 4)]);
			inst <<= 4;

			if (splitInstructions_ && (j % 2 != 0))
				outStr.push_back(' ');
		}

		if (!splitInstructions_)
			outStr.push_back(' ');

		if ((i + 1) % groups_ == 0) outStr.push_back('\n');
	}

	ofs.write(outStr.data(), outStr.size());

	return 1;
}