#ifndef PARSER_HPP
#define PARSER_HPP

#include "common.hpp"
#include "compiler_commands.hpp"

inline void trimWhitespaces(string &rStr_, const char *whitespaces_ = " \t") {
	rStr_.erase(rStr_.find_last_not_of(whitespaces_) + 1);
	rStr_.erase(0, rStr_.find_first_not_of(whitespaces_));
}

inline void trimWhitespacesPtr(const char *&rBegin_, const char *&rEnd_) {
	while (*rBegin_ != '\0' && isspace(*rBegin_)) rBegin_++;
	while (rEnd_ > rBegin_ && *rEnd_ != '\0' && isspace(*rEnd_)) rEnd_--;
}

inline bool strStartsWith(const string &str_, const string &starts_, size_t off_ = 0) {
	if (starts_.size() > str_.size() + off_) return false;
	for (size_t i = 0; i < starts_.size(); i++)
		if (str_[i + off_] != starts_[i]) return false;
	return true;
}

inline bool removeBrackets(string &rStr_, char begin_, char end_) {
	if (rStr_.size() < 2 || rStr_.front() != begin_ || rStr_.back() != end_) return 0;
	rStr_.erase(0, 1); rStr_.pop_back();
	return 1;
}

inline void removeComments(string &rStr_, const char *commentBegin_ = "$$") {
	const size_t commentBegin = rStr_.find(commentBegin_);
	if (commentBegin != string::npos)
		rStr_.erase(commentBegin);
}

inline bool isPartOfName(char ch_) {
	return
		(ch_ >= 'a' && ch_ <= 'z') ||
		(ch_ >= 'A' && ch_ <= 'Z') ||
		(ch_ >= '0' && ch_ <= '9') ||
		ch_ == '_' || ch_ == '.' || ch_ == '%';
		// '.' for file extensions
		// '%' for compiler macros
}

inline bool isNameValid(const string &str_) {
	if (str_.empty()) return false;
	if (str_[0] >= '0' && str_[0] <= '9') return false;
	for (int i = 1; i < str_.size(); i++)
		if (!isPartOfName(str_[i]))  return false;
	return true;
}

inline size_t getNameLength(const string &str_, size_t off_) {
	size_t end = off_;
	while (end < str_.size() && isPartOfName(str_[end])) end++;
	return end - off_;
}

inline bool isOneOf(const char char_, const char *list_) {
	for (const char *c = list_; *c != '\0'; c++)
		if (char_ == *c) return true;
	return false;
}

void removeComments(vector<string> &rScript_, const char *singleLineCommentBegin_ = "$$", const char *multilineCommentBegin_ = "$vvv", const char *multilineCommentEnd_ = "$^^^");

void getNextToken(const char *&rTokBeg_, const char *&rTokEnd_, const char *end_, const char *delim_ = " \t", const char *range_ = "()[]{}<>");

vector<string> split(const char *begin_, const char *end_, const char *delim_ = " \t", const char *range_ = "()[]{}<>", const char *skip_ = "+", const bool removeEmpty_ = true, const char *trimRangeIndictators_ = "{}");

size_t replaceMacros(string &str_, const MacroMap &macros_, size_t maxIterationNumber_ = 256, const string &prefix_ = "", const string &suffix_ = "");

inline  size_t replaceMacros(string &rStr_, const MacroMap &globalMacros_, const MacroMap &localMacros_, size_t maxIterationNumber_ = 256, const string &prefix_ = "", const string &suffix_ = "") {
	return
		replaceMacros(rStr_, localMacros_, maxIterationNumber_, prefix_, suffix_) +
		replaceMacros(rStr_, globalMacros_, maxIterationNumber_, prefix_, suffix_);
}

void replaceMacrosProperties(string &rStr_, const MacroMap &globalMacros_, const MacroMap &localMacros_);

inline void prepareScript(vector<string> &rScript_) {
	removeComments(rScript_, "$$", "$vvv", "$^^^");
	for (string &s : rScript_) {
		trimWhitespaces(s);
		for (char &c : s) c = tolower(c);
	}

	for (int i = 0; i < rScript_.size() - 1; i++) {
		if (!rScript_[i].empty() && rScript_[i].back() == '\\') {
			rScript_[i].pop_back();
			rScript_[i].insert(rScript_[i].end(), rScript_[i + 1].begin(), rScript_[i + 1].end());
			rScript_[i + 1].clear(); i++;
		}
	}
};

inline size_t getFirstCharacterPos(const string &str_) {
	for (size_t i = 0; i < str_.size(); i++)
		if (!isspace(str_[i])) return i;
	return string::npos;
}

template <typename _Arr, typename _Cond>
inline size_t removeIf(_Arr &rArr_, _Cond cond_, size_t begin_ = 0, size_t end_ = ~0U) {

	if (end_ > rArr_.size()) end_ = rArr_.size();

	size_t p = begin_, offset = 0;
	while (true) {
		const size_t pos = p + offset;
		if (pos >= end_) break;

		if (cond_(rArr_[pos]))
			offset++;
		else
			rArr_[p++] = rArr_[pos];
	}

	rArr_.erase(rArr_.begin() + p, rArr_.begin() + end_);
	return offset;
}

