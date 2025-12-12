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
const char POISON_FOOD = '!';
const char WALL = '#';
const char EMPTY = ' ';
const int BASE_SPEED = 150000;
const int MIN_SPEED = 50000;
const int SPEED_STEP = 5000;
const int FOOD_SCORE = 10;
const int SPECIAL_SCORE = 50;
const int POISON_PENALTY = 20;
const int FOODS_PER_LEVEL = 5;
const int SPECIAL_FOOD_CHANCE = 20;
const int SPECIAL_FOOD_LIFETIME = 30;
const int SPECIAL_COOLDOWN_INIT = 20;
const int POISON_FOOD_CHANCE = 15;
const int POISON_COOLDOWN_INIT = 25;

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
    
    void moveTo(const Position& newHead, bool grow) {
        body.insert(body.begin(), newHead);
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
    
    void shrink(int amount) {
        while (amount > 0 && body.size() > 1) {
            body.pop_back();
            --amount;
        }
    }
    
    void setBodyAndDirection(const vector<Position>& newBody, const Position& newDir) {
        body = newBody;
        dir = newDir;
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
    Position poisonFood;
    int score;
    int highScore;
    int foodsEaten;
    bool hasSpecialFood;
    bool hasPoisonFood;
    int specialFoodTimer;
    int specialCooldown;
    int poisonCooldown;
    bool gameOver;
    bool gamePaused;
    TerminalInput terminal;
    ScoreTracker scoreTracker;
    bool easyMode;
    bool wrapMode;
    int speedMode;
    string saveFileName;
    int tickCount;
    
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
    
    char waitForKey() {
        char k = 0;
        while (k == 0) {
            if (terminal.kbhit()) {
                k = terminal.getch();
                if (k != 0) break;
            }
            usleep(50000);
        }
        return k;
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
            if (hasPoisonFood && newFood == poisonFood) {
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
    
    void spawnPoisonFood() {
        if (!hasPoisonFood && poisonCooldown <= 0) {
            if ((rand() % 100) < POISON_FOOD_CHANCE) {
                poisonFood = generateFood();
                hasPoisonFood = true;
                poisonCooldown = POISON_COOLDOWN_INIT;
            }
        }
    }
    
    void updatePoisonFood() {
        if (!hasPoisonFood) {
            if (poisonCooldown > 0) {
                --poisonCooldown;
            } else {
                spawnPoisonFood();
            }
        }
    }
    
    void updateFoods() {
        updateSpecialFood();
        updatePoisonFood();
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
        if (speedMode == 1) baseSpeed = static_cast<int>(baseSpeed * 1.5);
        else if (speedMode == 3) baseSpeed = static_cast<int>(baseSpeed * 0.7);
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
        char bodyChar = (tickCount % 2 == 0 ? SNAKE_BODY : 'o');
        
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
                } else if (hasPoisonFood && pos == poisonFood) {
                    cout << POISON_FOOD;
                } else {
                    bool isSnakeBody = false;
                    for (size_t i = 1; i < body.size(); i++) {
                        if (body[i] == pos) {
                            cout << bodyChar;
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
        
        cout << "  Mode: " << (easyMode ? "Easy " : "Normal ")
             << (wrapMode ? "| Wrap " : "| NoWrap ")
             << "| Speed: " << (speedMode == 1 ? "Slow" : (speedMode == 2 ? "Normal" : "Fast")) << "\n";
        
        if (hasSpecialFood && !gameOver && !gamePaused) {
            cout << "  " << SPECIAL_FOOD << " = " << SPECIAL_SCORE << " points (limited time)\n";
        }
        if (hasPoisonFood && !gameOver && !gamePaused) {
            cout << "  " << POISON_FOOD << " = -" << POISON_PENALTY << " points, snake shrinks\n";
        }
        
        if (gamePaused && !gameOver) {
            cout << "  [PAUSED] P=Resume | S=Save | L=Load | Q=Quit\n";
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
        } else if (!gamePaused) {
            cout << "  Controls: Arrow Keys or WASD | P=Pause | Q=Quit\n";
        }
    }
    
    bool checkCollision(const Position& head) {
        if (!wrapMode) {
            if (head.x <= 0 || head.x >= BOARD_WIDTH - 1 ||
                head.y <= 0 || head.y >= BOARD_HEIGHT - 1) {
                return true;
            }
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
        } else if (hasPoisonFood && head == poisonFood) {
            score -= POISON_PENALTY;
            if (score < 0) score = 0;
            hasPoisonFood = false;
            snake.shrink(3);
            poisonCooldown = POISON_COOLDOWN_INIT;
            return false;
        } else if (head == food) {
            score += FOOD_SCORE;
            foodsEaten++;
            food = generateFood();
            spawnSpecialFood();
            spawnPoisonFood();
            return true;
        }
        return false;
    }
    
    void handleCollisionInEasyMode() {
        score -= 50;
        if (score < 0) score = 0;
        snake = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
        foodsEaten = 0;
        hasSpecialFood = false;
        hasPoisonFood = false;
        specialFoodTimer = 0;
        specialCooldown = SPECIAL_COOLDOWN_INIT;
        poisonCooldown = POISON_COOLDOWN_INIT;
        food = generateFood();
    }
    
    void update() {
        if (gameOver || gamePaused) return;
        
        updateFoods();
        
        Position dir = snake.getDirection();
        Position newHead = snake.head();
        newHead.x += dir.x;
        newHead.y += dir.y;
        
        if (wrapMode) {
            if (newHead.x <= 0) newHead.x = BOARD_WIDTH - 2;
            else if (newHead.x >= BOARD_WIDTH - 1) newHead.x = 1;
            if (newHead.y <= 0) newHead.y = BOARD_HEIGHT - 2;
            else if (newHead.y >= BOARD_HEIGHT - 1) newHead.y = 1;
        }
        
        if (checkCollision(newHead)) {
            if (easyMode) {
                handleCollisionInEasyMode();
                return;
            } else {
                gameOver = true;
                if (score > highScore) {
                    highScore = score;
                }
                if (score > 0) {
                    scoreTracker.saveScore(score);
                }
                return;
            }
        }
        
        bool grew = handleFoodCollision(newHead);
        snake.moveTo(newHead, grew);
    }
    
    bool saveGame() {
        ofstream file(saveFileName);
        if (!file.is_open()) return false;
        
        file << score << " " << foodsEaten << " "
             << easyMode << " " << wrapMode << " " << speedMode << " "
             << hasSpecialFood << " " << specialFood.x << " " << specialFood.y << " "
             << specialFoodTimer << " " << specialCooldown << " "
             << hasPoisonFood << " " << poisonFood.x << " " << poisonFood.y << " "
             << poisonCooldown << "\n";
        
        file << food.x << " " << food.y << "\n";
        
        const vector<Position>& body = snake.getBody();
        file << body.size() << "\n";
        for (const auto& seg : body) {
            file << seg.x << " " << seg.y << "\n";
        }
        Position dir = snake.getDirection();
        file << dir.x << " " << dir.y << "\n";
        
        return true;
    }
    
    bool loadGame() {
        ifstream file(saveFileName);
        if (!file.is_open()) return false;
        
        int easyFlag, wrapFlag;
        int hasSpec, hasPois;
        file >> score >> foodsEaten
             >> easyFlag >> wrapFlag >> speedMode
             >> hasSpec >> specialFood.x >> specialFood.y
             >> specialFoodTimer >> specialCooldown
             >> hasPois >> poisonFood.x >> poisonFood.y
             >> poisonCooldown;
        
        easyMode = (easyFlag != 0);
        wrapMode = (wrapFlag != 0);
        hasSpecialFood = (hasSpec != 0);
        hasPoisonFood = (hasPois != 0);
        
        file >> food.x >> food.y;
        
        size_t len;
        file >> len;
        vector<Position> body;
        body.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            Position p;
            file >> p.x >> p.y;
            body.push_back(p);
        }
        Position dir;
        file >> dir.x >> dir.y;
        
        if (!file) return false;
        
        snake.setBodyAndDirection(body, dir);
        gameOver = false;
        gamePaused = false;
        tickCount = 0;
        highScore = scoreTracker.getHighScore();
        
        return true;
    }
    
    void handleInput() {
        if (!terminal.kbhit()) return;
        
        char key = terminal.getch();
        
        if (gamePaused && !gameOver) {
            if (key >= 'A' && key <= 'Z') key = key + 32;
            switch (key) {
                case 'p':
                    gamePaused = false;
                    return;
                case 's':
                    saveGame();
                    return;
                case 'l':
                    if (loadGame()) gamePaused = false;
                    return;
                case 'q':
                    showCursor();
                    exit(0);
            }
            return;
        }
        
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
                if (!gameOver) gamePaused = !gamePaused;
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
        hasPoisonFood = false;
        specialFoodTimer = 0;
        specialCooldown = SPECIAL_COOLDOWN_INIT;
        poisonCooldown = POISON_COOLDOWN_INIT;
        food = generateFood();
        tickCount = 0;
        highScore = scoreTracker.getHighScore();
    }
    
    void configureModes() {
        clearScreen();
        cout << "============================================\n";
        cout << "            SNAKE GAME SETTINGS\n";
        cout << "============================================\n\n";
        cout << "Select mode:\n";
        cout << "  1. Normal\n";
        cout << "  2. Easy (no death, penalty on hit)\n";
        cout << "  3. Wrap (through walls)\n";
        cout << "  4. Easy + Wrap\n\n";
        cout << "Press 1-4 to choose.\n";
        
        char c = 0;
        while (c < '1' || c > '4') {
            c = waitForKey();
        }
        if (c == '1') { easyMode = false; wrapMode = false; }
        else if (c == '2') { easyMode = true; wrapMode = false; }
        else if (c == '3') { easyMode = false; wrapMode = true; }
        else { easyMode = true; wrapMode = true; }
        
        clearScreen();
        cout << "============================================\n";
        cout << "            SPEED SETTINGS\n";
        cout << "============================================\n\n";
        cout << "Select speed:\n";
        cout << "  1. Slow\n";
        cout << "  2. Normal\n";
        cout << "  3. Fast\n\n";
        cout << "Press 1-3 to choose.\n";
        
        c = 0;
        while (c < '1' || c > '3') {
            c = waitForKey();
        }
        if (c == '1') speedMode = 1;
        else if (c == '2') speedMode = 2;
        else speedMode = 3;
        
        reset();
    }
    
public:
    SnakeGame()
        : snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2),
          score(0),
          highScore(0),
          foodsEaten(0),
          hasSpecialFood(false),
          hasPoisonFood(false),
          specialFoodTimer(0),
          specialCooldown(SPECIAL_COOLDOWN_INIT),
          poisonCooldown(POISON_COOLDOWN_INIT),
          gameOver(false),
          gamePaused(false),
          scoreTracker("scores.txt"),
          easyMode(false),
          wrapMode(false),
          speedMode(2),
          saveFileName("savegame.txt"),
          tickCount(0) {
        srand(time(0));
        scoreTracker.loadScores();
        reset();
        hideCursor();
    }
    
    ~SnakeGame() {
        showCursor();
    }
    
    void run() {
        clearScreen();
        cout << "  ============================================\n";
        cout << "  |         SNAKE GAME - C++                |\n";
        cout << "  ============================================\n\n";
        cout << "  N: New Game\n";
        cout << "  L: Load Saved Game\n";
        cout << "  Press N or L to continue...\n";
        
        char choice = 0;
        while (choice == 0) {
            choice = waitForKey();
            if (choice >= 'A' && choice <= 'Z') choice += 32;
            if (choice != 'n' && choice != 'l') choice = 0;
        }
        
        if (choice == 'l') {
            if (!loadGame()) {
                clearScreen();
                cout << "No valid save found. Starting new game.\n";
                usleep(1000000);
                configureModes();
            }
        } else {
            configureModes();
        }
        
        while (true) {
            handleInput();
            update();
            
            clearScreen();
            drawBoard();
            drawUI();
            
            int currentSpeed = getAdjustedSpeed();
            usleep(currentSpeed);
            ++tickCount;
        }
    }
};

int main() {
    SnakeGame game;
    game.run();
    return 0;
}

