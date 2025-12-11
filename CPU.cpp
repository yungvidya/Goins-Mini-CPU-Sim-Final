#include <bits/stdc++.h>
using namespace std;

/*
 Mini MIPS-like CPU simulator (single-file)

 - 4 KB byte-addressable RAM (MEMORY_SIZE)
 - 32 registers (r[0] is hardwired to 0)
 - Program Counter (PC) holds byte address
 - Supports R-type (ADD,SUB), I-type (ADDI,LW,SW,BEQ)
 - Fetch/Decode/Execute/Memory/Writeback
 - Loads machine code (vector<uint32_t>) into memory at address 0 (big-endian words in loader)
 - Prints cycle-by-cycle traces
 Project: Mini CPU & Instruction Set Simulator — C++ + Python Assembler

Objective: Build a simplified MIPS-like CPU simulator and assembler that can encode, load, and execute small programs. Provide cycle-by-cycle execution trace and documentation.

Implemented features

Instruction set: ADD (R), SUB (R), ADDI (I), LW (I), SW (I), BEQ (I).

Instruction format: R-type: opcode(6)|rs(5)|rt(5)|rd(5)|shamt(5)|funct(6); I-type: opcode(6)|rs(5)|rt(5)|imm(16).

Simulator:

4 KB byte-addressable RAM

32 registers (regs[0] hardwired to zero)

Program Counter (PC) is byte-addressed and increments by 4 on fetch

Fetch → Decode → Execute → Memory → Write-back per cycle (sequential, not pipelined)

Loads instruction words as big-endian bytes (so hex words map directly). Data LW/SW use little-endian word reads/writes for natural integer storage (documented in code). This choice is modular and can be changed if desired.

Branch semantics follow MIPS: branch target = PC_after_increment + (signext(imm) << 2).

Trace output: Per cycle print of PC, IR, mnemonic, memory writes/reads, register write-backs, and a small memory window.
*/

static const int MEMORY_SIZE = 4096; // 4 KB

struct CPU {
    vector<uint8_t> mem;
    uint32_t regs[32];
    uint32_t PC;
    uint64_t cycle;

    CPU(): mem(MEMORY_SIZE, 0), PC(0), cycle(0) {
        memset(regs, 0, sizeof(regs));
    }

    // Helper: load word (32-bit) into memory at byte address addr (word-aligned)
    void store_word_be(uint32_t addr, uint32_t word) {
        if (addr + 3 >= mem.size()) {
            cerr << "Memory store out of range: " << addr << endl;
            exit(1);
        }
        // We'll store in memory as big-endian word bytes to match typical hex listing:
        mem[addr + 0] = (word >> 24) & 0xFF;
        mem[addr + 1] = (word >> 16) & 0xFF;
        mem[addr + 2] = (word >> 8) & 0xFF;
        mem[addr + 3] = (word >> 0) & 0xFF;
    }

    // Load 32-bit instruction from mem at address (big-endian)
    uint32_t fetch_word_be(uint32_t addr) {
        if (addr + 3 >= mem.size()) {
            cerr << "Memory fetch out of range: " << addr << endl;
            exit(1);
        }
        uint32_t w = 0;
        w |= (uint32_t)mem[addr + 0] << 24;
        w |= (uint32_t)mem[addr + 1] << 16;
        w |= (uint32_t)mem[addr + 2] << 8;
        w |= (uint32_t)mem[addr + 3] << 0;
        return w;
    }

    // Word (4 bytes) read / write (little-endian useful for data)
    int32_t read_word_le(uint32_t addr) {
        if (addr + 3 >= mem.size()) {
            cerr << "Data load out of range: " << addr << endl;
            exit(1);
        }
        uint32_t w = 0;
        w |= (uint32_t)mem[addr + 0] << 0;
        w |= (uint32_t)mem[addr + 1] << 8;
        w |= (uint32_t)mem[addr + 2] << 16;
        w |= (uint32_t)mem[addr + 3] << 24;
        return (int32_t)w;
    }

    void write_word_le(uint32_t addr, uint32_t val) {
        if (addr + 3 >= mem.size()) {
            cerr << "Data store out of range: " << addr << endl;
            exit(1);
        }
        mem[addr + 0] = (val >> 0) & 0xFF;
        mem[addr + 1] = (val >> 8) & 0xFF;
        mem[addr + 2] = (val >> 16) & 0xFF;
        mem[addr + 3] = (val >> 24) & 0xFF;
    }

    // Sign-extend a 16-bit immediate to 32 bits
    int32_t signext16(uint16_t v) {
        if (v & 0x8000) return (int32_t)(0xFFFF0000 | v);
        else return (int32_t)v;
    }

