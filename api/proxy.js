const express = require('express');
const axios = require('axios');
const bodyParser = require('body-parser');
const cors = require('cors');

const app = express();

app.use(cors());
app.use(bodyParser.json());

const geminiApiEndpoint = 'https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent';
const geminiApiKey = process.env.GEMINI_API_KEY; // Use an environment variable

// Simple root route
app.get('/', (req, res) => {
  res.send('Welcome to the proxy server!');
});

// Route to generate content
app.post('/api/proxy/generateContent', async (req, res) => {
  try {
    const response = await axios.post(`${geminiApiEndpoint}?key=${geminiApiKey}`, req.body, {
      headers: {
        'Content-Type': 'application/json',
      },
    });
    res.json(response.data);
  } catch (error) {
    console.error('Error:', error);
    res.status(500).send('Error communicating with Google Gemini API');
  }
});

// Export the app for Vercel
module.exports = app;
