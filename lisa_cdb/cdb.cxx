// cdb.cxx - SDCC's CDB debug information for the LISA tools (see cdb.h)
#include "cdb.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

namespace lisa {

// ---- strings ---------------------------------------------------------

static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

static int to_int(const std::string& s) { return (int)strtol(s.c_str(), nullptr, 10); }
static uint16_t to_hex(const std::string& s) { return (uint16_t)strtoul(s.c_str(), nullptr, 16); }

// "<level>_<n>" -> level
static int level_of(const std::string& s) { return to_int(s); }

// ---- types -----------------------------------------------------------

CdbType parse_cdb_type(const std::string& s) {
    CdbType t;
    std::string rest = s;
    if (!rest.empty() && rest[0] == '{') {
        size_t e = rest.find('}');
        t.size = to_int(rest.substr(1, e - 1));
        rest = rest.substr(e + 1);
    }
    for (const std::string& item : split(rest, ',')) {
        if (item.empty()) continue;
        size_t colon = item.rfind(':');
        if (colon != std::string::npos && colon + 2 == item.size() && (item[colon + 1] == 'S' || item[colon + 1] == 'U')) {
            t.base = item.substr(0, colon);
            t.is_unsigned = item[colon + 1] == 'U';
        } else
            t.mods.push_back(item);
    }
    return t;
}

int CdbType::array_len() const {
    if (!is_array()) return 0;
    return to_int(mods[0].substr(2));           // "DA150d": the digits after DA
}

CdbType CdbType::element() const {
    CdbType e = *this;
    if (!e.mods.empty()) {
        if (e.is_array() && array_len() > 0) e.size = size / array_len();
        else e.size = 0;                        // a pointer's or function's target: the base's size is used
        e.mods.erase(e.mods.begin());
    }
    return e;
}

static int base_size(const std::string& base) {
    if (base == "SC" || base == "SV") return 1;
    if (base == "SI" || base == "SS") return 2;
    if (base == "SL" || base == "SF") return 4;
    return 0;
}

std::string CdbType::describe() const {
    std::string b;
    if (base == "SC") b = is_unsigned ? "unsigned char" : "char";
    else if (base == "SI") b = is_unsigned ? "unsigned int" : "int";
    else if (base == "SS") b = is_unsigned ? "unsigned short" : "short";
    else if (base == "SL") b = is_unsigned ? "unsigned long" : "long";
    else if (base == "SF") b = "float";
    else if (base == "SV") b = "void";
    else if (base.rfind("ST", 0) == 0) b = "struct " + base.substr(2);
    else if (base.rfind("SB", 0) == 0) b = "bit-field " + base.substr(2);
    else b = base;
    // the declarator, outermost modifier first: a pointer prefixes, an
    // array or function suffixes and parenthesizes a pointer it applies to
    std::string d;
    bool ptr = false;
    for (const std::string& m : mods) {
        char k = m.size() > 1 ? m[1] : ' ';
        if (k == 'A' || k == 'F') {
            if (ptr) d = "(" + d + ")";
            d += k == 'A' ? "[" + std::to_string(to_int(m.substr(2))) + "]" : "()";
            ptr = false;
        } else {
            d = std::string(k == 'C' ? "__code *" : "*") + d;
            ptr = true;
        }
    }
    return d.empty() ? b : b + " " + d;
}

// ---- loading ---------------------------------------------------------

bool Cdb::load(const std::string& path, std::string* err) {
    std::ifstream in(path);
    if (!in) {
        if (err) *err = "cannot open " + path;
        return false;
    }
    *this = Cdb();
    path_ = path;
    size_t slash = path.find_last_of('/');
    dir_ = slash == std::string::npos ? "." : path.substr(0, slash);

    std::map<std::string, std::pair<std::string, uint16_t>> ends;   // function end addresses
    std::vector<std::tuple<char, std::string, std::string, uint16_t>> addrs;  // scope, module/function, name, addr
    // The records come in assembly order, so a line record follows the
    // address record of its function and precedes that function's end;
    // the last one of a function sits after its last instruction - at the
    // next function's start - and is dropped below.
    std::string cur_fn;
    std::vector<std::string> line_owner;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.size() < 2 || line[1] != ':') continue;
        char kind = line[0];
        std::string rest = line.substr(2);

