# Compiler Design ( CD-VI-T169)
## JAM: Just another Micro-Language Compiler
*A Mini Compiler Prototype built in C for educational exploration of compiler design*
### Team Invicta Ittai

---

##  Introduction  
**JAM (Just Another Micro-Language)** is a lightweight, C-based language and compiler prototype designed to demonstrate the **phases of compiler construction** — lexical analysis, parsing, semantic analysis, and execution.  

The project is modular and educational, offering a simplified yet functional compiler-interpreter pipeline that emphasizes **error detection, AST generation, symbol management, and runtime execution**.  

---

##  Project Abstract  
The primary goal of **JAMC** is to build a **functional, modular compiler** that processes a minimal, functional-style programming language.  

Key highlights:  
- Implements **lexer, parser, semantic analyzer, and execution engine**.  
- Supports **basic control structures** (`if`, `else`, loops), **multiple data types** (Int, Float, Bool, String, Void, arrays, structs), and **function declarations**.  
- Demonstrates **variable scope management** with a **symbol table**.  
- Modular design in C  
This project serves as a **learning platform for compiler principles** and a foundation for future extensions (advanced control structures, GUI, improved optimizations).  

---

##  Why JAM  

- **Lightweight Language:** Minimal, easy to grasp for students.  
- **Syntax Inspiration:** Combines **TypeScript-style syntax** with OOP principles.  
- **Functional Programming Support:** Encourages exploration of functional constructs.  
- **Educational Tool:** Teaches compiler phases via live examples and AST visualization.  
- **C-Only Implementation:** Focused learning without distractions from large ecosystems.  
- **Multi-typed yet Minimal Design:** Supports Int, Float, Bool, String, arrays, and structs.  

