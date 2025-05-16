from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from uuid import uuid4
from scripts.chat_bot_groq import generate_jam_help
import os
import logging
app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("chat")
HISTORY_DIR = "history"
if not os.path.exists(HISTORY_DIR):
    os.makedirs(HISTORY_DIR)

class ChatRequest(BaseModel):
    session_id: str = None
    message: str
def get_history_path(session_id: str) -> str:
    return os.path.join(HISTORY_DIR, f"{session_id}.txt")
@app.post("/chat")
async def chat_endpoint(request: ChatRequest):
    session_id = request.session_id or str(uuid4())
    history_path = get_history_path(session_id)
    if os.path.exists(history_path):
        with open(history_path, "r", encoding="utf-8") as f:
            history = f.read()
    else:
        history = ""
    history += f"User: {request.message}\n"
    prompt = f"{history}EcoGuide: "
    try:
        response = generate_jam_help(prompt)
        history += f"Assistant: {response}\n"
        with open(history_path, "w", encoding="utf-8") as f:
            f.write(history)
        return {"session_id": session_id, "response": response}
    except Exception as e:
        logger.error("Error in /chat endpoint", exc_info=True)
        raise HTTPException(status_code=500, detail="Processing failed")
if __name__ == "__main__":
    import uvicorn
    uvicorn.run("fastapi_app.main:app", host="0.0.0.0", port=8000, reload=True)
