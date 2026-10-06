LISA (Little ISA) 8-bit RISC Processor CORE — 16‑bit Opcode ISA Summary

Overview
- Word size: 8‑bit data, 16‑bit instruction word (`inst[15:0]`).
- Instruction space: up to 2^PC_BITS words (typically 32K), PC_BITS‑bit PC). All instruction addresses and immediates refer to words.
- Data space: up to 2^D_BITS bytes (typically 32K), 8‑bit data bus.
- Key registers:
  - `A` (8): accumulator.
  - `IX` (PC_BITS+1c): index register with an associated 1‑bit condition tag (`ix_cond`).
  - `SP` (PC_BITS): stack pointer.
  - `PC` (PC_BITS): program counter.
  - `RA` (PC_BITS+1c): return address with an associated 1‑bit condition tag (`ra_cond`).
  - `IA` (PC_BITS): interrupt link register for ISR return.
  - `Z`, `C`: flags (zero, carry/borrow). No other architected flags.
- Conditional execution: an internal 2‑bit condition latch `cond[1:0]` gates most ops. The `if` instruction updates it. Opcode execution updates it with `cond <= {1'b1, cond[1}`.  Branchs, calls, `jal` update `cond <= 3`.
- Arithmetic mode (`amode[2:0]`):
  - `amode[1]`: signed mode for compare/div/rem/mul and arithmetic shifts (sign‑extend on right shifts and 16‑bit stream ops when set).
  - `amode[0]`: shift‑through‑carry (inserts `C` on shifts when set; otherwise logical 0 on shl/shr LSB/MSB).
  - `amode[2]`: FP rounding mode for BF16 ops; when set, `fadd/fdiv` round toward zero (0 = default rounding per FPU core).

Encoding Model (16‑bit)
- This core consumes 16‑bit opcodes.  8‑bit immediates live in `inst[7:0]`.
- Wide operands are widened in 16‑bit mode:
  - Absolute jump target (`jal`): 15 bits (`inst[14:0]`).
  - PC‑relative branch: signed 11 bits (`inst[10:0]`).
  - IX/SP indexed offsets: up to 9 bits zero‑extended (`inst[8:0]`) with IX/SP selection in `inst[9]`.
  - Small immediates otherwise remain 8 bits (`inst[7:0]`).

Addressing
- Indirect indexed: `[IX + uimm9]` or `[SP + uimm9]`. Bit `inst[9]` selects base: 0=IX, 1=SP; offset in `inst[8:0]` (zero‑extended).
- Absolute 256 address direct (low RAM/IO): `[p:imm8]` where `p=inst[9]` selects peripheral/RAM space p=0 for RAM, p=1 for peripheal; `imm8=inst[7:0]`.
- PC‑relative: `pc + sext(imm11)` where `imm11=inst[10:0]`.
- Literal immediates: `imm8=inst[7:0]`.

Control Flow
- `jal imm15` (jump and link): `inst[15]==0`; `PC <= imm15`, `RA <= PC+1` with `ra_cond <= cond[1]`, `cond <= 2'h3`.
- `call ix`: `inst[15:5]==10001010100`; `RA <= PC+1` (with `ra_cond <= cond[1]`), `PC <= IX`, `IX <= IX + 1`, `cond <= 2'h3`.
- `jmp ix`: `inst[15:5]==10001010101`; `PC <= IX` (no RA write), `cond <= 2'h3`.
- Relative branches (signed 11‑bit offset in inst[10:0]) (no RA write):
  - `br  rel11`:   `inst[15:11]==10110`, unconditional, `cond <= 2'h3`.
  - `bnz rel11`:   `inst[15:11]==10101`, taken if `Z==0`, `cond <= 2'h3`.
  - `bz  rel11`:   `inst[15:11]==10111`, taken if `Z==1`, `cond <= 2'h3`.
  - Offset is added to the current `PC` during decode; no implicit +1 bias beyond `pc + sext(rel11)`.
- Returns:
  - `ret #imm8`:  `inst[15:10]==100011`; `A<=imm8`, `PC<=RA`.
  - `ret`:        `inst[15:7]==100010100`; `PC<=RA`.
  - `rc`:         `inst[15:6]==1000101100xx`; return if `C==1`, `PC<=RA`.
  - `rz`:         `inst[15:6]==1000101110xx`; return if `Z==1`, `PC<=RA`.
  - `rets`:       `inst[15:6]==1000101101xx`; return from ISR, `PC<=IA`, re‑enables interrupts.

