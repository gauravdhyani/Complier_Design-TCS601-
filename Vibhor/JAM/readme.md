# JAM Compiler Static Library

## Required Directory Structure
   
   ```bash
├── JAM/                       # JAM Compiler Source Files
│   ├── lexer.c
│   ├── lexer.h
│   ├── parser.c
│   ├── parser.h
│   ├── semanticanalyser.c
│   ├── semanticanalyser.h
│   ├── main.c                 # Optional: for standalone JAM execution
   ```
## 🛠️ Steps to Build

 **Compile source files into object files**  
   Run the following command inside the `JAM` directory:

   ```bash
   gcc -c compiler lexer.c parser.c semanticanalyser.c
   ./compiler
   ```

This will produce semantic output.
