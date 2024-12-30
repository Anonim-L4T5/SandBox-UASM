#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include "common.hpp"
#include "parser.hpp"

Result assembleInstruction(const vector<string> &params_, uint64_t &rInst_, size_t instructionBits_ = compiledInstructionLengthBits);

#endif