        if (kind == 'L') {
            size_t colon = rest.rfind(':');
            if (colon == std::string::npos) continue;
            uint16_t addr = to_hex(rest.substr(colon + 1));
            std::string head = rest.substr(0, colon);
            if (head.rfind("C$", 0) == 0) {                   // a source line
                auto f = split(head.substr(2), '$');
                if (f.size() >= 4) {
                    CdbLine l;
                    l.file = f[0]; l.line = to_int(f[1]); l.level = level_of(f[2]); l.block = to_int(f[3]); l.addr = addr;
                    lines_.push_back(l);
                    line_owner.push_back(cur_fn);
                }
            } else if (head.rfind("A$", 0) == 0) {
                ;                                               // an assembly line
            } else if (head.rfind("X", 0) == 0) {               // a function's end
                std::string h = head.substr(1);
                char scope = h[0];
                auto f = split(h.substr(1), '$');
                if (f.size() >= 2) {
                    std::string key = std::string(1, scope) + f[0] + "$" + f[1];
                    ends[key] = { f[0], addr };
                    if (key == cur_fn) cur_fn.clear();      // written after the next function's start
                }
            } else {                                            // a symbol's address
                char scope = head[0];
                auto f = split(head.substr(1), '$');
                if (f.size() >= 2) {
                    addrs.emplace_back(scope, f[0], f[1], addr);
                    if (scope != 'L') {
                        std::string key = std::string(1, scope) + f[0] + "$" + f[1];
                        for (const CdbFunction& fn : functions_)
                            if (key == (fn.is_global ? "G$" + fn.name : "F" + fn.module + "$" + fn.name.substr(fn.module.size() + 1))) {
                                cur_fn = key;
                                break;
                            }
                    }
                }
            }
            continue;
        }

        if (kind == 'F' || kind == 'S') {
            size_t paren = rest.find('(');
            if (paren == std::string::npos) continue;
            int depth = 0; size_t close = paren;
            for (size_t i = paren; i < rest.size(); i++) {
                if (rest[i] == '(') depth++;
                else if (rest[i] == ')' && --depth == 0) { close = i; break; }
            }
            std::string scope_part = rest.substr(0, paren);
            std::string type_str = rest.substr(paren + 1, close - paren - 1);
            auto fields = split(rest.substr(close + 1), ',');     // "", space, onstack, offset, ...
            char scope = scope_part[0];
            auto f = split(scope_part.substr(1), '$');          // [module|function], name, level, block
            if (f.size() < 4) continue;
            if (kind == 'F') {
                CdbFunction fn;
                fn.is_global = scope == 'G';
                fn.module = scope == 'G' ? "" : f[0];
                fn.name = scope == 'G' ? f[1] : f[0] + "." + f[1];
                fn.level = level_of(f[2]); fn.block = to_int(f[3]);
                fn.type = parse_cdb_type(type_str);
                functions_.push_back(fn);
            } else {
                CdbSymbol s;
                s.is_global = scope == 'G';
                if (scope == 'L') s.function = f[0];
                else if (scope == 'F') s.module = f[0];
                s.name = f[1];
                s.level = level_of(f[2]); s.block = to_int(f[3]);
                s.type = parse_cdb_type(type_str);
                if (fields.size() >= 4) {
                    s.space = fields[1].empty() ? ' ' : fields[1][0];
                    s.on_stack = to_int(fields[2]) != 0;
                    s.offset = to_int(fields[3]);
                }
                if (fields.size() >= 5) {                       // [a] / [] / [a,ix]
                    std::string r = fields[4];
                    for (size_t i = 5; i < fields.size(); i++) r += "," + fields[i];
                    if (!r.empty() && r[0] == '[') r = r.substr(1);
                    if (!r.empty() && r.back() == ']') r.pop_back();
                    for (const std::string& x : split(r, ',')) if (!x.empty()) s.regs.push_back(x);
                }
                symbols_.push_back(s);
            }
            continue;
        }

