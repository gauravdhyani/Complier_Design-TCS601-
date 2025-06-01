import os
import torch
from transformers import AutoModelForCausalLM, AutoTokenizer, pipeline
model_path = "C:/Users/JAI/.cache/huggingface/hub/models--meta-llama--Llama-3.2-1B/snapshots/4e20de362430cd3b72f300e6b0f18e50e7166e08/"
tokenizer = AutoTokenizer.from_pretrained(model_path)
model = AutoModelForCausalLM.from_pretrained(model_path, torch_dtype=torch.float16)
text_gen_pipeline = pipeline(
    "text-generation",
    model=model,
    tokenizer=tokenizer,
    device=0 
)

def get_local_response(prompt: str, max_length: int = 256) -> str:
    """
    Generate a response using the locally loaded model.
    """
    # You might include extra context if needed. Here we simply use the prompt.
    output = text_gen_pipeline(prompt, max_length=max_length, truncation=True)
    return output[0]['generated_text']

# For quick testing from the command line:
if __name__ == "__main__":
    prompt = input("Enter prompt: ")
    response = get_local_response(prompt)
    print("Response:", response)
