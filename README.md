# Goins-Mini-CPU-Sim-Final
For COSC A315

Hex machine code used (MIPS standard opcodes/funct)

Registers used: $zero=0, $t0=8, $t1=9, $t2=10

Program 1 (Increment Loop)
Assembly:
ADDI $t0,$zero,10  -> 0x2008000A
ADDI $t1,$zero,1   -> 0x20090001
ADD  $t2,$t0,$t1   -> 0x01095020
SW   $t2,0($zero)  -> 0xAC0A0000
Hex:
0x2008000A
0x20090001
0x01095020
0xAC0A0000

Program 2 (Summation Loop)
Assembly:
ADDI $t0,$zero,5   -> 0x20080005
ADDI $t1,$zero,0   -> 0x20090000
ADD  $t1,$t1,$t0   -> 0x01284820
ADDI $t0,$t0,-1    -> 0x2108FFFF
BEQ  $t0,$zero,END -> 0x11000001  (offset to END = +1)
BEQ  $0,$0,LOOP    -> 0x1000FFFC  (offset to LOOP = -4)
SW   $t1,0($zero)  -> 0xAC090000
Hex:
0x20080005
0x20090000
0x01284820
0x2108FFFF
0x11000001
0x1000FFFC
0xAC090000

Execution Traces

Program 1 — Execution trace (4 cycles)

Initial state: all regs = 0, PC = 0x00000000

Cycle 1: PC=0x00000000 IR=0x2008000A (ADDI)
  WB: $8 <= 10
Cycle 2: PC=0x00000004 IR=0x20090001 (ADDI)
  WB: $9 <= 1
Cycle 3: PC=0x00000008 IR=0x01095020 (ADD)
  WB: $10 <= 11
Cycle 4: PC=0x0000000C IR=0xAC0A0000 (SW)
  SW wrote 0x0000000B to 0x00000000


Final (key values):

$t0 ($8) = 10

$t1 ($9) = 1

$t2 ($10) = 11

Memory[0..3] = 0x0000000B (decimal 11)

Program 2 — Execution trace (22 cycles)

Initial: regs = 0, PC = 0x0. This loop sums 5+4+3+2+1 = 15 into $t1 then stores.

Cycle 1:  PC=0x00000000 IR=0x20080005 (ADDI)   -> $8 = 5
Cycle 2:  PC=0x00000004 IR=0x20090000 (ADDI)   -> $9 = 0
Cycle 3:  PC=0x00000008 IR=0x01284820 (ADD)    -> $9 = 5
Cycle 4:  PC=0x0000000C IR=0x2108FFFF (ADDI)   -> $8 = 4
Cycle 5:  PC=0x00000010 IR=0x11000001 (BEQ)    -> not taken
Cycle 6:  PC=0x00000014 IR=0x1000FFFC (BEQ)    -> taken to 0x8 (LOOP)
Cycle 7:  PC=0x00000008 IR=0x01284820 (ADD)    -> $9 = 9
Cycle 8:  PC=0x0000000C IR=0x2108FFFF (ADDI)   -> $8 = 3
Cycle 9:  PC=0x00000010 IR=0x11000001 (BEQ)    -> not taken
Cycle10:  PC=0x00000014 IR=0x1000FFFC (BEQ)    -> taken to 0x8
Cycle11:  PC=0x00000008 IR=0x01284820 (ADD)    -> $9 = 12
Cycle12:  PC=0x0000000C IR=0x2108FFFF (ADDI)   -> $8 = 2
Cycle13:  PC=0x00000010 IR=0x11000001 (BEQ)    -> not taken
Cycle14:  PC=0x00000014 IR=0x1000FFFC (BEQ)    -> taken to 0x8
Cycle15:  PC=0x00000008 IR=0x01284820 (ADD)    -> $9 = 14
Cycle16:  PC=0x0000000C IR=0x2108FFFF (ADDI)   -> $8 = 1
Cycle17:  PC=0x00000010 IR=0x11000001 (BEQ)    -> not taken
Cycle18:  PC=0x00000014 IR=0x1000FFFC (BEQ)    -> taken to 0x8
Cycle19:  PC=0x00000008 IR=0x01284820 (ADD)    -> $9 = 15
Cycle20:  PC=0x0000000C IR=0x2108FFFF (ADDI)   -> $8 = 0
Cycle21:  PC=0x00000010 IR=0x11000001 (BEQ)    -> taken to 0x18 (END)
Cycle22:  PC=0x00000018 IR=0xAC090000 (SW)     -> MEM[0] <= 15


Final (key values):

$t0 ($8) = 0

$t1 ($9) = 15

Memory[0..3] = 0x0000000F (decimal 15)

Instruction count: 22 cycles

Compile with the following
g++ -std=c++17 -O2 -o minisim minisim.cpp
./minisim