    // Print registers (compact)
    void print_regs() {
        ios old(nullptr);
        old.copyfmt(cout);
        cout << hex << setfill('0');
        for (int i = 0; i < 32; ++i) {
            cout << "$" << dec << setw(2) << i << "=0x" << setw(8) << regs[i];
            if ((i % 4) == 3) cout << "\n";
            else cout << "  ";
        }
        cout << dec;
        cout.copyfmt(old);
    }

    // Print small memory window (address 0..16)
    void print_mem_window(uint32_t start=0, uint32_t bytes=16) {
        cout << "Memory [" << start << " .. " << (start + bytes -1) << "]: ";
        for (uint32_t a = start; a < start + bytes && a < mem.size(); ++a) {
            cout << hex << setw(2) << setfill('0') << (int)mem[a] << " ";
        }
        cout << dec << setfill(' ') << "\n";
    }

    // Single step: fetch, decode, execute, memory, writeback
    // Returns false when it hits an instruction we treat as HALT (for simplicity we'll halt on PC out-of-range or an infinite loop limit).
    bool step(bool verbose=true) {
        // Basic guard: PC must be word-aligned and in memory
        if (PC + 3 >= mem.size()) {
            if (verbose) cout << "PC out of memory range or reached end. Halting.\n";
            return false;
        }

        ++cycle;
        uint32_t IR = fetch_word_be(PC);        // fetch (big-endian load as instruction)
        uint32_t oldPC = PC;
        PC += 4;                                // PC increments (we use MIPS convention: branch offsets applied relative to PC after this)

        uint32_t opcode = (IR >> 26) & 0x3F;
        uint32_t rs = (IR >> 21) & 0x1F;
        uint32_t rt = (IR >> 16) & 0x1F;
        uint32_t rd = (IR >> 11) & 0x1F;
        uint32_t shamt = (IR >> 6) & 0x1F;
        uint32_t funct = IR & 0x3F;
        uint16_t imm16 = IR & 0xFFFF;
        int32_t simm = signext16(imm16);

        string decoded = "UNKNOWN";

        // For write-back
        bool writeReg = false;
        uint32_t writeRegIdx = 0;
        uint32_t writeRegVal = 0;

        // For memory store
        bool memWrite = false;
        uint32_t memAddr = 0;
        uint32_t memWriteVal = 0;

        // For memory load
        bool memRead = false;
        uint32_t memReadReg = 0;

        // Execute / Decode
        if (opcode == 0) {
            // R-type: determine by funct
            if (funct == 0x20) { // ADD
                decoded = "ADD";
                int32_t a = (int32_t)regs[rs];
                int32_t b = (int32_t)regs[rt];
                int32_t res = a + b;
                writeReg = true;
                writeRegIdx = rd;
                writeRegVal = (uint32_t)res;
            } else if (funct == 0x22) { // SUB
                decoded = "SUB";
                int32_t a = (int32_t)regs[rs];
                int32_t b = (int32_t)regs[rt];
                int32_t res = a - b;
                writeReg = true;
                writeRegIdx = rd;
                writeRegVal = (uint32_t)res;
            } else {
                decoded = "R-UNIMPL";
            }
        } else if (opcode == 0x08) { // ADDI
            decoded = "ADDI";
            int32_t a = (int32_t)regs[rs];
            int32_t res = a + simm;
            writeReg = true;
            writeRegIdx = rt;
            writeRegVal = (uint32_t)res;
        } else if (opcode == 0x23) { // LW
            decoded = "LW";
            uint32_t addr = (uint32_t)((int32_t)regs[rs] + simm);
            if (addr + 3 >= mem.size()) {
                cerr << "LW address out of range: " << addr << "\n";
                return false;
            }
            memRead = true;
            memReadReg = rt;
            memAddr = addr;
        } else if (opcode == 0x2B) { // SW
            decoded = "SW";
            uint32_t addr = (uint32_t)((int32_t)regs[rs] + simm);
            if (addr + 3 >= mem.size()) {
                cerr << "SW address out of range: " << addr << "\n";
                return false;
            }
            memWrite = true;
            memAddr = addr;
            // For store, value is regs[rt]
            memWriteVal = regs[rt];
        } else if (opcode == 0x04) { // BEQ
            decoded = "BEQ";
            if (regs[rs] == regs[rt]) {
                // Branch target: PC = PC + (sign-extended immediate << 2)
                uint32_t target = PC + ((uint32_t)simm << 2);
                PC = target;
            }
        } else {
            decoded = "UNIMPL_OP";
        }

        // Print trace for this cycle
        if (verbose) {
            ios old(nullptr);
            old.copyfmt(cout);
            cout << "Cycle " << cycle << ": PC=0x" << hex << setw(8) << setfill('0') << oldPC
                 << " IR=0x" << setw(8) << IR << " (" << decoded << ") " << dec << setfill(' ') << "\n";
            cout.copyfmt(old);
        }

        // Memory stage for LW/SW
        if (memWrite) {
            // store as little-endian word for data memory (this simulator keeps code loaded as big-endian words but data as little-endian words)
            write_word_le(memAddr, memWriteVal);
            if (verbose) {
                cout << "  SW wrote 0x" << hex << setw(8) << setfill('0') << memWriteVal
                     << " to addr 0x" << setw(8) << memAddr << dec << setfill(' ') << "\n";
            }
        }
        if (memRead) {
            uint32_t val = (uint32_t)read_word_le(memAddr);
            writeReg = true;
            writeRegIdx = memReadReg;
            writeRegVal = val;
            if (verbose) {
                cout << "  LW loaded 0x" << hex << setw(8) << setfill('0') << val
                     << " from addr 0x" << setw(8) << memAddr << dec << setfill(' ') << "\n";
            }
        }

        // Write-back (R-type, ADDI, LW)
        if (writeReg) {
            if (writeRegIdx != 0) { // register 0 is always zero
                regs[writeRegIdx] = writeRegVal;
                if (verbose) {
                    cout << "  WB: Reg $" << writeRegIdx << " <= 0x" << hex << setw(8) << setfill('0') << writeRegVal << dec << setfill(' ') << "\n";
                }
            } else {
                if (verbose) cout << "  WB: attempt to write $0 ignored\n";
            }
        }

        // Enforce $zero = 0
        regs[0] = 0;

        if (verbose) {
            cout << " Registers after cycle " << cycle << ":\n";
            print_regs();
            cout << " Memory (first 16 bytes):\n";
            print_mem_window(0, 16);
            cout << "--------------------------------------------------\n";
        }

        // Return true to continue. We'll let main decide halting conditions (e.g., max cycles)
        return true;
    }
};