---
## Result
   
   ```bash
INPUT Code:
var arr: [Int] = [1, 2, 3, 4];
var i: Int = 0;
print("loop");
while (i < 10) {
    i = i + 1;
    print(i);
}
print("Factorial of 5:");
var result: Int = factorial(5);
print(result);
fn factorial(n: Int) -> Int {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }


OUTPUT: 
===== Tokens =====
Token(type=7, lexeme='var', line=1, col=1)
Token(type=1, lexeme='arr', line=1, col=5)
Token(type=37, lexeme=':', line=1, col=8)
Token(type=34, lexeme='[', line=1, col=10)
Token(type=12, lexeme='Int', line=1, col=11)
Token(type=35, lexeme=']', line=1, col=14)
Token(type=20, lexeme='=', line=1, col=16)
Token(type=34, lexeme='[', line=1, col=18)
Token(type=2, lexeme='1', line=1, col=19)
Token(type=36, lexeme=',', line=1, col=20)
Token(type=2, lexeme='2', line=1, col=22)
Token(type=36, lexeme=',', line=1, col=23)
Token(type=2, lexeme='3', line=1, col=25)
Token(type=36, lexeme=',', line=1, col=26)
Token(type=2, lexeme='4', line=1, col=28)
Token(type=35, lexeme=']', line=1, col=29)
Token(type=38, lexeme=';', line=1, col=30)
Token(type=7, lexeme='var', line=2, col=1)
Token(type=1, lexeme='i', line=2, col=5)
Token(type=37, lexeme=':', line=2, col=6)
Token(type=12, lexeme='Int', line=2, col=8)
Token(type=20, lexeme='=', line=2, col=12)
Token(type=2, lexeme='0', line=2, col=14)
Token(type=38, lexeme=';', line=2, col=15)
Token(type=1, lexeme='print', line=3, col=1)
Token(type=30, lexeme='(', line=3, col=6)
Token(type=3, lexeme='loop', line=3, col=7)
Token(type=31, lexeme=')', line=3, col=13)
Token(type=38, lexeme=';', line=3, col=14)
Token(type=10, lexeme='while', line=4, col=1)
Token(type=30, lexeme='(', line=4, col=7)
Token(type=1, lexeme='i', line=4, col=8)
Token(type=23, lexeme='<', line=4, col=10)
Token(type=2, lexeme='10', line=4, col=12)
Token(type=31, lexeme=')', line=4, col=14)
Token(type=32, lexeme='{', line=4, col=16)
Token(type=1, lexeme='i', line=5, col=5)
Token(type=20, lexeme='=', line=5, col=7)
Token(type=1, lexeme='i', line=5, col=9)
Token(type=15, lexeme='+', line=5, col=11)
Token(type=2, lexeme='1', line=5, col=13)
Token(type=38, lexeme=';', line=5, col=14)
Token(type=1, lexeme='print', line=6, col=5)
Token(type=30, lexeme='(', line=6, col=10)
Token(type=1, lexeme='i', line=6, col=11)
Token(type=31, lexeme=')', line=6, col=12)
Token(type=38, lexeme=';', line=6, col=13)
Token(type=33, lexeme='}', line=7, col=1)
Token(type=1, lexeme='print', line=8, col=1)
Token(type=30, lexeme='(', line=8, col=6)
Token(type=3, lexeme='Factorial of 5:', line=8, col=7)
Token(type=31, lexeme=')', line=8, col=24)
Token(type=38, lexeme=';', line=8, col=25)
Token(type=7, lexeme='var', line=9, col=1)
Token(type=1, lexeme='result', line=9, col=5)
Token(type=37, lexeme=':', line=9, col=11)
Token(type=12, lexeme='Int', line=9, col=13)
Token(type=20, lexeme='=', line=9, col=17)
Token(type=1, lexeme='factorial', line=9, col=19)
Token(type=30, lexeme='(', line=9, col=28)
Token(type=2, lexeme='5', line=9, col=29)
Token(type=31, lexeme=')', line=9, col=30)
Token(type=38, lexeme=';', line=9, col=31)
Token(type=1, lexeme='print', line=10, col=1)
Token(type=30, lexeme='(', line=10, col=6)
Token(type=1, lexeme='result', line=10, col=7)
Token(type=31, lexeme=')', line=10, col=13)
Token(type=38, lexeme=';', line=10, col=14)
Token(type=4, lexeme='fn', line=11, col=1)
Token(type=1, lexeme='factorial', line=11, col=4)
Token(type=30, lexeme='(', line=11, col=13)
Token(type=1, lexeme='n', line=11, col=14)
Token(type=37, lexeme=':', line=11, col=15)
Token(type=12, lexeme='Int', line=11, col=17)
Token(type=31, lexeme=')', line=11, col=20)
Token(type=40, lexeme='->', line=11, col=22)
Token(type=12, lexeme='Int', line=11, col=25)
Token(type=32, lexeme='{', line=11, col=29)
Token(type=5, lexeme='if', line=12, col=5)
Token(type=30, lexeme='(', line=12, col=8)
Token(type=1, lexeme='n', line=12, col=9)
Token(type=24, lexeme='<=', line=12, col=11)
Token(type=2, lexeme='1', line=12, col=14)
Token(type=31, lexeme=')', line=12, col=15)
Token(type=32, lexeme='{', line=12, col=17)
Token(type=8, lexeme='return', line=13, col=9)
Token(type=2, lexeme='1', line=13, col=16)
Token(type=38, lexeme=';', line=13, col=17)
Token(type=33, lexeme='}', line=14, col=5)
Token(type=6, lexeme='else', line=14, col=7)
Token(type=32, lexeme='{', line=14, col=12)
Token(type=8, lexeme='return', line=15, col=9)
Token(type=1, lexeme='n', line=15, col=16)
Token(type=17, lexeme='*', line=15, col=18)
Token(type=1, lexeme='factorial', line=15, col=20)
Token(type=30, lexeme='(', line=15, col=29)
Token(type=1, lexeme='n', line=15, col=30)
Token(type=16, lexeme='-', line=15, col=32)
Token(type=2, lexeme='1', line=15, col=34)
Token(type=31, lexeme=')', line=15, col=35)
Token(type=38, lexeme=';', line=15, col=36)
Token(type=33, lexeme='}', line=16, col=5)
Token(type=33, lexeme='}', line=17, col=1)
Token(type=0, lexeme='EOF', line=18, col=1)

===== AST =====
Program (8 stmts):
  VarDecl: arr
    TypeAnnotation:
      Type: array of:
        Type: int
    Initializer:
      ArrayLiteral (4 elements):
        Element 0:
          Number: 1
        Element 1:
          Number: 2
        Element 2:
          Number: 3
        Element 3:
          Number: 4
  VarDecl: i
    TypeAnnotation:
      Type: int
    Initializer:
      Number: 0
  PrintStmt:
    String: "loop"
  WhileStmt:
    Condition:
      BinaryOp: <
        Identifier: i
        Number: 10
    Body:
      Program (2 stmts):
        ExprStmt:
            BinaryOp: =
              Identifier: i
              BinaryOp: +
                Identifier: i
                Number: 1
        PrintStmt:
          Identifier: i
  PrintStmt:
    String: "Factorial of 5:"
  VarDecl: result
    TypeAnnotation:
      Type: int
    Initializer:
      FunctionCall:
        Callee:
          Identifier: factorial
        Arg 0:
          Number: 5
  PrintStmt:
    Identifier: result
  Function: factorial
    Param 0:
      VarDecl: n
        TypeAnnotation:
          Type: int
    ReturnType:
      Type: int
    Program (1 stmts):
      IfStmt:
        Condition:
          BinaryOp: <=
            Identifier: n
            Number: 1
        Then:
          Program (1 stmts):
            Return:
              Number: 1
        Else:
          Program (1 stmts):
            Return:
              BinaryOp: *
                Identifier: n
                FunctionCall:
                  Callee:
                    Identifier: factorial
                  Arg 0:
                    BinaryOp: -
                      Identifier: n
                      Number: 1

===== Semantic Analysis =====
Node: PROGRAM with 8 statements
Node: VAR_DECL - Name: arr
Var Type:
Node: TYPE - Kind: TYPE_ARRAY
Initializer:
Node: ARRAY_LITERAL with 4 elements
Node: NUMBER - Value: 1
Node: NUMBER - Value: 2
Node: NUMBER - Value: 3
Node: NUMBER - Value: 4
Node: VAR_DECL - Name: i
Var Type:
Node: TYPE - Kind: TYPE_INT
Initializer:
Node: NUMBER - Value: 0
Node: PRINT_STATEMENT
Node: STRING - Value: loop
Node: WHILE
Condition:
Node: BINARY_EXPR - Operator: <
Node: IDENTIFIER - Name: i
Node: NUMBER - Value: 10
Body:
Node: PROGRAM with 2 statements
Node: EXPR_STMT
Node: BINARY_EXPR - Operator: =
Node: IDENTIFIER - Name: i
Node: BINARY_EXPR - Operator: +
Node: IDENTIFIER - Name: i
Node: NUMBER - Value: 1
Node: PRINT_STATEMENT
Node: IDENTIFIER - Name: i
Node: PRINT_STATEMENT
Node: STRING - Value: Factorial of 5:
Node: VAR_DECL - Name: result
Var Type:
Node: TYPE - Kind: TYPE_INT
Initializer:
Node: FUNCTION_CALL - Arguments: 1
Callee:
Node: IDENTIFIER - Name: factorial
Node: NUMBER - Value: 5
Node: PRINT_STATEMENT
Node: IDENTIFIER - Name: result
Node: FUNCTION - Name: factorial, Params: 1
Return Type:
Node: TYPE - Kind: TYPE_INT
Body:
Node: PROGRAM with 1 statements
Node: IF
Condition:
Node: BINARY_EXPR - Operator: <=
Node: IDENTIFIER - Name: n
Node: NUMBER - Value: 1
Then Branch:
Node: PROGRAM with 1 statements
Node: RETURN
Node: NUMBER - Value: 1
Else Branch:
Node: PROGRAM with 1 statements
Node: RETURN
Node: BINARY_EXPR - Operator: *
Node: IDENTIFIER - Name: n
Node: FUNCTION_CALL - Arguments: 1
Callee:
Node: IDENTIFIER - Name: factorial
Node: BINARY_EXPR - Operator: -
Node: IDENTIFIER - Name: n
Node: NUMBER - Value: 1
Semantic analysis completed successfully.


===== Execution =====
loop
1.00
2.00
3.00
4.00
5.00
6.00
7.00
8.00
9.00
10.00
Factorial of 5:
120                
```

