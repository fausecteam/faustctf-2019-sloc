#!/bin/bash

set -e

# Runs a single .sl program

prog=${1}

# Compile the Compiler
make Compiler

# Compile program
./Compiler ${prog}

# Assemble the program
g++ -no-pie program.s -o program

# Run the program
./program
