# SandBox-UASM
Universal program assembler for Sand:Box CPUs


## Syntax

Each line starts with an instruction (`_`), a compiler command (`%`), or a macro.
 
Double quotes or braces can be used to pass tokens containing whitespaces.
 
The `\` symbol can be used to connect multiple lines in the script.\
Multiple tokens can be connected using the `#` character.

Single-line comments are marked with `$$`, and multi-line comments are enclosed between `$v` and `$^`.

When using a compiler command, an expression can be evaluated by using parentheses `()`.\
Supported operators include `+`, `-`, `*`, `/`, `**`, `==`, `!=`, `>`, `<`, `>=`, `<=`, `||`, `&&`, `|`, `&`, `^`, and `!`.

Prefixes can be used to get information about macros or retrive parts of a label's address:\
  `def:` Checks if the specified macro is defined.\
  `lo:` Retrieves the low byte of the label's address.\
  `hi:` Retrieves the high byte of the label's address.

The language is not case-sensitive, so e.g. `MOV R4 R7`, `mov r4 r7` and `mOv R4 R7` will be assembled into the same instruction.\
It is also important to use only lowercase file names, in order to make it possible to include them in the project.

## Compiler commands

Compiler commands can be used to control the compilation process and manage different aspects of code assembly.

`define [global] <macro> [value]`\
  Defines a macro. Local macros are automatically undefined when exiting the file's scope, whereas global macros persist until explicitly undefined or throughout the entire compilation process.
	
`undef [global] <macro>`\
  Undefines the specified macro. If the "global" keyword is used, it will undefine a global macro; otherwise, it defaults to undefining a local macro.

`include <file> [params...]`\
  Includes the content of the specified file. Additional parameters can be passed to customize the inclusion, which will be defined as local macros for the duration of file processing.
  The "<" and ">" symbols can be used to specify the full path to file, with the root being the working directory from which the compilation was started.

`jump <label> [cond]`\
  Jumps to the specified label in the code. An optional condition can be provided, making the jump conditional based on that criterion.
  The command searches for the label in a downwards direction. If the label is not found, the search is restarted from the top of the file.

`marker <text>`\
  Adds a marker to the compiled code, which can help with debugging and identyfying the locations of selected code fragments.

`error [code] [description]`\
  Generates an error with the specified code. This can be used to deliberately trigger errors for debugging or validation purposes.

`file_push <path> [params...]`\
  Pushes a new file onto the stack for processing. The file's path and optional parameters can be provided. Each file push isolates macros and other settings within its scope.\
  **However, it's important to remember that manually managing the file stack can lead to many potential errors, and it is strongly recommended to use `%include` command instead.**

`file_pop`
  Pops the most recently pushed file off the stack. This command exits the current file and returns to processing the previous file.


## IRN instruction format

Instructions can be manually constructed using the Integer-Register-Null format.
The instructions need to start with `_` symbol, followed with the list of expected tokens.
Each token is specified by a letter: `i` for integer, `r` for register, and `n` for null/empty, along with the number of bits it occupies in the generated instruction.
If the provided tokens do not match the expected format, an appriopriate error is returned.

Here are some examples:

`_i4r4r4r4 5 R2 R6 R8`\
  Compiles into `0x5268`

`_i5i10 0b10001 0b10110111`\
  Turns into `0x896E` (`0b10001 0010110111 0`)

`_i1n5r3 1 R4`\
  Gets transformed into `0x8200` (`0b1 00000 100 0000000`)

`_i1r2 R5 R2`\
  Returns `UnexpectedToken` error

`_i4 0b11111`\
  Returns `InvalidValue` error
