import os
from groq import Groq
from dotenv import load_dotenv
load_dotenv()   
api_key = os.environ.get("API_KEY")
if not api_key:
    raise ValueError("GROQ_API_KEY environment variable not set")
client = Groq(api_key=api_key)
jam_prompt="""
Syntax Design for JAM:
• Token separators: space, \\t, \\n, \\r\\n  
• Comments: ** single-line, *- ... -* multiline  
• Keywords: fn, if, else, var, return, import, loop, forloop  
• Identifiers: [A-Za-z_][A-Za-z0-9_]*  
• Literals: Integer (\\d+), Float (\\d+\\.\\d+), String (“…” with escapes)  
• Operators: +, -, *, /, %, =, ==, !=, <, <=, >, >=, &&, ||, !  
• Arrow (return type): ->  
• Delimiters: ( ) { } [ ], :, ;, .  
• Data types: Int, Float, Bool, String, Void  
• Arrays: var arr:[Int]=[1,2,3,4]  
• Tuples: var p:(Int,Int)=(10,20)  
• Structs: struct p { name:String, age:Int }  

### Example Function:

```jam
fn factorial(n: Int) -> Int {
  if n <= 1 {
    return 1;
  } else {
    return n * factorial(n - 1);
  }
}
var result: Int = factorial(5);
Built by Jai,Vibhor,Gaurav
"""
def generate_jam_help(user_message: str) -> str:
    system_prompt = (
        "You are JAMBot — an expert on the JAM compiler project. "
        "Here’s the full JAM syntax reference and a sample factorial function:\n"
        f"{jam_prompt}\n"
        "When the user asks questions, provide clear, concise guidance on lexer/parser design, "
        "AST construction, syntax rules, or debugging examples."
        )
    response = client.chat.completions.create(
        model="meta-llama/llama-4-scout-17b-16e-instruct",
        messages=[
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_message}
        ],
        temperature=0.7
    )
    return response.choices[0].message.content.strip()

if __name__ == "__main__":
    user_prompt = input("Welcome to JAMBot! Ask me anything about the JAM compiler language.")
    while True:
        user_prompt = input("\nJAM Question> ").strip()
        if not user_prompt or user_prompt.lower() in ("exit", "quit"):
            print("Goodbye!")
            break
        answer = generate_jam_help(user_prompt)
        # Clean up any extra blank lines
        print("\nJAMBot:\n", "\n".join(line.strip() for line in answer.splitlines() if line.strip()))
