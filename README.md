## My Role
Responsible for core gameplay logic, including snake movement, collision detection, score tracking, and game state management.
Contributed to bug fixing, modular design, and code maintainability using Git-based collaboration.

## Features

- **ASCII-based Visual Layout**: Console-based game with character graphics
- **Main Menu System**: Interactive menu with leaderboard access
- **Progressive Difficulty**: Game speed increases as you level up
- **Special Food Items**: Bonus food (`$`) worth 50 points (5x regular food)
- **Level System**: Level up every 5 foods eaten
- **Score Tracking System**: Persistent high score tracking with leaderboard
- **Real-time Game Rendering**: Smooth console-based UI updates
- **C++ Implementation**: Native C++ code for performance


## Building and Running

### Prerequisites
- C++ compiler (GCC 4.8+ or compatible)
- Linux/Unix system (for terminal functions) or Windows with WSL
- Make utility (optional, for easier building)

### Build Instructions

**Using Makefile:**
```bash
cd backend
make
```

This will build three executables:
- `snake_game` - The main game
- `game_menu` - Menu system with leaderboard
- `score_tracker` - Standalone score tracker utility

**Manual compilation:**
```bash
cd backend

# Compile snake game
g++ -std=c++11 -Wall -O2 -o snake_game snake_game.cpp

# Compile menu system
g++ -std=c++11 -Wall -O2 -o game_menu game_menu.cpp

# Compile score tracker
g++ -std=c++11 -Wall -O2 -o score_tracker score_tracker.cpp
```

### Running the Game

```bash
cd backend
./game_menu
```

The menu provides:
- Start New Game
- View Leaderboard
- Quit

Navigate with:
- **Arrow Keys** or **W/S**: Move selection
- **Enter** or **Space**: Select option
- **1/2/3**: Quick select by number
- **Q**: Quit


## Game Controls

- **Arrow Keys** or **WASD**: Move the snake
- **P**: Pause/Resume game
- **R**: Restart after game over
- **Q**: Quit game

### Menu Controls
- **Arrow Keys** or **W/S**: Navigate menu
- **Enter** or **Space**: Select option
- **1/2/3**: Quick select menu items
- **Q**: Quit

## Game Features

### Core Gameplay
- **30x20 game board** with ASCII graphics
- Snake grows when eating food
- Collision detection (walls and self)
- Real-time score display
- High score persistence
- Pause functionality

### Advanced Features
- **Progressive Difficulty**: Game speed increases with each level
  - Base speed: 150ms per move
  - Speed increases by 5ms per level
  - Maximum speed: 50ms per move
  
- **Level System**: 
  - Level increases every 5 foods eaten
  - Current level displayed in UI
  - Level reached shown on game over screen
  
- **Special Food Items**:
  - Bonus food appears randomly (20% chance)
  - Worth 50 points (5x regular food)
  - Disappears after 30 game cycles if not eaten
  - Visual indicator: `$` symbol
  
- **Balanced Movement**: 
  - Speed adjusted for terminal aspect ratio
  - Vertical and horizontal movement feel equal

## Score System

The score tracker maintains:
- Current game score
- High score tracking
- Leaderboard (top 10 scores)
- Timestamp for each score entry
- Automatic score saving on game over


## Technical Details

### Speed Calculation
- Base speed: 150,000 microseconds (150ms)
- Speed reduction: 5,000 microseconds per level
- Minimum speed: 50,000 microseconds (50ms)
- Vertical movement: 1.8x multiplier to compensate for terminal aspect ratio

### Special Food Mechanics
- Spawn chance: 20% per game cycle (when no special food exists)
- Duration: 30 game cycles
- Despawn: Automatically after timer expires

### Level Progression
- Level 1: 0-4 foods eaten
- Level 2: 5-9 foods eaten
- Level 3: 10-14 foods eaten
- And so on...


### Game Speed Issues
- The game automatically adjusts speed based on direction
- If movement feels uneven, try resizing your terminal window

### Leaderboard Not Showing
- Make sure you've played at least one game
- Check that `scores.txt` exists in the backend directory
- Scores are automatically saved when game ends

## License

This project is part of CSE3150 coursework.

