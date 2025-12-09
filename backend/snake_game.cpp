#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <fstream>
#include <algorithm>
#include <sstream>

using namespace std;

// Score tracker implementation (inline to avoid multiple main() definitions)
struct ScoreEntry {
    int score;
    string timestamp;
    
    ScoreEntry() : score(0), timestamp("") {}
    ScoreEntry(int s, string t) : score(s), timestamp(t) {}
    
    bool operator>(const ScoreEntry& other) const {
        return score > other.score;
    }
};

class ScoreTracker {
private:
    string scoreFile;
    vector<ScoreEntry> scores;
    
    string getCurrentTimestamp() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        stringstream ss;
        ss << (1900 + ltm->tm_year) << "-"
           << setfill('0') << setw(2) << (1 + ltm->tm_mon) << "-"
           << setfill('0') << setw(2) << ltm->tm_mday << " "
           << setfill('0') << setw(2) << ltm->tm_hour << ":"
           << setfill('0') << setw(2) << ltm->tm_min;
        return ss.str();
    }
    
public:
    ScoreTracker(const string& filename = "scores.txt") : scoreFile(filename) {
        loadScores();
    }
    
    void loadScores() {
        scores.clear();
        ifstream file(scoreFile);
        if (file.is_open()) {
            int score;
            string timestamp;
            while (file >> score) {
                file.ignore();
                getline(file, timestamp);
                scores.push_back(ScoreEntry(score, timestamp));
            }
            file.close();
            sort(scores.begin(), scores.end(), greater<ScoreEntry>());
        }
    }
    
    void saveScore(int score) {
        string timestamp = getCurrentTimestamp();
        scores.push_back(ScoreEntry(score, timestamp));
        sort(scores.begin(), scores.end(), greater<ScoreEntry>());
        if (scores.size() > 10) {
            scores.resize(10);
        }
        ofstream file(scoreFile);
        if (file.is_open()) {
            for (const auto& entry : scores) {
                file << entry.score << " " << entry.timestamp << endl;
            }
            file.close();
        }
    }
    
    int getHighScore() {
        if (scores.empty()) {
            return 0;
        }
        return scores[0].score;
    }
};

using namespace std;

// Game constants
const int BOARD_WIDTH = 30;
const int BOARD_HEIGHT = 20;
const char SNAKE_BODY = 'O';
const char SNAKE_HEAD = '@';
const char FOOD = '*';
const char SPECIAL_FOOD = '$';
const char WALL = '#';
const char EMPTY = ' ';
const int BASE_SPEED = 150000; // microseconds (slower = faster game)
const int MIN_SPEED = 50000;   // Maximum speed limit

// Game state
struct Position {
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

// Linux terminal input functions
class TerminalInput {
private:
    struct termios oldTermios;
    bool initialized;
    
public:
    TerminalInput() : initialized(false) {
        tcgetattr(STDIN_FILENO, &oldTermios);
        struct termios newTermios = oldTermios;
        newTermios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
        initialized = true;
    }
    
    ~TerminalInput() {
        if (initialized) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
        }
    }
    
    bool kbhit() {
        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0;
    }
    
    char getch() {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            return ch;
        }
        return 0;
    }
};

class SnakeGame {
private:
    vector<Position> snake;
    Position food;
    Position specialFood;
    Position direction;
    int score;
    int highScore;
    int level;
    int foodsEaten;
    bool hasSpecialFood;
    int specialFoodTimer;
    bool gameOver;
    bool gamePaused;
    TerminalInput terminal;
    ScoreTracker scoreTracker;
    
    void clearScreen() {
        system("clear");
    }
    
    void setCursorPosition(int x, int y) {
        // ANSI escape code for cursor positioning
        cout << "\033[" << (y + 1) << ";" << (x + 1) << "H";
    }
    
    void hideCursor() {
        // ANSI escape code to hide cursor
        cout << "\033[?25l";
    }
    
    void showCursor() {
        // ANSI escape code to show cursor
        cout << "\033[?25h";
    }
    
    Position generateFood() {
        Position newFood;
        bool valid = false;
        while (!valid) {
            newFood.x = rand() % (BOARD_WIDTH - 2) + 1;
            newFood.y = rand() % (BOARD_HEIGHT - 2) + 1;
            valid = true;
            // Make sure food doesn't spawn on snake
            for (const auto& segment : snake) {
                if (segment == newFood) {
                    valid = false;
                    break;
                }
            }
            // Make sure it doesn't spawn on special food
            if (hasSpecialFood && newFood == specialFood) {
                valid = false;
            }
        }
        return newFood;
    }
    
