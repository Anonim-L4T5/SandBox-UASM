#include "parser.hpp"

void removeComments(vector<string> &rScript_, const char *singleLineCommentBegin_, const char *multilineCommentBegin_, const char *multilineCommentEnd_) {

	const bool allowMultilineComments = multilineCommentBegin_ != nullptr && multilineCommentEnd_ != nullptr;

	bool inMultilineComment = false;
	for (string &line : rScript_) {
		if (allowMultilineComments) {
			size_t firstCharIdx = line.find_first_not_of(" \t");
			if (firstCharIdx == string::npos) continue;

			if (strStartsWith(line, multilineCommentBegin_, firstCharIdx)) {
				inMultilineComment = true;
			}
			else if (strStartsWith(line, multilineCommentEnd_, firstCharIdx)) {
				inMultilineComment = false;
				line.clear();
				continue;
			}

			if (inMultilineComment) {
				line.clear();
				continue;
			}
		}
		
		removeComments(line, singleLineCommentBegin_);
	}
}

void getNextToken(const char *&rTokBeg_, const char *&rTokEnd_, const char *end_, const char *delim_, const char *range_) {
	
	// [ ] token
	while (true) {
		if (rTokBeg_ == end_) {
			rTokEnd_ = rTokBeg_;
			return;
		}
		
		if (isOneOf(*rTokBeg_, delim_))
			rTokBeg_++;
		else
			break;
	}
	// [t]oken

	for (rTokEnd_ = rTokBeg_; rTokEnd_ != end_; rTokEnd_++) {

		// ["]some(string)"
		if (*rTokEnd_ == '"') {
			do rTokEnd_++; while (*rTokEnd_ != '"' && rTokEnd_ != end_);
			if (rTokEnd_ == end_) return;
		}
		// "some(string)"[ ]

		// hello[<]wor>ld
		{
			char beginOfRange = *rTokEnd_;
			char endOfRange = '\0';

			for (const char *r = range_; *r != '\0' && *(r + 1) != '\0'; r++) {
				if (*rTokEnd_ == *r) {
					endOfRange = *(r + 1);
					break;
				}
			}

			if (endOfRange != '\0') {
				size_t rangeDepth = 0;

				do {
					if (*rTokEnd_ == beginOfRange) rangeDepth++;
					else if (*rTokEnd_ == endOfRange) rangeDepth--;
					if (++rTokEnd_ == end_) return;
				} while (rangeDepth != 0);
			}
		}
		// hello<wor>[l]d

		if (rTokEnd_ == end_ || isOneOf(*rTokEnd_, delim_)) return;
	}
}

vector<string> split(const char *begin_, const char *end_, const char *delim_, const char *range_, const char *skip_, const bool removeEmpty_, const char *trimRangeIndictators_) {
	const char *begin = begin_, *last;
	vector<string> result;
	while (true) {
		getNextToken(begin, last, end_, delim_, range_);
		if (begin == end_) return result;

		string newTok;
		const char *tokenBegin = begin, *tokenEnd = last;
		begin = last;

		if (removeEmpty_ && tokenBegin == tokenEnd) continue;
			
		if (tokenEnd - tokenBegin > 1) {
			if (*tokenBegin == '"' && *(tokenEnd - 1) == '"') { // here we keep the spaces...
				tokenBegin++;
				tokenEnd--;
			}

			for (const char *r = trimRangeIndictators_; *r != '\0' && *(r + 1) != '\0'; r++) { // ...and here we don't
				if (*tokenBegin == *r && *(tokenEnd - 1) == *(r + 1)) { // { { ABC } } will turn into { ABC } ... Idrk if there will be any use for this
					tokenBegin++;
					tokenEnd--;
					trimWhitespacesPtr(tokenBegin, tokenEnd);
					break;
				}
			}
		}
		
		for (const char *c = tokenBegin; c < tokenEnd; c++)
			if (!isOneOf(*c, skip_)) newTok += *c;
		result.push_back(newTok);		
	}
}