        if (kind == 'T') {                                      // T:F<module>$<name>[(member)...]
            size_t br = rest.find('[');
            if (br == std::string::npos || rest[0] != 'F') continue;
            auto f = split(rest.substr(1, br - 1), '$');
            if (f.size() < 2) continue;
            CdbStruct st;
            st.module = f[0]; st.name = f[1];
            std::string body = rest.substr(br + 1);
            size_t i = 0;
            while (i < body.size()) {
                if (body[i] != '(') { i++; continue; }
                int depth = 0; size_t j = i;
                for (; j < body.size(); j++) {
                    if (body[j] == '(') depth++;
                    else if (body[j] == ')' && --depth == 0) break;
                }
                std::string m = body.substr(i + 1, j - i - 1);   // {off}S:S$name$lvl$blk(type),Z,0,0
                size_t e = m.find('}');
                int off = to_int(m.substr(1, e - 1));
                size_t p = m.find('(', e);
                int d = 0; size_t c = p;
                for (size_t k = p; k < m.size(); k++) {
                    if (m[k] == '(') d++;
                    else if (m[k] == ')' && --d == 0) { c = k; break; }
                }
                auto mf = split(m.substr(e + 1, p - e - 1), '$');  // "S:S", name, lvl, blk
                CdbStruct::Member mem;
                mem.name = mf.size() > 1 ? mf[1] : "?";
                mem.offset = off;
                mem.type = parse_cdb_type(m.substr(p + 1, c - p - 1));
                st.members.push_back(mem);
                i = j + 1;
            }
            structs_.push_back(st);
            continue;
        }
    }

    // The linker's addresses are bytes: a code address is twice the word
    // (the PC), a code-space constant's is four times the pointer value
    // (its ldi/ret pairs, see gptrget.s); data addresses are bytes already.
    for (CdbLine& l : lines_) l.addr /= 2;
    for (const auto& [scope, owner, name, addr] : addrs) {
        bool done = false;
        for (CdbFunction& fn : functions_) {
            bool match = scope == 'G' ? (fn.is_global && fn.name == name)
                                      : (!fn.is_global && fn.module == owner && fn.name == owner + "." + name);
            if (match && !fn.has_addr) { fn.addr = addr / 2; fn.has_addr = true; done = true; break; }
        }
        if (done) continue;
        for (CdbSymbol& s : symbols_) {
            bool match = scope == 'G' ? (s.is_global && s.name == name)
                       : scope == 'F' ? (!s.is_global && s.module == owner && s.function.empty() && s.name == name)
                                      : (s.function == owner && s.name == name);
            if (match && !s.has_addr) {
                s.addr = s.space == 'D' ? addr / 4 : s.space == 'C' ? addr / 2 : addr;
                s.has_addr = true;
                break;
            }
        }
    }
    std::map<std::string, size_t> fn_by_key;
    for (size_t i = 0; i < functions_.size(); i++) {
        CdbFunction& fn = functions_[i];
        std::string key = (fn.is_global ? "G" : "F" + fn.module) + "$" + (fn.is_global ? fn.name : fn.name.substr(fn.module.size() + 1));
        fn_by_key[key] = i;
        auto it = ends.find(key);
        if (it != ends.end()) { fn.end = it->second.second / 2; fn.has_end = true; }
    }
    // SDCC labels a function's closing line after its last instruction
    // too, where the next function starts.  For a function written on one
    // line that label is the opening line's as well, and the assembler
    // keeps only the later definition: the record then stands for the
    // lost opening one and is moved to the function's start.  (All the
    // records of such a function share its line; a closing brace's line
    // has no other record.)
    auto trailing = [&](size_t i) {
        auto fi = fn_by_key.find(line_owner[i]);
        return fi != fn_by_key.end() && functions_[fi->second].has_end && lines_[i].addr >= functions_[fi->second].end;
    };
    std::vector<CdbLine> kept;
    for (size_t i = 0; i < lines_.size(); i++) {
        if (trailing(i)) {
            bool one_line = true;
            for (size_t j = 0; j < lines_.size() && one_line; j++)
                if (j != i && line_owner[j] == line_owner[i] && !trailing(j)
                    && (lines_[j].line != lines_[i].line || lines_[j].file != lines_[i].file))
                    one_line = false;
            if (!one_line) continue;
            lines_[i].addr = functions_[fn_by_key[line_owner[i]]].addr;
        }
        kept.push_back(lines_[i]);
    }
    lines_.swap(kept);
    finish();
    loaded_ = true;
    return true;
}

void Cdb::finish() {
    std::stable_sort(lines_.begin(), lines_.end(), [](const CdbLine& a, const CdbLine& b) { return a.addr < b.addr; });
    std::stable_sort(functions_.begin(), functions_.end(), [](const CdbFunction& a, const CdbFunction& b) { return a.addr < b.addr; });
    // the sources: beside the .cdb, or by their base name there
    for (const CdbLine& l : lines_) {
        if (source_paths_.count(l.file)) continue;
        std::string cand = dir_ + "/" + l.file;
        if (std::ifstream(cand)) { source_paths_[l.file] = cand; continue; }
        size_t slash = l.file.find_last_of('/');
        if (slash != std::string::npos) {
            cand = dir_ + "/" + l.file.substr(slash + 1);
            if (std::ifstream(cand)) { source_paths_[l.file] = cand; continue; }
        }
        source_paths_[l.file] = "";
    }
}

// ---- lookups ---------------------------------------------------------

// The last record at or before addr - of several at one address (a block's
// opening and its first statement) the last, the statement.  Only within a
// function that has lines: past the last C line lies assembly code.
const CdbLine* Cdb::line_at(uint16_t addr) const {
    auto it = std::upper_bound(lines_.begin(), lines_.end(), addr,
                               [](uint16_t a, const CdbLine& l) { return a < l.addr; });
    if (it == lines_.begin()) return nullptr;
    const CdbLine* l = &*(it - 1);
    const CdbFunction* f = function_at(addr);
    if (!f || l->addr < f->addr) return nullptr;
    return l;
}

const CdbLine* Cdb::line_starting(uint16_t addr) const {
    auto it = std::upper_bound(lines_.begin(), lines_.end(), addr,
                               [](uint16_t a, const CdbLine& l) { return a < l.addr; });
    if (it == lines_.begin() || (it - 1)->addr != addr) return nullptr;
    return &*(it - 1);
}