Conditional Execution (`if`)
- Encoding: `inst[15:8]==10100010` then `inst[4:3]=type`, `inst[2:0]=condcode`.
- Condcode truth based on current flags (`Z`, `C`) and signedness correction (below):
  - 0: EQ  (Z)
  - 1: NE  (!Z)
  - 2: NC  (!C)
  - 3: C   (C)
  - 4: GT  (!C_s & !Z)
  - 5: LT  (C_s & !Z)
  - 6: GE  (!C_s | Z)
  - 7: LE  (C_s | Z)
- Signedness for the C‑based relations uses internal `C_s`, which flips `C` when in a signed compare path. For plain `cpi` and memory `cmp`, signed vs unsigned depends on `amode[1]`.
- Type:
  - 0 (`if`):   `cond[0] = truth; cond[1] = 1`.
  - 1 (`iftt`): `cond[0] = truth; cond[1] = truth` (then/then sequencing).
  - 2 (`ifte`): `cond[0] = truth; cond[1] = !truth` (then/else sequencing).
  - 3: alias of type 0 (`if`).
- Effect: most ops execute only if `cond[0]==1`. Certain events (branches, 2‑stage ops) restore `cond` to a known state as per the core.

Immediate/Accumulator Ops (top 6 bits shown)
- `ldi  #imm8`          `10000000xxxxxxxx`: `A<=imm8`, `C<=0`, `Z<=A==0`.
- `adc  #imm8`          `10010000xxxxxxxx`: `A<=A+imm8+C`.
- `ads  #simm8`         `10010100xxxxxxxx`: `SP<=SP+sext(imm8)`.
- `adx  #simm8`         `10011000xxxxxxxx`: `IX<=IX+sext(imm8)`.
- `dcx  [b+uimm9]--`    `100111siiiiiiiii`: decrement mem; updates `Z`/`C` as specified below.
- `cpi  #imm8`          `10100100xxxxxxxx`: compare A vs imm; sets `Z`; `C` per unsigned compare or signed (`amode[1]`).
- `ldc  #imm1`          `101000000000100b`: set carry to bit `b`.
- `ldz  src`            `10100000011000cb`: if `c=0` then `Z<=b`; if `c=1` then `Z<= (b?C:~Z)`.
- `notz`                `1010000001110100`: `Z<= (A!=0)`.
- `eidi b`              `101000000111100b`: enable (`b=1`) / disable (`b=0`) interrupts.
- `brk`                 `1010000001111100`: software breakpoint (debug halt).
- `amode m2:m1:m0`      `1010000101000mmm`: set arithmetic/shift mode bits, BF16 rouding mode.
- `ldac cond`           `1010000111000ccc`: `A<={7'b0, cond(truth)}` using same condcodes as `if`.

Data Movement, Stack, and RA/IX Utilities
- `push a`              `1010000010xxxxxx`: push `A` (and `C`) to stack (implementation writes two bytes if needed).  The push writes A then decrements SP (an ASIC bug).
- `pop a`               `1010000011xxxxxx`: pop `A` (and `C`) from stack.  The pop increments PC then reads memory to restore A.
- `sra`                 `1010000101100000`: save (push) RA to stack (2 bytes: low then high with `ra_cond` in MSB).  The push stores then decrements SP for each upper/lower byte (an ASIC bug). 
- `lra`                 `1010000101100100`: load (pop) RA from stack.  The pop increments SP then reads RAM to restore upper/lower bytes (an ASIC bug).
- `push ix`             `1010000101101000`: push IX (2 bytes: low then high with `ix_cond` in MSB).  The push writes then decrements SP (an ASIC bug) for each upper/lower byte.
- `pop ix`              `1010000101101100`: pop IX. The pop reads then increments PC (an ASIC bug) for each upper/lower byte.
- `xchg ra`             `1000101011000000`: swap IX with RA (moves both value and condition bit).
- `xchg ia`             `1000101011000100`: swap IX with IA.
- `xchg sp`             `1000101011001000`: swap IX with SP.
- `spix`                `1000101011001100`: `IX<=SP` (does not modify SP).
- `cpx ra`              `1000101011010000`: compare IX vs RA; sets `Z` and `C`.
- `cpx sp`              `1000101011011000`: compare IX vs SP; sets `Z` and `C`.
- `txa`                 `1010000000010000`: `A<=IX[7:0]`.
- `txau`                `1010000000011000`: `A<={ix_cond, IX[PC_BITS-1:8]}`.
- `tax`                 `1010000100000000`: `IX[7:0]<=A`.
- `taxu`                `1010000100001000`: `{ix_cond, IX[PC_BITS-1:8]}<=A`.