template <typename T>
inline string numToStr(T val_, std::chars_format format_ = std::chars_format::fixed, int precision_ = 6, bool removeTrailingZeros_ = false) {
	char buf[64];

	std::to_chars_result result;
	if constexpr (std::is_floating_point_v<T>) {
		result = std::to_chars(buf, buf + sizeof(buf), val_, format_, precision_);
		if (removeTrailingZeros_ && format_ == std::chars_format::fixed) {
			while (*(result.ptr - 1) == '0') result.ptr--;
			if (*(result.ptr - 1) == '.') result.ptr--;
		}
	}
	else if constexpr (std::is_same_v<T, bool>) {
		return val_ ? "true" : "false";
	}
	else if constexpr (std::is_integral_v<T>) {
		result = std::to_chars(buf, buf + sizeof(buf), val_);
	}
	else {
		static_assert(false, "Invalid type passed to numToStr().");
	}

	return string(buf, result.ptr);
}

template <typename T>
bool strToNum(const string &str_, T &rVal_, size_t off_ = 0, bool allowPrefixes_ = true, std::chars_format fmt_ = std::chars_format::general) {

	T tmp;
	std::from_chars_result result;
	const char* begin = str_.data() + off_;
	const char* end = str_.data() + str_.size();

	if constexpr (std::is_integral_v<T>) {

		int base = 10;
		if (allowPrefixes_ && str_.size() > 2) {
			begin += 2;

			if (strStartsWith(str_, "0x")) base = 16;
			else if (strStartsWith(str_, "0b")) base = 2;
			else begin -= 2;
		}

		result = std::from_chars(begin, end, tmp, base);
	}
	else if constexpr (std::is_floating_point_v<T>) {
		result = std::from_chars(begin, end, tmp, fmt_);
	}
	else {
		static_assert(false, "Invalid type passed to strToNum().");
	}

	if (result.ptr != end || result.ec != std::errc{}) return 0;
	rVal_ = tmp;
	return 1;
}

enum ValueType { Register, Number, Invalid };

struct Param {
	string str;
	ValueType type;
	int64_t value;

	static constexpr int registerCount = 16;

	inline bool isValid() const {
		switch (type) {
		case Register: return value >= 0 && value < registerCount;
		case Number: return true;
		default: return false;
		}
	}

	Param() : type(Invalid), value(0) {}

	Param(const string &str_) : str(str_), type(Invalid), value(0) {

		if (str_.empty()) return;

		if (str_[0] == 'r') {
			if (str_.size() < 2) return;
			type = Register;
		}
		else type = Number;

		if (!strToNum(str_, value, type == Register, type == Number)) type = Invalid;
	}
};

struct Expression {
public:

	enum MathOper {
		Plus, Minus, Times, DividedBy, Modulo,
		ToThePowerOf,
		IsEqualTo, IsNotEqualTo, IsGreaterThan, IsLessThan, IsGreaterOrEqualTo, IsLessOrEqualTo,
		BinOr, BinAnd, BinXor, And, Or,
		InvalidOper
	};

	enum Type : unsigned char { Integer, Float, Invalid } type;

	Expression(const char *begin_, const char *end_, const bool allowEquations_ = true) {
		parse(begin_, end_, allowEquations_);
	}
	Expression(const string &str_, const bool allowEquations_ = true) {
		parse(str_.data(), str_.data() + str_.size(), allowEquations_);
	}

	Expression() : type(Expression::Type::Invalid), i(0) {}

	Expression(float f_) : type(Expression::Type::Float), f(f_) {}
	Expression(int i_) : type(Expression::Type::Integer), i(i_) {}

	Expression(const Expression &a_, const Expression &b_, MathOper &oper_) { *this = operation(a_, oper_, b_); }
	Expression(const Expression &a_, const Expression &b_, const string &oper_) { *this = operation(a_, getOperEnum(oper_), b_); }

	string toString(bool removeTrailingZeros_ = true, int precision_ = 6) const;
	float getFloat() const;
	int getInt() const;
	bool nonZero() const;
	bool zero() const { return !nonZero(); };
	Expression &convert(Expression::Type type_);
	inline bool valid() const { return type != Expression::Type::Invalid; };

	Expression operation(MathOper oper_, const Expression &b_);
	static Expression operation(const Expression &a_, MathOper oper_, const Expression &b_);

	static inline MathOper getOperEnum(const string &str_) {
		auto it = operStrToEnum.find(str_);
		if (it == operStrToEnum.end()) return MathOper::InvalidOper;
		return it->second;
	}

private:

	union {
		int i;
		float f;
	};

	static const unordered_map<string, MathOper> operStrToEnum;

	template<typename T>
	static Expression calculate(T a_, MathOper oper_, T b_);

	bool parse(const char *begin_, const char *end_, const bool allowEquations_ = true);

};

#endif