static bool same_file(const std::string& a, const std::string& b) {
    if (a == b) return true;
    auto base = [](const std::string& s) { size_t p = s.find_last_of('/'); return p == std::string::npos ? s : s.substr(p + 1); };
    return base(a) == base(b);
}

std::vector<uint16_t> Cdb::addrs_of_line(const std::string& file, int line) const {
    std::vector<uint16_t> out;
    for (const CdbLine& l : lines_)
        if (l.line == line && same_file(l.file, file) && (out.empty() || out.back() != l.addr))
            out.push_back(l.addr);
    return out;
}

const CdbLine* Cdb::next_line_with_code(const std::string& file, int line) const {
    const CdbLine* best = nullptr;
    for (const CdbLine& l : lines_)
        if (same_file(l.file, file) && l.line >= line && (!best || l.line < best->line))
            best = &l;
    return best;
}

std::vector<std::string> Cdb::files() const {
    std::vector<std::string> out;
    for (const auto& kv : source_paths_) out.push_back(kv.first);
    return out;
}

std::string Cdb::source(const std::string& file, int line) const {
    std::string key;
    for (const auto& kv : source_paths_) if (same_file(kv.first, file)) { key = kv.first; break; }
    if (key.empty() || source_paths_.at(key).empty()) return "";
    auto it = source_cache_.find(key);
    if (it == source_cache_.end()) {
        std::vector<std::string> v;
        std::ifstream in(source_paths_.at(key));
        std::string s;
        while (std::getline(in, s)) { while (!s.empty() && (s.back() == '\r')) s.pop_back(); v.push_back(s); }
        it = source_cache_.emplace(key, v).first;
    }
    if (line < 1 || line > (int)it->second.size()) return "";
    return it->second[line - 1];
}

int Cdb::source_lines(const std::string& file) const {
    source(file, 1);                                     // caches it
    for (const auto& kv : source_cache_) if (same_file(kv.first, file)) return (int)kv.second.size();
    return 0;
}

const CdbFunction* Cdb::function_at(uint16_t addr) const {
    // sorted by address, those without one (addr 0, has_addr false) first
    auto it = std::upper_bound(functions_.begin(), functions_.end(), addr,
                               [](uint16_t a, const CdbFunction& f) { return a < f.addr; });
    while (it != functions_.begin()) {
        --it;
        if (!it->has_addr) return nullptr;
        if (it->has_end && addr >= it->end) return nullptr;
        return &*it;
    }
    return nullptr;
}

const CdbFunction* Cdb::function(const std::string& name) const {
    for (const CdbFunction& f : functions_)
        if (f.name == name || (!f.is_global && f.name.substr(f.module.size() + 1) == name))
            return &f;
    return nullptr;
}

// a local's function is "module.function"; a global function's name has
// no module
static bool in_function(const CdbSymbol& s, const std::string& function) {
    if (s.function.empty() || function.empty()) return false;
    if (s.function == function) return true;
    size_t n = function.size();
    return s.function.size() > n + 1 && s.function[s.function.size() - n - 1] == '.'
        && s.function.compare(s.function.size() - n, n, function) == 0;
}

std::vector<const CdbSymbol*> Cdb::locals(const std::string& function) const {
    std::vector<const CdbSymbol*> out;
    for (const CdbSymbol& s : symbols_)
        if (in_function(s, function)) out.push_back(&s);
    return out;
}

const CdbSymbol* Cdb::global(const std::string& name) const {
    const CdbSymbol* best = nullptr;
    for (const CdbSymbol& s : symbols_)
        if (s.function.empty() && s.name == name && (!best || s.is_global))
            best = &s;
    return best;
}

const CdbStruct* Cdb::struct_named(const std::string& name) const {
    for (const CdbStruct& s : structs_) if (s.name == name) return &s;
    return nullptr;
}

uint16_t Cdb::local_address(const CdbSymbol& s, uint16_t entry_sp, bool ra_saved, int ret_slot) {
    // an argument (offset >= 0) was pushed by the caller: at entry_sp + 1
    // up, past the result slot; a local (offset < 0) lies below the return
    // address the sra pushed
    int a = (int)entry_sp + 1 + s.offset;
    if (s.offset >= 0) a += ret_slot;
    else if (ra_saved) a -= 2;
    return (uint16_t)(a & 0x7fff);
}

// A byte comes back in A, two bytes (int, a pointer) in IX; anything wider
// or a struct in a slot the caller reserves just above the return address.
int Cdb::return_slot(const CdbFunction& f) const {
    if (!f.type.is_function()) return 0;
    CdbType r = f.type.element();
    if (r.is_pointer() || r.base == "SV") return 0;
    int n = type_size(r);
    return (r.base.rfind("ST", 0) == 0 || n > 2) ? n : 0;
}

