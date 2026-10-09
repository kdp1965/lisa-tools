// cdb.h - SDCC's CDB debug information (sdcc --debug writes <program>.cdb)
// for the LISA tools: the source lines behind the code addresses, the
// functions, the globals and the locals with their stack offsets and
// types, and the structs' layouts.  Shared by lisa_sim (source-level
// stepping and printing on the simulator) and lisa_ide (the same on the
// chip).  The format, as SDCC writes it:
//
//   M:<module>
//   F:<scope>$<name>$<level>_<n>$<block>(<type>),<space>,<onstack>,<offset>,<isr>,<isrno>,<bank>
//   S:<scope>$<name>$<level>_<n>$<block>(<type>),<space>,<onstack>,<offset>[,[<regs>]]
//   T:F<module>$<struct>[({<offset>}S:S$<member>$...(<type>),Z,0,0)...]
//   L:C$<file>$<line>$<level>_<n>$<block>:<address>         a source line
//   L:A$<file>$<asmline>:<address>                          an assembly line
//   L:<scope>$<name>$<level>_<n>$<block>:<address>          a symbol's address
//   L:X<scope>$<name>$...:<address>                         a function's end
//
// <scope> is G (global), F<module> (file static) or L<module>.<function>
// (a local).  <type> is {<size>} then modifiers and a base, comma
// separated, the base with :S or :U: DA<n> array, DG generic pointer, DC
// code pointer, DD data pointer, DF function; SC char, SS short, SI int,
// SL long, SF float, SV void, ST<struct>, SB<bits> bit-field.  <space>
// is B (stack), E (data), C (code, a function), D (a constant in code
// space), I (a peripheral register), R (register), Z (struct member).
// A stack <offset> is relative to the stack pointer at the function's
// entry (see Cdb::local_address).
//
// The file's addresses are the linker's bytes; this reader converts them
// as it loads: a line's or function's address is the word the PC holds,
// a code-space constant's (space D) is the pointer value without its tag
// (its bytes are ldi/ret pairs at twice that word), data addresses stay.
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace lisa {

struct CdbType {
    int size = 0;                       // bytes
    std::vector<std::string> mods;      // outermost first: "DA150", "DG", "DC", "DD", "DF"
    std::string base;                   // "SC", "SI", "SL", "SS", "SF", "SV", "ST<name>", "SB<n>"
    bool is_unsigned = false;
    bool is_pointer() const { return !mods.empty() && mods[0][1] != 'A' && mods[0][1] != 'F'; }
    bool is_array() const { return !mods.empty() && mods[0][1] == 'A'; }
    bool is_function() const { return !mods.empty() && mods[0][1] == 'F'; }
    int array_len() const;              // of the outermost DA
    CdbType element() const;            // the type without the outermost modifier
    std::string describe() const;       // "unsigned char", "int *", "char [24]", "struct s"
};

struct CdbSymbol {
    std::string name;
    std::string function;               // "module.function" for a local, "" otherwise
    std::string module;                 // for a file static
    bool is_global = false;
    int level = 0, block = 0;
    CdbType type;
    char space = ' ';
    bool on_stack = false;
    int offset = 0;                     // stack offset at entry (space B)
    std::vector<std::string> regs;      // space R: the registers
    uint16_t addr = 0;
    bool has_addr = false;
};

struct CdbFunction {
    std::string name;                   // "function" or "module.function"
    std::string module;
    bool is_global = false;
    int level = 0, block = 0;
    CdbType type;
    uint16_t addr = 0, end = 0;
    bool has_addr = false, has_end = false;
};

struct CdbLine {
    std::string file;
    int line = 0, level = 0, block = 0;
    uint16_t addr = 0;
};

struct CdbStruct {
    std::string name, module;
    struct Member { std::string name; int offset; CdbType type; };
    std::vector<Member> members;
};

class Cdb {
public:
    bool load(const std::string& path, std::string* err = nullptr);
    bool loaded() const { return loaded_; }
    const std::string& path() const { return path_; }
    const std::string& dir() const { return dir_; }   // where the sources are looked for

    // lines
    const CdbLine* line_at(uint16_t addr) const;      // the line whose code covers addr
    const CdbLine* line_starting(uint16_t addr) const; // a line that starts exactly at addr
    std::vector<uint16_t> addrs_of_line(const std::string& file, int line) const;
    const CdbLine* next_line_with_code(const std::string& file, int line) const;
    const std::vector<CdbLine>& lines() const { return lines_; }
    std::vector<std::string> files() const;
    // the text of a source line (1-based), "" if the file is not found
    std::string source(const std::string& file, int line) const;
    int source_lines(const std::string& file) const;    // 0 if not found
    std::string source_path(const std::string& file) const;   // where it was found, "" if not

    // functions and symbols
    const CdbFunction* function_at(uint16_t addr) const;
    const CdbFunction* function(const std::string& name) const;   // "main" or "module.main"
    std::vector<const CdbSymbol*> locals(const std::string& function) const;
    const CdbSymbol* global(const std::string& name) const;        // globals and file statics
    const CdbStruct* struct_named(const std::string& name) const;
    const std::vector<CdbSymbol>& symbols() const { return symbols_; }
    const std::vector<CdbFunction>& functions() const { return functions_; }

    // where a stack local lives: entry_sp is the stack pointer at the
    // function's entry (before its sra), ra_saved whether it pushed RA.
    // The caller's arguments sit above the return address - above the
    // slot it reserves for a result wider than two bytes or a struct
    // (ret_slot bytes, see return_slot) - and the locals below it.
    static uint16_t local_address(const CdbSymbol& s, uint16_t entry_sp, bool ra_saved, int ret_slot = 0);
    int return_slot(const CdbFunction& f) const;

    // a value as text: reads data bytes through rd, code-space constants
    // (the ldi/ret pairs of a code pointer) through rdcode (the instruction
    // word at an address), both optional
    using ReadData = std::function<uint8_t(uint16_t)>;
    using ReadCode = std::function<uint16_t(uint16_t)>;
    std::string format(const CdbType& t, uint16_t addr, bool in_code, const ReadData& rd, const ReadCode& rdcode, int depth = 0) const;
    int type_size(const CdbType& t) const;              // a struct's from its members

    // An expression over the program's variables: a name, with *, &,
    // [index], .member and ->member, in the frame of the function halted
    // in (frame may be null: globals only).  The readers as for format,
    // plus one for the peripheral registers (space I).
    struct Frame {
        std::string function;       // as CdbFunction::name
        uint16_t entry_sp = 0x7fff;
        bool ra_saved = false;
        int ret_slot = 0;           // return_slot() of the function
        uint8_t a = 0;              // the register a byte argument arrives in
        bool a_valid = false;       // only in the innermost frame
    };
    struct Value {
        CdbType type;
        uint32_t addr = 0;          // of an lvalue (a pointer value for in_code)
        bool in_code = false;
        bool is_lvalue = false;
        uint32_t val = 0;           // of an rvalue: a register, an address taken
        char space = ' ';
    };
    bool evaluate(const std::string& expr, const Frame* frame, Value& out, std::string* err,
                  const ReadData& rd, const ReadData& rdperiph, const ReadCode& rdcode) const;
    // evaluate and format: "<value>  <type> at <where>"
    std::string print(const std::string& expr, const Frame* frame, std::string* err,
                      const ReadData& rd, const ReadData& rdperiph, const ReadCode& rdcode) const;
    // a variable of the frame (a local, else a global) as a Value
    bool variable(const std::string& name, const Frame* frame, Value& out) const;
    std::string format_value(const Value& v, const ReadData& rd, const ReadData& rdperiph, const ReadCode& rdcode) const;

    // ---- frames without a shadow call stack (the chip) ----
    // What a function's prologue has done to SP by the time pc is reached:
    // `sra` (2 bytes, RA saved) and `ads #-n` (its locals) are the whole of
    // it, so at a statement boundary SP = entry SP - displacement.
    struct Prologue { bool ra_saved = false; int displacement = 0; uint16_t end = 0; };
    Prologue prologue(const CdbFunction& f, uint16_t pc, const ReadCode& rdcode) const;
    // the bytes a caller pushes for f: its stack arguments and the result
    // slot (popped after the call)
    int arg_bytes(const CdbFunction& f) const;
    // The frames from the registers: the return address is in RA until a
    // function saves it (low byte at the entry SP, high byte below), the
    // caller's SP at its statement boundary is the callee's entry SP plus
    // the arguments.  Stops at code without a function.
    struct StackFrame {
        uint16_t pc;                    // frame 0: the PC; others: the return address into them
        const CdbFunction* function;
        uint16_t entry_sp;
        bool ra_saved;
        uint16_t ret_pc;                // where this frame returns to
    };
    std::vector<StackFrame> unwind(uint16_t pc, uint16_t sp, uint16_t ra, const ReadData& rd, const ReadCode& rdcode, int max = 32) const;

    // Where execution leaves the source line at pc, for a `next` that runs
    // to breakpoints instead of stepping: the line starts reachable from pc
    // without crossing another line start (a loop back to pc's own line
    // included), with calls stepped over - or, with `into`, the first line
    // of a callee that has lines - and ret_pc for a return out of the
    // function (0 if unknown).  `unknown` is set when a jmp ix or an
    // overlong scan leaves a path unaccounted for.
    struct Exits { std::vector<uint16_t> stops; bool unknown = false; };
    Exits line_exits(uint16_t pc, bool into, uint16_t ret_pc, const ReadCode& rdcode) const;
    // the first line of a function past its prologue, 0 if it has none
    uint16_t first_line_addr(const CdbFunction& f) const;

private:
    bool loaded_ = false;
    std::string path_, dir_;
    std::vector<CdbLine> lines_;        // sorted by address
    std::vector<CdbFunction> functions_;// sorted by address
    std::vector<CdbSymbol> symbols_;
    std::vector<CdbStruct> structs_;
    std::map<std::string, std::string> source_paths_;   // file -> path found
    mutable std::map<std::string, std::vector<std::string>> source_cache_;
    void finish();
};

CdbType parse_cdb_type(const std::string& s);   // "{2}DG,SC:U"

}  // namespace lisa
