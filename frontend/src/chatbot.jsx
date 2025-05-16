import React, { useState } from 'react';
import axios from 'axios';
import ReactMarkdown from 'react-markdown';
const Chatbot = () => {
  const [message, setMessage] = useState('');
  const [conversation, setConversation] = useState([    {      role: 'assistant',
     text: '👋 Hi there! I’m JAMBot, your compiler-friendly assistant for the JAM language. Ask me anything about syntax, parsing, ASTs, or implementation details!'    }  ]);
  const sendMessage = async (e) => {
    e.preventDefault();
    if (!message.trim()) return;
    const userMessage = { role: 'user', text: message };
    setConversation((prev) => [...prev, userMessage]);
    try {
      const response = await axios.post('http://localhost:8000/chat', { message }, {
        headers: { 'Content-Type': 'application/json' }
      });
      const assistantMessage = { role: 'assistant', text: response.data.response };
      setConversation((prev) => [...prev, assistantMessage]);
    } catch (error) {
      console.error('Error:', error);
      const errorMessage = { role: 'assistant', text: 'Sorry, an error occurred.' };
      setConversation((prev) => [...prev, errorMessage]);
    }
    setMessage('');
  };
  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100vh', fontFamily: 'Arial, sans-serif' }}>
      <div style={{ flex: 1, overflowY: 'auto', padding: '1rem', backgroundColor: '#f0f0f0' }}>
        {conversation.map((msg, index) => (
          <div key={index} style={{ marginBottom: '1rem' }}>
            <strong>{msg.role === 'user' ? 'You' : 'JAMBot'}:</strong>
            <ReactMarkdown>{msg.text}</ReactMarkdown>
          </div>
        ))}
      </div>
      <form onSubmit={sendMessage} style={{ display: 'flex', padding: '1rem', backgroundColor: '#fff', borderTop: '1px solid #ccc' }}>
        <input
          type="text"
          value={message}
          onChange={(e) => setMessage(e.target.value)}
          placeholder="Ask anything about JAM Language."
          style={{ flex: 1, padding: '0.5rem', fontSize: '1rem' }}
        />
        <button type="submit" style={{ padding: '0.5rem 1rem', marginLeft: '0.5rem', fontSize: '1rem' }}>Send</button>
      </form>
    </div>
  );
};

export default Chatbot;
