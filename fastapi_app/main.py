from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from scripts.chat_bot_llama import get_local_response
import logging

app = FastAPI()
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("chat")

class ChatRequest(BaseModel):
    question: str

@app.post("/chat")
async def chat_endpoint(chat_request: ChatRequest):
    try:
        response = get_local_response(chat_request.question)
        return {"question": chat_request.question, "response": response}
    except Exception as e:
        logger.error("Error in /chat endpoint", exc_info=True)
        raise HTTPException(status_code=500, detail="Processing failed")

# For local testing, you can run this directly.
if __name__ == "__main__":
    import uvicorn
    uvicorn.run("fastapi_app.main:app", host="0.0.0.0", port=8000, reload=True)