    void spawnSpecialFood() {
        if (!hasSpecialFood && (rand() % 100) < 20) { // 20% chance
            specialFood = generateFood();
            hasSpecialFood = true;
            specialFoodTimer = 30; // Lasts for 30 game cycles
        }
    }
    
    void updateSpecialFood() {
        if (hasSpecialFood) {
            specialFoodTimer--;
            if (specialFoodTimer <= 0) {
                hasSpecialFood = false;
            }
        } else {
            spawnSpecialFood();
        }
    }
    
    int calculateSpeed() {
        // Speed increases with level (game gets faster)
        int speed = BASE_SPEED - (level * 5000);
        if (speed < MIN_SPEED) {
            speed = MIN_SPEED;
        }
        return speed;
    }
    
    int getAdjustedSpeed() {
        // Adjust speed based on direction to compensate for terminal aspect ratio
        // Terminal characters are taller than wide, so vertical movement appears faster
        int baseSpeed = calculateSpeed();
        
        // If moving vertically (up/down), slow down to match horizontal speed visually
        if (direction.y != 0) {
            // Multiply by ~1.8 to compensate for typical terminal character aspect ratio
            return (int)(baseSpeed * 1.8);
        }
        
        return baseSpeed;
    }
    
    int calculateLevel() {
        // Level increases every 5 foods eaten
        return (foodsEaten / 5) + 1;
    }
    
    void drawBoard() {
        setCursorPosition(0, 0);
        
        // Draw top border
        for (int i = 0; i < BOARD_WIDTH; i++) {
            cout << WALL;
        }
        cout << endl;
        
        // Draw game area
        for (int y = 1; y < BOARD_HEIGHT - 1; y++) {
            cout << WALL; // Left border
            for (int x = 1; x < BOARD_WIDTH - 1; x++) {
                Position pos(x, y);
                if (pos == snake[0]) {
                    cout << SNAKE_HEAD;
                } else if (pos == food) {
                    cout << FOOD;
                } else if (hasSpecialFood && pos == specialFood) {
                    cout << SPECIAL_FOOD;
                } else {
                    bool isSnakeBody = false;
                    for (size_t i = 1; i < snake.size(); i++) {
                        if (snake[i] == pos) {
                            cout << SNAKE_BODY;
                            isSnakeBody = true;
                            break;
                        }
                    }
                    if (!isSnakeBody) {
                        cout << EMPTY;
                    }
                }
            }
            cout << WALL; // Right border
            cout << endl;
        }
        
        // Draw bottom border
        for (int i = 0; i < BOARD_WIDTH; i++) {
            cout << WALL;
        }
        cout << endl;
    }
    
    void drawUI() {
        // Draw score and game info
        cout << "\n";
        cout << "  Score: " << setw(6) << score;
        cout << "  |  High Score: " << setw(6) << highScore;
        cout << "  |  Level: " << setw(3) << level;
        cout << "  |  Length: " << setw(3) << snake.size();
        cout << "\n";
        
        if (hasSpecialFood && !gameOver && !gamePaused) {
            cout << "  [BONUS FOOD AVAILABLE! $" << SPECIAL_FOOD << " = 50 points]\n";
        }
        
        if (gamePaused) {
            cout << "  [PAUSED] Press 'P' to resume\n";
        }
        
        if (gameOver) {
            cout << "\n";
            cout << "  ========================================\n";
            cout << "  |         GAME OVER!                  |\n";
            cout << "  |         Final Score: " << setw(6) << score << "      |\n";
            cout << "  |         Level Reached: " << setw(3) << level << "        |\n";
            cout << "  ========================================\n";
            if (score > highScore) {
                cout << "  *** NEW HIGH SCORE! ***\n";
            }
            cout << "  Press 'R' to restart or 'Q' to quit\n";
        } else {
            cout << "  Controls: Arrow Keys or WASD | P=Pause | Q=Quit\n";
        }
    }
    
    bool checkCollision(const Position& head) {
        // Check wall collision
        if (head.x <= 0 || head.x >= BOARD_WIDTH - 1 ||
            head.y <= 0 || head.y >= BOARD_HEIGHT - 1) {
            return true;
        }
        
        // Check self collision
        for (size_t i = 1; i < snake.size(); i++) {
            if (snake[i] == head) {
                return true;
            }
        }
        
        return false;
    }
    
