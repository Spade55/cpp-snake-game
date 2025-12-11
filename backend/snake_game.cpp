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

const int BOARD_WIDTH = 30;
const int BOARD_HEIGHT = 20;
const char SNAKE_BODY = 'O';
const char SNAKE_HEAD = '@';
const char FOOD = '*';
const char SPECIAL_FOOD = '$';
const char WALL = '#';
const char EMPTY = ' ';
const int BASE_SPEED = 150000;
const int MIN_SPEED = 50000;
const int SPEED_STEP = 5000;
const int FOOD_SCORE = 10;
const int SPECIAL_SCORE = 50;
const int FOODS_PER_LEVEL = 5;
const int SPECIAL_FOOD_CHANCE = 20;
const int SPECIAL_FOOD_LIFETIME = 30;
const int SPECIAL_COOLDOWN_INIT = 20;

struct Position {
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

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

class Snake {
public:
    Snake(int startX = BOARD_WIDTH / 2, int startY = BOARD_HEIGHT / 2) {
        body.push_back(Position(startX, startY));
        dir = Position(1, 0);
    }
    
    const vector<Position>& getBody() const { return body; }
    const Position& head() const { return body[0]; }
    const Position& getDirection() const { return dir; }
    
    void setDirection(const Position& newDir) {
        if (newDir.x == 0 && newDir.y == 0) return;
        if (body.size() > 1) {
            Position nextHead = head();
            nextHead.x += newDir.x;
            nextHead.y += newDir.y;
            if (nextHead == body[1]) {
                return;
            }
        }
        dir = newDir;
    }
    
    Position nextHead() const {
        Position nh = head();
        nh.x += dir.x;
        nh.y += dir.y;
        return nh;
    }
    
    void move(bool grow) {
        Position nh = nextHead();
        body.insert(body.begin(), nh);
        if (!grow && !body.empty()) {
            body.pop_back();
        }
    }
    
    bool hitsSelf(const Position& p) const {
        for (size_t i = 1; i < body.size(); ++i) {
            if (body[i] == p) return true;
        }
        return false;
    }
    
private:
    vector<Position> body;
    Position dir;
};

class SnakeGame {
private:
    Snake snake;
    Position food;
    Position specialFood;
    int score;
    int highScore;
    int foodsEaten;
    bool hasSpecialFood;
    int specialFoodTimer;
    int specialCooldown;
    bool gameOver;
    bool gamePaused;
    TerminalInput terminal;
    ScoreTracker scoreTracker;
    
    void clearScreen() {
        cout << "\033[2J\033[H";
    }
    
    void setCursorPosition(int x, int y) {
        cout << "\033[" << (y + 1) << ";" << (x + 1) << "H";
    }
    
    void hideCursor() {
        cout << "\033[?25l";
    }
    
    void showCursor() {
        cout << "\033[?25h";
    }
    
    Position generateFood() {
        Position newFood;
        bool valid = false;
        const vector<Position>& body = snake.getBody();
        while (!valid) {
            newFood.x = rand() % (BOARD_WIDTH - 2) + 1;
            newFood.y = rand() % (BOARD_HEIGHT - 2) + 1;
            valid = true;
            for (const auto& segment : body) {
                if (segment == newFood) {
                    valid = false;
                    break;
                }
            }
            if (hasSpecialFood && newFood == specialFood) {
                valid = false;
            }
        }
        return newFood;
    }
    
    void spawnSpecialFood() {
        if (!hasSpecialFood && specialCooldown <= 0) {
            if ((rand() % 100) < SPECIAL_FOOD_CHANCE) {
                specialFood = generateFood();
                hasSpecialFood = true;
                specialFoodTimer = SPECIAL_FOOD_LIFETIME;
                specialCooldown = SPECIAL_COOLDOWN_INIT;
            }
        }
    }
    
    void updateSpecialFood() {
        if (hasSpecialFood) {
            if (--specialFoodTimer <= 0) {
                hasSpecialFood = false;
            }
        } else {
            if (specialCooldown > 0) {
                --specialCooldown;
            } else {
                spawnSpecialFood();
            }
        }
    }
    
    int getLevel() const {
        return foodsEaten / FOODS_PER_LEVEL + 1;
    }
    
    int calculateSpeed() const {
        int level = getLevel();
        int speed = BASE_SPEED - (level - 1) * SPEED_STEP;
        if (speed < MIN_SPEED) speed = MIN_SPEED;
        return speed;
    }
    