// ---- values ----------------------------------------------------------

static std::string hex4(uint16_t v) { char b[8]; snprintf(b, sizeof b, "0x%04x", v); return b; }

// a byte of an object: in data, or in code space as the ldi of its ldi/ret pair
static int read_byte(uint16_t addr, bool in_code, const Cdb::ReadData& rd, const Cdb::ReadCode& rdcode) {
    if (in_code) {
        if (!rdcode) return -1;
        return rdcode((uint16_t)((2u * (addr & 0x7fffu)) & 0x7fffu)) & 0xff;
    }
    if (!rd) return -1;
    return rd(addr);
}

static bool read_n(uint16_t addr, int n, bool in_code, const Cdb::ReadData& rd, const Cdb::ReadCode& rdcode, uint32_t& v) {
    v = 0;
    for (int i = 0; i < n; i++) {
        int b = read_byte((uint16_t)(addr + i), in_code, rd, rdcode);
        if (b < 0) return false;
        v |= (uint32_t)b << (8 * i);
    }
    return true;
}

std::string Cdb::format(const CdbType& t, uint16_t addr, bool in_code, const ReadData& rd, const ReadCode& rdcode, int depth) const {
    char buf[64];
    if (depth > 3) return "...";

    if (t.is_array()) {
        CdbType e = t.element();
        int n = t.array_len();
        int esz = e.size > 0 ? e.size : base_size(e.base);
        if (esz <= 0) esz = 1;
        if (e.base == "SC" && e.mods.empty()) {                  // a string, if it reads as one
            std::string s = "\"";
            bool text = true;
            for (int i = 0; i < n && i < 60; i++) {
                int b = read_byte((uint16_t)(addr + i), in_code, rd, rdcode);
                if (b <= 0) { text = i > 0 && b == 0; break; }
                if (b >= 32 && b < 127) s += (char)b;
                else { text = false; break; }
            }
            if (text) return s + "\"";
        }
        std::string s = "{";
        for (int i = 0; i < n && i < 8; i++) {
            if (i) s += ", ";
            s += format(e, (uint16_t)(addr + i * esz), in_code, rd, rdcode, depth + 1);
        }
        if (n > 8) s += ", ...";
        return s + "}";
    }

    if (t.is_function()) return "function";

    if (t.is_pointer()) {
        uint32_t p;
        if (!read_n(addr, 2, in_code, rd, rdcode, p)) return "?";
        std::string s = hex4((uint16_t)p);
        const std::string& m = t.mods[0];
        bool target_code = (m.size() > 1 && m[1] == 'C') || (p & 0x8000);
        if (p & 0x8000) s += " (code)";
        CdbType e = t.element();
        if (e.mods.empty() && e.base == "SC" && p != 0) {        // a string it points to, if it reads as one
            std::string str = " \"";
            bool text = false;
            for (int i = 0; i < 40; i++) {
                int b = read_byte((uint16_t)((p & 0x7fff) + i), target_code, rd, rdcode);
                if (b <= 0) { text = i > 0 && b == 0; break; }
                if (b < 32 || b >= 127) break;
                str += (char)b;
                if (i == 39) text = true;
            }
            if (text) s += str + "\"";
        }
        return s;
    }

    if (t.base.rfind("ST", 0) == 0) {
        const CdbStruct* st = struct_named(t.base.substr(2));
        if (!st) return "struct " + t.base.substr(2) + " {?}";
        std::string s = "{";
        bool first = true;
        for (const auto& m : st->members) {
            if (!first) s += ", ";
            first = false;
            s += m.name + " = " + format(m.type, (uint16_t)(addr + m.offset), in_code, rd, rdcode, depth + 1);
        }
        return s + "}";
    }

    int n = t.size > 0 ? t.size : base_size(t.base);
    if (t.base == "SV") return "void";
    if (n <= 0 || n > 4) return "?";
    uint32_t v;
    if (!read_n(addr, n, in_code, rd, rdcode, v)) return "?";
    if (t.base == "SF") {
        float f; memcpy(&f, &v, 4);
        snprintf(buf, sizeof buf, "%g (0x%08x)", f, v);
        return buf;
    }
    if (t.base.rfind("SB", 0) == 0) { snprintf(buf, sizeof buf, "0x%02x (bit-field)", v & 0xff); return buf; }
    long long sv = (long long)v;
    if (!t.is_unsigned) {
        if (n == 1) sv = (int8_t)v; else if (n == 2) sv = (int16_t)v; else sv = (int32_t)v;
    }
    if (n == 1) {
        if (v >= 32 && v < 127) snprintf(buf, sizeof buf, "%lld '%c' (0x%02x)", sv, (char)v, v);
        else snprintf(buf, sizeof buf, "%lld (0x%02x)", sv, v);
    } else if (n == 2) snprintf(buf, sizeof buf, "%lld (0x%04x)", sv, v);
    else snprintf(buf, sizeof buf, "%lld (0x%08x)", sv, v);
    return buf;
}

