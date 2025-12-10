# Goins-Mini-CPU-Sim-Final
For COSC A315

Hex machine code used (MIPS standard opcodes/funct)

Registers used: $zero=0, $t0=8, $t1=9, $t2=10

Program 1 (Increment Loop)

ADDI $t0,$zero,10  -> 0x2008000A
ADDI $t1,$zero,1   -> 0x20090001
ADD  $t2,$t0,$t1   -> 0x01095020
SW   $t2,0($zero)  -> 0xAC0A0000


Program 2 (Summation Loop)

ADDI $t0,$zero,5   -> 0x20080005
ADDI $t1,$zero,0   -> 0x20090000
ADD  $t1,$t1,$t0   -> 0x01284820
ADDI $t0,$t0,-1    -> 0x2108FFFF
BEQ  $t0,$zero,END -> 0x11000001  (offset to END = +1)
BEQ  $0,$0,LOOP    -> 0x1000FFFC  (offset to LOOP = -4)
SW   $t1,0($zero)  -> 0xAC090000

Compile with the following
g++ -std=c++17 -O2 -o minisim minisim.cpp
./minisim