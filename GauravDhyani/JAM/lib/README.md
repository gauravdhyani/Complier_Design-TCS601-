# JAM Compiler Static Library

This guide shows how to compile the JAM compiler source files into a static library (`libjam.a`).

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
   ```
## 🛠️ Steps to Build

1. **Compile source files into object files**  
   Run the following command inside the `JAM` directory:

   ```bash
   gcc -c lexer.c parser.c semanticanalyser.c executionengine.c ```
2. Create the static library libjam.a
   Use the ar command to bundle the object files:

   ```bash
   ar rcs libjam.a lexer.o parser.o semanticanalyser.o executionengine.o ```

This will generate libjam.a, which can now be linked with your shell or other applications.