    void update() {
        if (gameOver || gamePaused) return;
        
        // Update level based on foods eaten
        level = calculateLevel();
        
        // Update special food timer
        updateSpecialFood();
        
        // Calculate new head position
        Position newHead = snake[0];
        newHead.x += direction.x;
        newHead.y += direction.y;
        
        // Check collisions
        if (checkCollision(newHead)) {
            gameOver = true;
            if (score > highScore) {
                highScore = score;
            }
            // Save score to leaderboard
            if (score > 0) {
                scoreTracker.saveScore(score);
            }
            return;
        }
        
        // Move snake
        snake.insert(snake.begin(), newHead);
        
        // Check special food collision (worth more points)
        if (hasSpecialFood && newHead == specialFood) {
            score += 50;
            foodsEaten++;
            hasSpecialFood = false;
            food = generateFood();
        }
        // Check regular food collision
        else if (newHead == food) {
            score += 10;
            foodsEaten++;
            food = generateFood();
            // Chance to spawn special food after eating regular food
            spawnSpecialFood();
        } else {
            snake.pop_back();
        }
    }
    
    void handleInput() {
        if (terminal.kbhit()) {
            char key = terminal.getch();
            
            // Handle arrow keys (ANSI escape sequences)
            if (key == '\033') {
                terminal.getch(); // Skip '['
                char arrow = terminal.getch();
                switch (arrow) {
                    case 'A': // Up
                        if (direction.y == 0) {
                            direction = Position(0, -1);
                        }
                        break;
                    case 'B': // Down
                        if (direction.y == 0) {
                            direction = Position(0, 1);
                        }
                        break;
                    case 'C': // Right
                        if (direction.x == 0) {
                            direction = Position(1, 0);
                        }
                        break;
                    case 'D': // Left
                        if (direction.x == 0) {
                            direction = Position(-1, 0);
                        }
                        break;
                }
                return;
            }
            
            // Convert to lowercase
            if (key >= 'A' && key <= 'Z') {
                key = key + 32;
            }
            
            switch (key) {
                case 'w':
                    if (direction.y == 0) {
                        direction = Position(0, -1);
                    }
                    break;
                case 's':
                    if (direction.y == 0) {
                        direction = Position(0, 1);
                    }
                    break;
                case 'a':
                    if (direction.x == 0) {
                        direction = Position(-1, 0);
                    }
                    break;
                case 'd':
                    if (direction.x == 0) {
                        direction = Position(1, 0);
                    }
                    break;
                case 'p':
                    if (!gameOver) {
                        gamePaused = !gamePaused;
                    }
                    break;
                case 'r':
                    if (gameOver) {
                        reset();
                    }
                    break;
                case 'q':
                    showCursor();
                    exit(0);
                    break;
            }
        }
    }
    
    void reset() {
        snake.clear();
        snake.push_back(Position(BOARD_WIDTH / 2, BOARD_HEIGHT / 2));
        direction = Position(1, 0);
        score = 0;
        level = 1;
        foodsEaten = 0;
        gameOver = false;
        gamePaused = false;
        hasSpecialFood = false;
        food = generateFood();
        highScore = scoreTracker.getHighScore();
    }
    
public:
    SnakeGame() : direction(1, 0), score(0), highScore(0), level(1), 
                  foodsEaten(0), hasSpecialFood(false), specialFoodTimer(0),
                  gameOver(false), gamePaused(false) {
        srand(time(0));
        scoreTracker.loadScores();
        reset();
        hideCursor();
    }
    
    ~SnakeGame() {
        showCursor();
    }
    
    void run() {
        cout << "\n";
        cout << "  ============================================\n";
        cout << "  |         SNAKE GAME - C++                |\n";
        cout << "  ============================================\n";
        cout << "  Press any key to start...\n";
        while (!terminal.kbhit()) {
            usleep(100000); // 100ms
        }
        terminal.getch(); // Consume the key
        
        while (true) {
            handleInput();
            update();
            
            clearScreen();
            drawBoard();
            drawUI();
            
            // Progressive speed - game gets faster with level
            // Adjusted for direction to compensate terminal aspect ratio
            int currentSpeed = getAdjustedSpeed();
            usleep(currentSpeed);
        }
    }
};

int main() {
    SnakeGame game;
    game.run();
    
    return 0;
}