int Cdb::type_size(const CdbType& t) const {
    if (t.size > 0) return t.size;
    if (t.is_pointer()) return 2;
    if (t.base.rfind("ST", 0) == 0) {
        const CdbStruct* st = struct_named(t.base.substr(2));
        int n = 0;
        if (st) for (const auto& m : st->members) n = std::max(n, m.offset + type_size(m.type));
        return n;
    }
    return base_size(t.base);
}

// ---- expressions -----------------------------------------------------

bool Cdb::variable(const std::string& name, const Frame* frame, Value& out) const {
    const CdbSymbol* sym = nullptr;
    if (frame)
        for (const CdbSymbol& s : symbols_)
            if (s.name == name && in_function(s, frame->function)) { sym = &s; break; }
    if (!sym) sym = global(name);
    if (!sym) return false;
    out = Value();
    out.type = sym->type;
    out.space = sym->space;
    if (sym->space == 'B') {
        if (!frame) return false;
        out.addr = local_address(*sym, frame->entry_sp, frame->ra_saved, frame->ret_slot);
        out.is_lvalue = true;
    } else if (sym->space == 'R') {
        if (!frame || !frame->a_valid) return false;
        bool in_a = false;
        for (const std::string& r : sym->regs) if (r == "a") in_a = true;
        if (!in_a) return false;
        out.val = frame->a;
    } else if (sym->has_addr) {
        out.addr = sym->addr;
        out.in_code = sym->space == 'D';
        out.is_lvalue = true;
    } else
        return false;
    return true;
}

namespace {
struct Parser {
    const Cdb& cdb;
    const Cdb::Frame* frame;
    const Cdb::ReadData& rd;
    const Cdb::ReadData& rdp;
    const Cdb::ReadCode& rc;
    std::string s;
    size_t i = 0;
    std::string err;

    void ws() { while (i < s.size() && s[i] == ' ') i++; }
    bool peek(const char* t) { ws(); return s.compare(i, strlen(t), t) == 0; }
    bool take(const char* t) { if (!peek(t)) return false; i += strlen(t); return true; }
    std::string ident() {
        ws();
        size_t b = i;
        while (i < s.size() && (isalnum((unsigned char)s[i]) || s[i] == '_')) i++;
        return s.substr(b, i - b);
    }

    // the bytes behind an lvalue
    bool read(const Cdb::Value& v, int n, uint32_t& out) {
        out = 0;
        for (int k = 0; k < n; k++) {
            int b;
            if (v.space == 'I') b = rdp ? rdp((uint16_t)(v.addr + k)) : -1;
            else b = read_byte((uint16_t)(v.addr + k), v.in_code, rd, rc);
            if (b < 0) { err = "memory not readable"; return false; }
            out |= (uint32_t)b << (8 * k);
        }
        return true;
    }

    bool deref(Cdb::Value& v) {
        if (v.type.is_array()) {                     // an array decays to its first element
            v.type = v.type.element();
            return true;
        }
        if (!v.type.is_pointer()) { err = "not a pointer"; return false; }
        uint32_t p = v.val;
        if (v.is_lvalue && !read(v, 2, p)) return false;
        bool code = v.type.mods[0].size() > 1 && v.type.mods[0][1] == 'C';
        if (p & 0x8000) code = true;
        v.type = v.type.element();
        v.addr = p & 0x7fff;
        v.in_code = code;
        v.is_lvalue = true;
        v.space = ' ';
        return true;
    }

    bool unary(Cdb::Value& v) {
        if (take("*")) {
            if (!unary(v)) return false;
            return deref(v);
        }
        if (take("&")) {
            if (!unary(v)) return false;
            if (!v.is_lvalue) { err = "not addressable"; return false; }
            v.val = (v.addr & 0x7fff) | (v.in_code ? 0x8000 : 0);
            v.type.mods.insert(v.type.mods.begin(), v.in_code ? "DC" : "DG");
            v.type.size = 2;
            v.is_lvalue = false;
            return true;
        }
        return postfix(v);
    }

    bool member(Cdb::Value& v) {
        std::string m = ident();
        if (v.type.base.rfind("ST", 0) != 0 || !v.type.mods.empty()) { err = "not a struct"; return false; }
        const CdbStruct* st = cdb.struct_named(v.type.base.substr(2));
        if (!st) { err = "unknown struct " + v.type.base.substr(2); return false; }
        for (const auto& mem : st->members)
            if (mem.name == m) {
                v.addr += mem.offset;
                v.type = mem.type;
                return true;
            }
        err = "no member " + m;
        return false;
    }

