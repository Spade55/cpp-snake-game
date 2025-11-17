# CSE3150-SnakeGame

A classic Snake game implementation with frontend and backend components.

## Features

- Classic snake game mechanics
- Score tracking
- High score persistence (localStorage)
- Backend API for score management
- Responsive and modern UI

## Setup Instructions

### Frontend

The frontend is a static HTML/CSS/JavaScript application. Simply open `frontend/index.html` in a web browser, or use a local server:

```bash
# Using Python
cd frontend
python -m http.server 8000

# Using Node.js (http-server)
npx http-server frontend -p 8000
```

Then open `http://localhost:8000` in your browser.

### Backend

1. Navigate to the backend directory:
```bash
cd backend
```

2. Install dependencies:
```bash
npm install
```

3. Start the server:
```bash
npm start
```

For development with auto-reload:
```bash
npm run dev
```

The backend will run on `http://localhost:3000`

## How to Play

- Use **Arrow Keys** or **WASD** to control the snake
- Eat the red food to grow and increase your score
- Avoid hitting the walls or yourself
- Try to achieve the highest score possible!

## Game Controls

- **Start Game**: Begin a new game
- **Pause**: Pause/resume the current game
- **Reset**: Reset the game to initial state

## API Endpoints

- `GET /api/scores` - Get top 10 scores
- `POST /api/scores` - Save a new score
- `GET /api/scores/high` - Get the highest score