static size_t replaceAll(string &rStr_, const string &macro_, const string &value_, bool onlySeparateTokens_ = true) {

	if (macro_.empty()) return 0;
	size_t replaced = 0;

	size_t startPos = 0;
	while ((startPos = rStr_.find(macro_, startPos)) != string::npos) {

		if (
			onlySeparateTokens_ == false ||
			((startPos == 0 || !isPartOfName(rStr_[startPos - 1])) &&
			(startPos + macro_.size() == rStr_.size() || !isPartOfName(rStr_[startPos + macro_.size()])))
		) {
			rStr_.replace(startPos, macro_.size(), value_);
			replaced++;
		}
		startPos += value_.size();
	}

	return replaced;
}

static size_t replaceMacroDefinitionChecks(string &rStr_, const MacroMap &globalMacros_, const MacroMap &localMacros_) {
	size_t count = 0;
	size_t pos = 0;
	while (true) {
		
		pos = rStr_.find("def:", pos);
		if (pos == string::npos) break;
		if (pos != 0 && isPartOfName(rStr_[pos - 1])) continue;

		size_t macroNamePos = pos + (sizeof("def:") - 1);
		size_t macroNameLenght = getNameLength(rStr_, macroNamePos);
		string macroName(rStr_.data() + macroNamePos, macroNameLenght);

		const string &isDefinedStr = isMacroDefined(macroName, globalMacros_, localMacros_) ? "true" : "false";
		rStr_.replace(rStr_.begin() + pos, rStr_.begin() + macroNamePos + macroNameLenght, isDefinedStr);
		count++;
	}

	return count;
}

size_t replaceMacros(string &rStr_, const MacroMap &globalMacros_, const MacroMap &localMacros_, size_t maxIterationNumber_) {
	size_t count;
	for (count = 0; count < maxIterationNumber_; count++) {
		
		replaceMacroDefinitionChecks(rStr_, globalMacros_, localMacros_);

		size_t replaced = 0;
		for (auto &[name, value] : localMacros_) {
			// We don't call replaceMacroDefinitionChecks after each replacement because macros can't contain def:-s
			replaced += replaceAll(rStr_, name, value);
		}

		for (auto &[name, value] : globalMacros_) {
			if (localMacros_.find(name) != localMacros_.end()) continue;
			replaced += replaceAll(rStr_, name, value);
		}

		if (replaced == 0) break;
	}

	return count;
}

bool Expression::parse(const char *begin_, const char *end_, const bool allowEquations_) {

	type = Expression::Type::Invalid;
	i = 0;

	bool negate = *begin_ == '!';
	if (negate) begin_++;

	int size = end_ - begin_;
	if (size < 1) return 0;

	if (std::equal(begin_, end_, "true")) {
		type = Expression::Type::Integer;
		i = 1 ^ negate;
		return 1;
	}

	if (std::equal(begin_, end_, "false")) {
		type = Expression::Type::Integer;
		i = 0 ^ negate;
		return 1;
	}

	if (allowEquations_ && size > 2 && *begin_ == '(' && *(end_ - 1) == ')') {

		vector<string> vec = split(begin_ + 1, end_ - 1, " \t", "()[]", "", true, "{}");

		// The number of tokens need to be odd --->   [ 1 ]  [ - 3 ]  [ + 5 ]  [ * 8 ]
		if (vec.empty() || (vec.size() - 1) % 2 != 0) return 0;

		*this = vec.front();

		for (int s = 1; s < vec.size(); s += 2) {
			operation(getOperEnum(vec[s]), vec[s + 1]);
			if (!valid()) return 0;
		}
	}
	else {

		size_t dotPos = string::npos;
		for (auto d = begin_; d < end_; d++)
			if (*d == '.') {
				dotPos = d - begin_;
				break;
			}

		if (dotPos != string::npos) {
			std::from_chars_result result = std::from_chars(begin_, end_, f);

			if (result.ec != std::errc{}) return 0;
			if (result.ptr == end_ - 1 && *result.ptr == '%') f /= 100.f;
			else if (result.ptr != end_) return 0;

			type = Expression::Type::Float;
		}
		else {
			std::from_chars_result result = std::from_chars(begin_, end_, i);

			if (result.ec != std::errc{}) return 0;
			if (result.ptr == end_ - 1 && *result.ptr == '%') i /= 100;
			else if (result.ptr != end_) return 0;

			type = Expression::Type::Integer;
		}
	}

	if (negate && valid()) {
		type = Expression::Type::Integer;
		i = zero();
	}
	return 1;
}

