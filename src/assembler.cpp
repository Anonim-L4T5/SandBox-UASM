#include "assembler.hpp"

struct ParamTemplate {
	ValueType type;
	size_t begin;
	size_t bits;
};

static bool parseParam(const char *&rpStr_, const char *pLast_, vector<ParamTemplate> &rParams_, size_t &rLastParamBegin_) {
	ParamTemplate param;

	switch (*rpStr_) {
	case 'i': param.type = ValueType::Number; break;
	case 'r': param.type = ValueType::Register; break;
	case 'n': param.type = ValueType::Invalid; break;
	default: return 0;
	}

	std::from_chars_result result = std::from_chars(rpStr_ + 1, pLast_, param.bits);

	if (result.ec != std::errc{}) return 0;
	rpStr_ = result.ptr;

	param.begin = rLastParamBegin_;
	rLastParamBegin_ += param.bits;

	if (param.type != ValueType::Invalid)
		rParams_.push_back(param);
	return 1;
}

static bool generateInstructionTemplate(const string &str_, vector<ParamTemplate> &rParams_) {
	if (str_.size() < 3) return 0;
	if (str_[0] != '_') return 0;

	const char *ptr = str_.data() + 1;
	const char *last = str_.data() + str_.size();

	size_t lastParamBegin = 0;

	while (ptr != last)
		if (!parseParam(ptr, last, rParams_, lastParamBegin)) return 0;

	return 1;
}

Result assembleInstruction(const vector<string> &line_, uint64_t &rInst_, size_t instructionBits_) {

	vector<ParamTemplate> params;
	
	if (!generateInstructionTemplate(line_[0], params)) {
		return { UnknownInstruction, line_[0] };
	}

	if (line_.size() - 1 != params.size()) {
		return { InvalidParameterCount, numToStr(params.size()) };
	}
	
	uint64_t inst = 0;
	
	for (int i = 0; i < line_.size() - 1; i++) {
		
		Param thisParam(line_[i + 1]);
		
		if (!thisParam.isValid() || thisParam.type != params[i].type) {
			return { UnexpectedToken, line_[i + 1] };
		}
		
		int64_t maxVal = (1LL << params[i].bits) - 1; // (1 << 8) - 1 = 255
		int64_t minVal = -(1LL << params[i].bits) / 2; // -(1 << 8) / 2 = -128
		if (thisParam.value < minVal || thisParam.value > maxVal) {
			return { InvalidValue, '[' + numToStr(minVal) + ", " + numToStr(maxVal) + ']' };
		}
		
		int offset = (int)sizeof(inst) * 8 - params[i].begin - params[i].bits;
		if (offset < 0) {
			return { InvalidValue };
		}
		inst |= (thisParam.value & maxVal) << offset;

		/*
						   [****] line_[i].value
			   [________________] inst

		  [****]                  line_[i].value >>= -(sizeof(inst) * 8)
			   [________________] inst

			   [****]             line_[i].value >>= params[i].bits
			   [________________] inst

					 [****]       line_[i].value >>= params[i].begin
			   [________________] inst
		*/
	}
	
	rInst_ = inst;

	return {};
}