Shifts and 16‑bit Stream Shifts
- `shl`                 `1010000000000000`: `A<={A[6:0], (amode[0]?C:1'b0)}`, `C<=old A[7]`.
- `shr`                 `1010000000000100`: `A<={(amode[1]?A[7]:amode[0]?C:1'b0), A[7:1]}`, `C<=old A[0]`.
- `shl16 [b+u2]`        `10100000001000ii`: two‑cycle stream left shift across `{mem,A}` at `[base+ii]`; memory byte gets `{mem[6:0], A[7]}`.
- `shr16 [b+u2]`        `10100000001100ii`: two‑cycle stream right shift across `{mem,A}`; memory byte gets `{(amode[1]?mem[7]:amode[0]?C:1'b0), mem[7:1]}`.
  - Base `b` is IX or SP depending on the op family’s usual `inst[9]` selector.

ALU + Memory (indirect via IX/SP)
- Major opcodes (`inst[15:10]`):
  - `110000` add:   `A<=A+M[base+uimm9]+C`.
  - `110010` sub:   `A<=A-M[base+uimm9]-C` (borrow‑in via `C`).
  - `111010` cmp:   compare `A` vs `M[...]`; sets `Z` and `C` (`C` is inverted add/sub carry; signedness per `amode[1]`).
  - `110100` and:   `A<=A & M[...]`.
  - `110101` andi:  `A<=A & imm8`.
  - `110110` or:    `A<=A | M[...]`.
  - `111000` xor:   `A<=A ^ M[...]`.
  - `111100` ldax:  `A<=M[base+uimm9]`.
  - `11111s` stax:  `M[base+uimm9]<=A` (store; `s` chooses base). Also drives peripheral when `inst[9]==1` in absolute forms.
  - `110111` swapi: swap `A` with low RAM/Peripheral `[p:imm8]` (`p=inst[9]`).
  - `111011` swap:  swap `A` with `M[base+uimm9]`.
  - `111001` inx:   `M[base+uimm9]++` (post‑inc; updates `Z`/`C` as below).
  - `100001` mulu:  `{temp, A}<=A * M[...]` (unsigned/signed high byte written via `mulu` path; see below).
  - `110001` mul:   `{temp, A}<=A * M[...]` (WANT_MUL gated).

Direct (absolute) Loads/Stores
- `lda  [p:imm8]`       `111101p0iiiiiiii`: `A<=mem[#imm8]`, `p` selects low RAM vs IO space.
- `sta  [p:imm8]`       `111111p0iiiiiiii`: `mem[#imm8]<=A`, `p` selects low RAM vs IO space.
- `cpi  #imm8`          `10010100XXXXXXXX` (immediate compare; see above).

Index Register Block Move via Stack Window
- `ldxx  [sp+uimm9]`    `1100110siiiiiiii`: two‑cycle load IX from stack window.
- `stxx  [sp+uimm9]`    `1100111siiiiiiii`: two‑cycle store IX to stack window.
  - Low byte transfers first; high byte includes the `ix_cond` bit in its MSB when written.

IX–A Arithmetic Helpers
- `addax`               `1010000110100c00`: `IX<=IX + {c?C:0, A}` (low 9 bits; updates `ix_cond=1`).
- `addaxu`              `1010000110100001`: `IX[MSB..8]<=IX[MSB..8] + A` (upper byte only; `ix_cond=1`).
- `subax`               `1010000110100c10`: `IX<=IX - {c?C:0, A}`.
- `subaxu`              `1010000110100011`: `IX[MSB..8]<=IX[MSB..8] - A`.