    int getAdjustedSpeed() const {
        int baseSpeed = calculateSpeed();
        if (snake.getDirection().y != 0) {
            return static_cast<int>(baseSpeed * 1.8);
        }
        return baseSpeed;
    }
    
    void drawBoard() {
        setCursorPosition(0, 0);
        for (int i = 0; i < BOARD_WIDTH; i++) {
            cout << WALL;
        }
        cout << endl;
        
        const vector<Position>& body = snake.getBody();
        
        for (int y = 1; y < BOARD_HEIGHT - 1; y++) {
            cout << WALL;
            for (int x = 1; x < BOARD_WIDTH - 1; x++) {
                Position pos(x, y);
                if (pos == body[0]) {
                    cout << SNAKE_HEAD;
                } else if (pos == food) {
                    cout << FOOD;
                } else if (hasSpecialFood && pos == specialFood) {
                    cout << SPECIAL_FOOD;
                } else {
                    bool isSnakeBody = false;
                    for (size_t i = 1; i < body.size(); i++) {
                        if (body[i] == pos) {
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
            cout << WALL;
            cout << endl;
        }
        
        for (int i = 0; i < BOARD_WIDTH; i++) {
            cout << WALL;
        }
        cout << endl;
    }
    
    void drawUI() {
        int level = getLevel();
        cout << "\n";
        cout << "  Score: " << setw(6) << score;
        cout << "  |  High Score: " << setw(6) << highScore;
        cout << "  |  Level: " << setw(3) << level;
        cout << "  |  Length: " << setw(3) << snake.getBody().size();
        cout << "\n";
        
        if (hasSpecialFood && !gameOver && !gamePaused) {
            cout << "  [BONUS FOOD AVAILABLE! " << SPECIAL_FOOD << " = " << SPECIAL_SCORE << " points]\n";
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
        if (head.x <= 0 || head.x >= BOARD_WIDTH - 1 ||
            head.y <= 0 || head.y >= BOARD_HEIGHT - 1) {
            return true;
        }
        if (snake.hitsSelf(head)) {
            return true;
        }
        return false;
    }
    
    bool handleFoodCollision(const Position& head) {
        if (hasSpecialFood && head == specialFood) {
            score += SPECIAL_SCORE;
            foodsEaten++;
            hasSpecialFood = false;
            food = generateFood();
            return true;
        } else if (head == food) {
            score += FOOD_SCORE;
            foodsEaten++;
            food = generateFood();
            spawnSpecialFood();
            return true;
        }
        return false;
    }
    
    void update() {
        if (gameOver || gamePaused) return;
        
        updateSpecialFood();
        
        Position newHead = snake.nextHead();
        
        if (checkCollision(newHead)) {
            gameOver = true;
            if (score > highScore) {
                highScore = score;
            }
            if (score > 0) {
                scoreTracker.saveScore(score);
            }
            return;
        }
        
        bool grew = handleFoodCollision(newHead);
        snake.move(grew);
    }
    
    void handleInput() {
        if (!terminal.kbhit()) return;
        
        char key = terminal.getch();
        
        if (key == '\033') {
            terminal.getch();
            char arrow = terminal.getch();
            switch (arrow) {
                case 'A':
                    snake.setDirection(Position(0, -1));
                    break;
                case 'B':
                    snake.setDirection(Position(0, 1));
                    break;
                case 'C':
                    snake.setDirection(Position(1, 0));
                    break;
                case 'D':
                    snake.setDirection(Position(-1, 0));
                    break;
            }
            return;
        }
        
        if (key >= 'A' && key <= 'Z') {
            key = key + 32;
        }
        
        switch (key) {
            case 'w':
                snake.setDirection(Position(0, -1));
                break;
            case 's':
                snake.setDirection(Position(0, 1));
                break;
            case 'a':
                snake.setDirection(Position(-1, 0));
                break;
            case 'd':
                snake.setDirection(Position(1, 0));
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
    
    void reset() {
        snake = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
        score = 0;
        foodsEaten = 0;
        gameOver = false;
        gamePaused = false;
        hasSpecialFood = false;
        specialFoodTimer = 0;
        specialCooldown = 0;
        food = generateFood();
        highScore = scoreTracker.getHighScore();
    }
    
public:
    SnakeGame()
        : snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2),
          score(0),
          highScore(0),
          foodsEaten(0),
          hasSpecialFood(false),
          specialFoodTimer(0),
          specialCooldown(0),
          gameOver(false),
          gamePaused(false) {
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
            usleep(100000);
        }
        terminal.getch();
        
        while (true) {
            handleInput();
            update();
            
            clearScreen();
            drawBoard();
            drawUI();
            
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