// Helper: pretty print hex 32-bit with leading zeros
string hex32(uint32_t v) {
    stringstream ss;
    ss << "0x" << hex << setw(8) << setfill('0') << v;
    return ss.str();
}

int main() {
    // Two sample programs (list of 32-bit words).
    // NOTE: We store these as uint32_t values (the loader will place them into memory as big-endian instruction words).
    vector<uint32_t> prog1 = {
        0x2008000A, // ADDI $t0,$zero,10
        0x20090001, // ADDI $t1,$zero,1
        0x01095020, // ADD  $t2,$t0,$t1
        0xAC0A0000  // SW   $t2,0($zero)
    };

    vector<uint32_t> prog2 = {
        0x20080005, // ADDI $t0,$zero,5
        0x20090000, // ADDI $t1,$zero,0
        0x01284820, // ADD  $t1,$t1,$t0
        0x2108FFFF, // ADDI $t0,$t0,-1
        0x11000001, // BEQ  $t0,$zero,END (offset=+1)
        0x1000FFFC, // BEQ  $0,$0,LOOP (offset=-4)
        0xAC090000  // SW   $t1,0($zero)
    };

    CPU cpu;

    // Choose program to load:
    int selectedProgram = 1; // 0=prog1, 1=prog2. Change if desired.
    vector<uint32_t> toLoad = (selectedProgram == 0 ? prog1 : prog2);

    // Load into memory at address 0 as big-endian instruction words (words contiguous)
    uint32_t addr = 0;
    for (uint32_t w : toLoad) {
        cpu.store_word_be(addr, w);
        addr += 4;
    }

    cout << "Loaded program " << selectedProgram << " (" << toLoad.size() << " words) into memory.\n";
    cout << "Starting simulation. PC=0x00000000\n";
    cout << "--------------------------------------------------\n";

    // Run until max cycles or until PC outside program range, or until we detect repeated PC beyond a large limit.
    const uint64_t MAX_CYCLES = 1000;
    uint64_t steps = 0;
    while (steps < MAX_CYCLES) {
        bool ok = cpu.step(true); // verbose
        if (!ok) break;
        ++steps;

        // Heuristic stop: if PC points into unused area beyond where we loaded program, stop
        if (cpu.PC >= MEMORY_SIZE) {
            cout << "PC >= memory size. Halting.\n";
            break;
        }
        // Alternatively, if PC points to region beyond loaded program and likely not code, we could halt.
        if (cpu.PC > (toLoad.size() * 4 + 100)) {
            // This heuristic avoids running wildly past program; adjust if desired
            cout << "PC beyond program area (heuristic). Halting.\n";
            break;
        }
    }

    cout << "Simulation finished after " << steps << " steps (cycles).\n";
    cout << "Final register and memory state:\n";
    cpu.print_regs();
    cout << "Memory first 16 bytes:\n";
    cpu.print_mem_window(0, 16);

    cout << "Value stored at memory[0] (word little-endian): ";
    int32_t result = cpu.read_word_le(0);
    cout << result << " (0x" << hex << setw(8) << setfill('0') << (uint32_t)result << dec << ")\n";

    return 0;
}