Division/Reminder Support (WANT_DIV)
- `lddiv`               `1010000101110000`: prepare 16‑bit dividend: `IX[7:0]<=A`; then in stage 2, `RA[7:0]<=mem[...]` (upper bytes captured via stack window per core sequence from #imm8 in next inst word).
- `div dv`              `10100011000000dv`: start division. Sources:
  - Divisor 16b = `{ dv[0] ? sext_or_0(A) : mem_byte , A }`.
  - Dividend 16b = `{ dv[1] ? sext_or_0(IX[7:0]) : RA[7:0] , IX[7:0] }`.
  - Signedness set by `amode[1]` (signed when 1; otherwise unsigned).
  - Result 16b appears with MSB available on data bus for store in stage 2 when configured; low 8 bits to `A` when ready.
- `rem dv`              `10100011000100dv`: like `div` but produces remainder.

Floating Point (BF16)
- Registers: `facc` (16‑bit BF16 accumulator), `f0..f3` (16‑bit BF16 registers, collectively `fx[0..3]`).
- Register select `ff=inst[1:0]`: `00:f0`, `01:f1`, `10:f2`, `11:f3`.
- Half‑transfer between `A` and `facc` (`h=inst[0]`, 0=low byte, 1=high byte):
  - `tfa h`            `101000011110000h`: `A <= (h?facc[15:8]:facc[7:0])`. Flags unaffected.
  - `taf h`            `101000011110001h`: `(h?facc[15:8]:facc[7:0]) <= A`. Flags unaffected.
- Binary arithmetic (result to `facc`; flags unaffected unless noted):
  - `fmul ff`          `10100001111001ff`: `facc <= facc * fx[ff]` (BF16 multiply).
  - `fadd ff`          `10100001111010ff`: `facc <= facc + fx[ff]` (BF16 add; rounding uses `amode[2]`: 1=round‑to‑zero).
  - `fdiv ff`          `10100001111110ff`: `facc <= facc / fx[ff]` (multi‑cycle BF16 divide; `C` reflects divider ready/valid during the op so code can poll/wait).
- Data movement and unary ops:
  - `fswap ff`         `10100001111100ff`: Swap `facc <=> fx[ff]`.
  - `fneg ff`          `10100001111011ff`: Negate `fx[ff]` in place (flip sign bit only).
  - `fcmp ff`          `10100001111101ff`: Compare `facc` vs `fx[ff]` (BF16): sets `Z=1` if equal; sets `C=1` if `facc > fx[ff]` (signed‑magnitude compare).
- Conversions and clear:
  - `itof`             `1010001100100000`: Convert 16‑bit integer in `facc` to BF16 in `facc` (`amode[1]` selects signed/unsigned source).
  - `ftoi`             `1010001100100001`: Convert BF16 in `facc` to 16‑bit integer in `facc` (`amode[1]` selects signed/unsigned destination range).
  - `fclr`             `1010001100100010`: `facc <= 0`.

Bit Test and Flag Effects
- `btst a,#bit3`       `1010000001000bbb`: sets `Z<=A[bit]`. Does not modify `C`.
- Flag updates:
  - `ldi` clears `C`.
  - `adc/add/sub/cmp/cpi` set `C` from carry/borrow and `Z` from equality.
  - `dcx/inx` set `C` when wrapping (`dcx: mem was 0`, `inx: mem was 0xFF`); `Z` set on specific post‑values (see core: `dcx` sets `Z` if new==1, `inx` if new==0xFF`).
  - Shifts: `C` <= shifted‑out bit; fill comes from `amode` as above.
  - `cpx ra/sp` sets `C` based on unsigned `<` and `Z` on equality.
  - `ldc/savec/restc` write/restore `C` or its saved shadow (and retain signed‑compare inversion info internally).
  - `fcmp` sets `Z=1` on equality and `C=1` when `facc > fx[ff]` (BF16 compare using sign/magnitude).
  - `fdiv` drives `C` with the divider “result valid/ready” signal during the operation (useful for polling/waits). Other FP ops leave flags unchanged.

Miscellaneous
- `brk`: debug breakpoint (halts in on‑chip debugger build).
- NOP: `1010000001110000`.
- Interrupts: `eidi` toggles global `ie`. On interrupt, `IA<=PC+1`, `PC` is vectored to a small fixed table (1..9) by 1‑hot `int_i`. `rets` returns and re‑enables `ie`.

Decoding Reference (by top bits)
- The core decodes on the MSBs of `inst` (16‑bit):
  - `inst[15]==0` → `jal` (absolute call/jump‑and‑link) with 15‑bit target.
  - `inst[15:11] in {10101,10110,10111}` → `bnz/br/bz` with 11‑bit signed offset.
  - `inst[15:10]==100000` → `ldi #imm8`.
  - `inst[15:10]==100011` → `ret #imm8`.
  - `inst[15:10]==100100` → `adc #imm8`.
  - `inst[15:10]==100101` → `ads #simm8`.
  - `inst[15:10]==100110` → `adx #simm8`.
  - `inst[15:10]==100111` → `dcx [b+uimm9]`.
  - `inst[15:1]==101000011110000`/`…001` → `tfa h`/`taf h` (with `h=inst[0]`).
  - `inst[15:2]==10100001111001` → `fmul ff` (`ff=inst[1:0]`).
  - `inst[15:2]==10100001111010` → `fadd ff`.
  - `inst[15:2]==10100001111011` → `fneg ff`.
  - `inst[15:2]==10100001111100` → `fswap ff`.
  - `inst[15:2]==10100001111101` → `fcmp ff`.
  - `inst[15:2]==10100001111110` → `fdiv ff`.
  - `inst[15:0] in {1010001100100000, …0001, …0010}` → `itof/ftoi/fclr`.
  - `inst[15:8]==10100010` → `if type,cond`.
  - `inst[15:12]==1010` with subpatterns → utility/shift/stack group (`ldc/ldz/notz/eidi/brk/push/pop/sra/lra/push_ix/pop_ix/tax/txa/…`).
  - `inst[15:11]==10001010100` → `call ix`; `…10101` → `jmp ix`.
  - `inst[15:10] in {110000,110010,110100,110101,110110,111000,111010,111011,111100,111001,111111,111101,110111}` → ALU/memory groups listed above.
  - `inst[15:12]==1010` and `…110000/110001` → `div/rem` when divider is enabled.

Calling/Return Model Hints (for LLVM)
- Calls: prefer `jal target` for absolute functions (15‑bit). Use `call ix` for indirect calls via `IX` (the frontend must place target in `IX`). Both write `RA=PC+1` and `ra_cond=cond[1]`. Use `call_ix` into single opcode `ret #val` table to access const strings in code space since `call_ix` auto-increments IX and then the `ret #val` can return the next byte of string / constant data.  
- Returns: plain `ret` reads `PC` from stack. For leaf functions that also materialize an 8‑bit return value, `ret #imm8` writes `A` then returns ... useful for loading constants or strings from program space.
- `RA` save/restore: `sra/lra` push/pop `RA` as two bytes; high byte packs `ra_cond` in its MSB. `push ix/pop ix` behave analogously with `ix_cond` in the MSB of the high byte.
- Stack: The ASIC implementation incorrectly implements push (A or IX) and `sra` by writing first and decrementing PC second.  Pop operations perform SP increment first then read.  So the ISA is symmetric, but the stack points to the next empty location vs. the value just pushed.  This means local stack variables are at SP+1, SP+2, etc. and not SP+0.

Register/Operand Model (for backend)
- GPRs: A (8) and IX (15). Treat SP and PC as special. Model RA as a link register (hidden) with spill via `sra/lra`.
- Flags: `Z`, `C` (model as implicit defs/uses).
- Addressing modes:
  - `Imm8`, `Rel11`, `Abs15`, `Abs8P(p,imm8)`, `Idx9(base=IX|SP, uimm9)`.
  - Two‑word instruction: `ldx` consumes the following instruction word as a 15‑bit literal for IX.
- Predication: most ops are predicated by `cond[0]` (set via `if`). Branches and returns are not predicated in the same way and reset `cond` as per hardware.
- Suggestion: Create "virtual" call frame registers as needed on the stack since SP based stack window directly accesses 9-bit offset.

Assembler Mnemonics (observed in core)
- `jal`, `br`, `bz`, `bnz`, `ret`, `ret #imm8`, `rc`, `rz`, `rets`.
- `ldi`, `adc`, `ads`, `adx`, `dcx`, `cpi`, `cmp`, `ldc`, `ldz`, `notz`, `eidi`, `brk`, `amode`, `ldac`.
- `sta`, `lda`, `stax`, `ldax`, `swap`, `swapi`, `inx`, `mulu`, `mul`.
- Floating point: `tfa`, `taf`, `fmul`, `fadd`, `fneg`, `fswap`, `fcmp`, `fdiv`, `itof`, `ftoi`, `fclr`.
- `call ix`, `jmp ix`, `xchg sp`, `xchg ra`, `xchg ia`, `spix`, `cpx ra`, `cpx sp`.
- `tax`, `taxu`, `txa`, `txau`, `ldx`, `push a`, `pop a`, `sra`, `lra`, `push ix`, `pop ix`.

Notes and Gotchas
- `btst` affects `Z` only (unlike the early comment in the source suggesting it loads `C`).
- `cpi/cmp` signedness is controlled by `amode[1]`; the condition evaluation hardware also inverts `C` for signed relations where needed internally.
- Branch offsets are 11‑bit signed and applied to the current `PC` (no extra `+1`).
- Absolute `jal` is the sole top‑bit‑0 instruction and writes `RA`. `RA` must be saved/restored via sra/lra if changed within a call frame.
- Direct `[p:imm8]` forms assert the `periph` qualifier when `p=1` (memory‐mapped IO region vs. low RAM).
- Many multi‑byte sequences (e.g., `ldxx/stxx`, `sra/lra`, `push/pop ix`) are implemented as two micro‑cycles by the core; model them as single instructions that may expand to 2 cycles in scheduling.

This document reflects the implemented 16‑bit decode in `lisa_core.v` (PWORD_SIZE=16). The original 14‑bit comments map by taking the top 6 bits unchanged and widening immediates as described above.

LLVM Mapping Notes
- Legal types: prefer `i8` as legal. Treat `i16` as legal for addresses and `IX` but note only 15 LSBs are architecturally significant; mask in prologue/critical defs if needed. Larger integers should be promoted and lowered via libcalls.
- Registers:
  - GPR8: `A`.
  - IDX16: `IX` (modeled as 16b; top bit unused), special: `SP`, `PC`, `RA`, `IA` are reserved.
  - Flags: `Z`, `C` are implicit defs/uses; do not model as allocatable regs.
- Calling convention (suggested):
  - Return: `i8` in `A`. Larger returns via memory.
  - Args: passed on stack (caller pushes right-to-left), 1‑byte aligned. Callee adjusts stack using `ads sp, #simm8`.
  - Caller‑saved: `A`, `IX`. Callee‑saved: `SP` (preserved), others non‑allocatable.
  - Calls: direct → `jal imm15`; indirect → materialize in `IX` then `call ix`.
- Branching and condition lowering:
  - Use `bz/bnz` for EQ/NE after `cmp/cpi`.
  - For LT/GE and carry‑based conditions, emit `if type,cond; br rel11`. If out‑of‑range, use `if; jal` or `if; ldx; jmp ix`.
- Addressing forms:
  - Indirect indexed: `[IX + uimm9]` or `[SP + uimm9]`.
  - Absolute direct: `[p:imm8]`; consider mapping `p=1` to an “IO/periph” address space or target memoperand flag.
  - PC‑relative branches: `rel11`.
- Multi‑word/latency notes:
  - `ldx` consumes the next word; model as a single MI with `UsesCustomInserter` or `hasPostISelHook` if needed for relaxation.
  - Two‑cycle memory sequences (`ldxx/stxx`, `push/pop ix`, `sra/lra`, `shl16/shr16`) can be modeled as single MIs; scheduler may mark them `Itinerary=2cy`.
- Division/remainder:
  - Present only if subtarget feature `+div`. Lower ISD DIV/UREM/… to `lddiv` + `div/rem` sequences with dv bits derived from operand signedness; result low byte to `A`, high byte available via memory side effect as per core.

Immediate/Operand constraints
- `imm2`: 0..3
- `imm8`: 0..255
- `simm8`: −128..127
- `uimm9`: 0..511 (index offsets)
- `rel11`: −1024..+1023 (branch reach)
- `abs15`: 0..32767 (code address)
- `bit3`: 0..7 (`btst`)
- `cond3`: 0..7 (`if` condition code)
- `type2`: 0..3 (`if` type)

Suggested pseudos (expand late)
- `Bcc rel11` → if cc in {EQ,NE}: `bz/bnz`, else: `if type,cond; br rel11`.
- `CALL abs15` → `jal abs15`.
- `CALLr IX` → `call ix`.
- `CALLi abs15_large` (relaxation) → `ldx #abs15; call ix` when `jal` range/relocation requires.
- `RET` → `ret`.
- `RETimm imm8` → `ret #imm8` when profitable; else `ldi a,#imm8; ret`.
- `MOVi8_A imm8` → `ldi a,#imm8`.
- `LD_A_abs p,imm8` → `lda [p:imm8]`.
- `ST_abs_A p,imm8` → `sta [p:imm8]`.
- Frame pseudos: `PROLOGUE framesize`, `EPILOGUE framesize` → expand to `ads sp, #-N` / restores and `ads sp, #+N`.

Starter TableGen skeleton
- Files to add:
  - `LISATarget.td`, `LISAInstrInfo.td`, `LISARegisterInfo.td`, `LISASubtarget.td`, `LISAPredicate.td`, `LISACallingConv.td`.
  - `AsmParser/`, `AsmPrinter/`, `ISelLowering/` C++ sources as usual.

Example: registers and classes (LISARegisterInfo.td)
```
class LISAReg<string n> : Register<n>;
def A  : LISAReg<"A">;
def IX : LISAReg<"IX">;  // Model as 16-bit; mask to 15 in prologue if needed
def SP : LISAReg<"SP">;  // reserved
def PC : LISAReg<"PC">;  // reserved
def RA : LISAReg<"RA">;  // reserved (link)
def IA : LISAReg<"IA">;  // reserved (isr link)

def GPR8 : RegisterClass<"LISA", [i8], 8, (add A)>;
def IDX  : RegisterClass<"LISA", [i16], 16, (add IX)>;
```

Example: operand types (LISAInstrInfo.td)
```
def imm8   : Operand<i8>;     // 0..255
def simm8  : Operand<i8>;     // -128..127 (assembler verify)
def uimm9  : Operand<i16>;    // 0..511   (assembler verify)
def rel11  : Operand<i16>;    // -1024..1023 (branch imm)
def abs15  : Operand<i16>;    // 0..32767  (code address)
```

Example: simple instructions
```
let hasSideEffects = 0, hasDelaySlot = 0 in {
  def LDI  : InstLISA<(outs), (ins imm8:$imm), "ldi\t$imm", []>;
  def ADCI : InstLISA<(outs), (ins imm8:$imm), "adc\t$imm", []>;
  def ANDI : InstLISA<(outs), (ins imm8:$imm), "andi\t$imm", []>;
  def LDAi : InstLISA<(outs), (ins i1:$p, imm8:$imm), "lda\t[$p:$imm]", []>;
  def STAi : InstLISA<(outs), (ins i1:$p, imm8:$imm), "sta\t[$p:$imm]", []>;
  def ADX  : InstLISA<(outs), (ins simm8:$off), "adx\t$off", []>;
  def ADS  : InstLISA<(outs), (ins simm8:$off), "ads\t$off", []>;
  def JAL  : InstLISA<(outs), (ins abs15:$tgt), "jal\t$tgt", []>;
  def BR   : InstLISA<(outs), (ins rel11:$off), "br\t$off",  []>;
  def BZ   : InstLISA<(outs), (ins rel11:$off), "bz\t$off",  []>;
  def BNZ  : InstLISA<(outs), (ins rel11:$off), "bnz\t$off", []>;
  def RET  : InstLISA<(outs), (ins),            "ret",        []>;
  def RETI : InstLISA<(outs), (ins imm8:$imm),  "ret\t#$imm", []>;
  def CALLIX : InstLISA<(outs), (ins IDX:$ix),  "call\tix",   []>;
  def JMPIX  : InstLISA<(outs), (ins IDX:$ix),  "jmp\tix",    []>;
}
```

Example: addressing helper and pattern stubs
```
def IXOff9 : ComplexPattern<iPTR, 1, "SelectIXOff9", [add] >; // base=IX/SP + uimm9

// Patterns (to be implemented in ISelLowering)
// add8 a, [ix+off]  → ADC w/ C cleared or ADD mem
// and8 a, imm8      → ANDI
// load8 a, [p:imm8] → LDAi
// store8 [p:imm8], a → STAi
```

Branch/compare lowering recipes
- EQ/NE: `cmp a, X` or `cpi a, imm8`; then `bz/bnz`.
- Unsigned LT/GE: `cmp`; then `if type=IF (0), cond=LT|GE; br`.
- Signed LT/GE: set `amode[1]=1`, `cmp`; then `if cond=LT|GE; br`; restore `amode` if needed.
- Out‑of‑range targets: relax to `if; ldx #abs15; jmp ix` or insert veneer.

Frame lowering sketch
- Prologue: optionally `sra` if RA changed; `ads sp, #-framesize`;  save callee‑saved (none by default); optionally `push ix` if used as callee‑saved by ABI profile.
- Epilogue: optionally `pop ix` if used; `ads sp, #+framesize`; optionally `lra` if RA changed in frame; `ret`.