    bool postfix(Cdb::Value& v) {
        if (!primary(v)) return false;
        for (;;) {
            if (take("[")) {
                ws();
                char* end;
                long idx = strtol(s.c_str() + i, &end, 0);
                if (end == s.c_str() + i) { err = "index expected"; return false; }
                i = end - s.c_str();
                if (!take("]")) { err = "] expected"; return false; }
                if (!deref(v)) return false;
                v.addr += idx * cdb.type_size(v.type);
            } else if (take("->")) {
                if (!deref(v) || !member(v)) return false;
            } else if (take(".")) {
                if (!member(v)) return false;
            } else
                return true;
        }
    }

    bool primary(Cdb::Value& v) {
        if (take("(")) {
            if (!unary(v)) return false;
            if (!take(")")) { err = ") expected"; return false; }
            return true;
        }
        std::string name = ident();
        if (name.empty()) { err = "variable expected"; return false; }
        if (!cdb.variable(name, frame, v)) {
            err = frame && !frame->a_valid ? "no " + name + " here" : "no variable " + name;
            return false;
        }
        return true;
    }
};
}  // namespace

bool Cdb::evaluate(const std::string& expr, const Frame* frame, Value& out, std::string* err,
                   const ReadData& rd, const ReadData& rdperiph, const ReadCode& rdcode) const {
    Parser p{*this, frame, rd, rdperiph, rdcode, expr, 0, ""};
    bool ok = p.unary(out);
    if (ok) { p.ws(); if (p.i != p.s.size()) { ok = false; p.err = "junk after expression"; } }
    if (!ok && err) *err = p.err;
    return ok;
}

std::string Cdb::format_value(const Value& v, const ReadData& rd, const ReadData& rdperiph, const ReadCode& rdcode) const {
    if (!v.is_lvalue) {
        char buf[32];
        if (v.type.is_pointer()) snprintf(buf, sizeof buf, "0x%04x", v.val & 0xffff);
        else snprintf(buf, sizeof buf, "%u (0x%02x)", v.val, v.val);
        return buf;
    }
    if (v.space == 'I') {
        uint32_t b = rdperiph ? rdperiph((uint16_t)v.addr) : 0;
        char buf[32];
        snprintf(buf, sizeof buf, "%u (0x%02x)", b, b);
        return buf;
    }
    return format(v.type, (uint16_t)v.addr, v.in_code, rd, rdcode);
}

// ---- frames from the registers ---------------------------------------

static int sext10(uint16_t v) { return (v & 0x200) ? (int)(v & 0x3ff) - 0x400 : (int)(v & 0x3ff); }

Cdb::Prologue Cdb::prologue(const CdbFunction& f, uint16_t pc, const ReadCode& rdcode) const {
    Prologue p;
    p.end = f.addr;
    if (!rdcode || !f.has_addr) return p;
    uint16_t a = f.addr;
    for (int i = 0; i < 8; i++, a++) {                  // sra, then the ads of the locals (two for a big frame)
        uint16_t inst = rdcode(a);
        bool is_sra = (inst & 0xfffc) == 0xa160;
        bool is_ads = (inst >> 10) == 0x25 && sext10(inst) < 0;
        if (!is_sra && !is_ads) break;
        p.end = a + 1;
        if (a >= pc) continue;                           // not executed yet
        if (is_sra) { p.ra_saved = true; p.displacement += 2; }
        else p.displacement -= sext10(inst);
    }
    return p;
}

int Cdb::arg_bytes(const CdbFunction& f) const {
    int n = 0;
    for (const CdbSymbol* s : locals(f.name))
        if (s->space == 'B' && s->offset >= 0) n = std::max(n, s->offset + type_size(s->type));
    return n + return_slot(f);
}

std::vector<Cdb::StackFrame> Cdb::unwind(uint16_t pc, uint16_t sp, uint16_t ra, const ReadData& rd, const ReadCode& rdcode, int max) const {
    std::vector<StackFrame> out;
    const CdbFunction* f = function_at(pc);
    uint16_t cur_pc = pc, cur_sp = sp;
    for (int depth = 0; f && depth < max; depth++) {
        Prologue p = prologue(*f, depth == 0 ? cur_pc : (uint16_t)(f->end ? f->end : 0x7fff), rdcode);
        StackFrame fr;
        fr.pc = cur_pc;
        fr.function = f;
        fr.entry_sp = (uint16_t)((cur_sp + p.displacement) & 0x7fff);
        fr.ra_saved = p.ra_saved;
        if (p.ra_saved) {
            if (!rd) break;
            fr.ret_pc = (uint16_t)(rd(fr.entry_sp) | ((rd((uint16_t)(fr.entry_sp - 1)) & 0x7f) << 8));
        } else if (depth == 0) {
            fr.ret_pc = ra & 0x7fff;
        } else {
            fr.ret_pc = 0;                               // a caller that never saved RA: the walk ends
            out.push_back(fr);
            break;
        }
        out.push_back(fr);
        const CdbFunction* caller = function_at((uint16_t)(fr.ret_pc - 1));
        if (!caller || fr.ret_pc == 0) break;
        uint16_t caller_sp = (uint16_t)((fr.entry_sp + arg_bytes(*f)) & 0x7fff);
        if (caller_sp <= cur_sp && depth > 0) break;     // the stack must grow back up
        cur_pc = fr.ret_pc;
        cur_sp = caller_sp;
        f = caller;
    }
    return out;
}

