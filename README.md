# updated-pascal-compiler
Spring 2026

Stage 0:
File I/O / constructor / destructor
Listing + object file generation
Symbol table class usage
Recursive descent parser structure
program / const / var / begin end
Identifier + constant insertion
Type/value lookup for constants
emitPrologue / emitEpilogue / emitStorage
Scanner (nextChar, nextToken)
Error handling

Compilation instructions:
STEP 1
mkdir stage0

STEP 2
cp /usr/local/4301/src/Makefile .

STEP 3
Edit Makefile to add a target of stage0 to targets2srcfiles

STEP 4
cp /usr/local/4301/include/stage0.h .

STEP 5
cp /usr/local/4301/src/stage0main.C .

STEP 6
make stage0
