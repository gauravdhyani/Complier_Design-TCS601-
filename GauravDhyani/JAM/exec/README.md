# JAM Compiler (Integrated Version)

This is the integrated version of the JAM compiler.

## Required Directory Structure
   
   ```bash
├── JAM/                       # JAM Compiler Source Files
│   ├── lexer.c
│   ├── lexer.h
│   ├── parser.c
│   ├── parser.h
│   ├── semanticanalyser.c
│   ├── semanticanalyser.h
│   ├── executionengine.c
│   ├── executionengine.h
│   ├── main.c                 # Optional: for standalone JAM execution

## 🚀 How to Run

1. **Compile the program**  
   Open your terminal in the `JAM` directory and run:

   ```bash
   gcc -o jam main.c lexer.c parser.c semanticanalyser.c executionengine.c -Wall -g
2. **Execute the program**
   After successful compilation, run the JAM interpreter:
   ./jam