// ---- where a line is left ----------------------------------------------

uint16_t Cdb::first_line_addr(const CdbFunction& f) const {
    if (!f.has_addr) return 0;
    for (const CdbLine& l : lines_)
        if (l.addr >= f.addr && (!f.has_end || l.addr < f.end)) return l.addr;
    return 0;
}

Cdb::Exits Cdb::line_exits(uint16_t pc, bool into, uint16_t ret_pc, const ReadCode& rdcode) const {
    Exits ex;
    std::set<uint16_t> stops, seen;
    std::vector<uint16_t> todo{pc};
    const CdbLine* l0 = line_at(pc);
    auto stop_at = [&](uint16_t a) { stops.insert(a); };
    // a successor: a line start stops there (pc's own line again means a
    // loop back), anything else is scanned on
    auto succ = [&](uint16_t a) {
        a &= 0x7fff;
        if (a == pc) { stop_at(a); return; }
        const CdbLine* l = line_starting(a);
        if (l && (!l0 || l->line != l0->line || l->file != l0->file || a < pc)) { stop_at(a); return; }
        if (!seen.count(a)) todo.push_back(a);
    };
    while (!todo.empty()) {
        uint16_t a = todo.back();
        todo.pop_back();
        if (seen.count(a)) continue;
        seen.insert(a);
        if (seen.size() > 4000) { ex.unknown = true; break; }
        uint16_t op = rdcode(a);
        uint16_t seq = (a + 1) & 0x7fff;
        int rel = ((int)(op & 0x7ff) << 21) >> 21;        // sign-extended inst[10:0]
        if (!(op & 0x8000)) {                                // jal
            const CdbFunction* callee = into ? function_at(op & 0x7fff) : nullptr;
            uint16_t first = callee && callee->addr == (op & 0x7fff) ? first_line_addr(*callee) : 0;
            if (first) stop_at(first);
            succ(seq);                                       // after the return
            continue;
        }
        uint16_t t5 = op >> 11, t6 = op >> 10, t9 = op >> 7, t10 = op >> 6, t11 = op >> 5;
        if (t5 == 0x16) { succ((uint16_t)(a + rel)); continue; }                    // br
        if (t5 == 0x15 || t5 == 0x17) { succ((uint16_t)(a + rel)); succ(seq); continue; }   // bnz / bz
        if (t6 == 0x23 || t9 == 0x114) { if (ret_pc) stop_at(ret_pc); else ex.unknown = true; continue; }   // ret #k / ret
        if (t10 == 0x22c || t10 == 0x22e) { if (ret_pc) stop_at(ret_pc); else ex.unknown = true; succ(seq); continue; }   // rc / rz
        if (t10 == 0x22d) { ex.unknown = true; continue; }                           // rets
        if (t11 == 0x454) { succ(seq); continue; }                                   // call ix: back at the next word
        if (t11 == 0x455) { ex.unknown = true; continue; }                           // jmp ix
        if ((op >> 4) == 0xa18) { succ((uint16_t)(a + 2)); continue; }               // ldx: the next word is its operand
        if ((op >> 8) == 0xa2) {                                                     // if / iftt / ifte: a skipped slot falls through
            int type = (op >> 3) & 3;
            if (type == 0 || type == 3) succ((uint16_t)(a + 2));
            else if (type == 1) succ((uint16_t)(a + 3));
            else { succ((uint16_t)(a + 2)); succ((uint16_t)(a + 3)); }
        }
        succ(seq);
    }
    ex.stops.assign(stops.begin(), stops.end());
    return ex;
}

std::string Cdb::print(const std::string& expr, const Frame* frame, std::string* err,
                       const ReadData& rd, const ReadData& rdperiph, const ReadCode& rdcode) const {
    Value v;
    if (!evaluate(expr, frame, v, err, rd, rdperiph, rdcode)) return "";
    std::string s = format_value(v, rd, rdperiph, rdcode) + "   " + v.type.describe();
    char buf[48];
    if (v.space == 'R') s += " in A";
    else if (v.space == 'I') { snprintf(buf, sizeof buf, " at peripheral 0x%02x", v.addr & 0xff); s += buf; }
    else if (v.is_lvalue) { snprintf(buf, sizeof buf, " at %s0x%04x", v.in_code ? "code " : "", v.addr & 0x7fff); s += buf; }
    return s;
}

}  // namespace lisa
