# JAM Compiler Static Library

## Required Directory Structure
   
   ```bash
├── JAM/                       # JAM Compiler Source Files
│   ├── lexer.c
│   ├── lexer.h
│   ├── parser.c
│   ├── parser.h
│   ├── main.c                 # Optional: for standalone JAM execution
   ```
## 🛠️ Steps to Build

 **Compile source files into object files**  
   Run the following command inside the `JAM` directory:

   ```bash
   gcc -o jamc lexer.c parser.c main.c
   ./jamc
   ```

This will produce semantic output.