string Expression::toString(bool removeTrailingZeros_, int precision_) const {
	switch (type) {
	case Expression::Type::Integer: return numToStr(i, std::chars_format::fixed, removeTrailingZeros_);
	case Expression::Type::Float: return numToStr(f, std::chars_format::fixed, removeTrailingZeros_);
	default: return {};
	}
}

float Expression::getFloat() const {
	switch (type) {
	case Expression::Type::Integer: return (float)i;
	case Expression::Type::Float: return f;
	default: return NAN;
	}
}

int Expression::getInt() const {
	switch (type) {
	case Expression::Type::Integer: return i;
	case Expression::Type::Float: return (int)f;
	default: return 0;
	}
}

bool Expression::nonZero() const {
	switch (type) {
	case Expression::Type::Float: return f != 0.;
	case Expression::Type::Integer: return i != 0;
	default: return true;
	}
}

Expression &Expression::convert(Expression::Type type_) {
	if (type == type_) return *this;

	switch (type_) {
	case Expression::Type::Float: return *this = i;
	case Expression::Type::Integer: return *this = f;
	default: throw;
	}
}

Expression Expression::operation(MathOper oper_, const Expression &b_) {
	if (type == Expression::Type::Invalid || b_.type == Expression::Type::Invalid) return *this = Expression();

	if (type == Expression::Type::Integer && b_.type == Expression::Type::Integer)
		return *this = calculate(getInt(), oper_, b_.getInt());
	else
		return *this = calculate(getFloat(), oper_, b_.getFloat());
}


Expression Expression::operation(const Expression &a_, MathOper oper_, const Expression &b_) {
	if (a_.type == Expression::Type::Invalid || b_.type == Expression::Type::Invalid) return Expression();

	if (a_.type == Expression::Type::Integer && b_.type == Expression::Type::Integer)
		return calculate(a_.getInt(), oper_, b_.getInt());
	else
		return calculate(a_.getFloat(), oper_, b_.getFloat());
}

template<typename T>
Expression Expression::calculate(T a_, MathOper oper_, T b_) {
	switch (oper_) {
	case Plus:					return Expression(a_ + b_);
	case Minus:					return Expression(a_ - b_);
	case Times:					return Expression(a_ * b_);
	case DividedBy:				return Expression(a_ / b_);
	case ToThePowerOf:			return Expression(pow((float)a_, (float)b_));

	case IsEqualTo:				return Expression(a_ == b_);
	case IsNotEqualTo:			return Expression(a_ != b_);
	case IsGreaterThan:			return Expression(a_ > b_);
	case IsLessThan:			return Expression(a_ < b_);
	case IsGreaterOrEqualTo:	return Expression(a_ >= b_);
	case IsLessOrEqualTo:		return Expression(a_ <= b_);

	case Or:					return Expression(((a_ != 0) || (b_ != 0)));
	case And:					return Expression(((a_ != 0) && (b_ != 0)));
	}

	if (std::is_integral_v<T>)
	switch (oper_) {
	case BinOr:				return Expression((int)a_ | (int)b_);
	case BinAnd:			return Expression((int)a_ & (int)b_);
	case BinXor:			return Expression((int)a_ ^ (int)b_);
	}

	return Expression();
}

const unordered_map<string, Expression::MathOper> Expression::operStrToEnum{
	{ "+", Plus },
	{ "-", Minus },
	{ "*", Times },
	{ "/", DividedBy },
	{ "%", Modulo },
	{ "**", ToThePowerOf },

	{ "==", IsEqualTo },
	{ "!=", IsNotEqualTo },
	{ ">", IsGreaterThan },
	{ "<", IsLessThan },
	{ ">=", IsGreaterOrEqualTo },
	{ "<=", IsLessOrEqualTo },

	{ "||", Or },
	{ "&&", And },

	{ "|", BinOr },
	{ "&", BinAnd },
	{ "^", BinXor }
};