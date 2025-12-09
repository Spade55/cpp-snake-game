#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

using namespace std;

// Score tracker implementation (inline to avoid linking issues)
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
    
    void displayLeaderboard() {
        cout << "\n";
        cout << "  ============================================\n";
        cout << "  |         LEADERBOARD (Top 10)            |\n";
        cout << "  ============================================\n";
        cout << "  Rank  Score    Date & Time\n";
        cout << "  --------------------------------------------\n";
        
        int rank = 1;
        for (const auto& entry : scores) {
            cout << "  " << setw(3) << rank++ << "   "
                 << setw(6) << entry.score << "   "
                 << entry.timestamp << "\n";
        }
        
        if (scores.empty()) {
            cout << "  No scores recorded yet.\n";
        }
        
        cout << "  ============================================\n";
    }
};

using namespace std;

// Simple terminal input handler for menu
class MenuInput {
private:
    struct termios oldTermios;
    bool initialized;
    
public:
    MenuInput() : initialized(false) {
        tcgetattr(STDIN_FILENO, &oldTermios);
        struct termios newTermios = oldTermios;
        newTermios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
        initialized = true;
    }
    
    ~MenuInput() {
        if (initialized) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
        }
    }
    
    char getKey() {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            return ch;
        }
        return 0;
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
    
    void waitForKey() {
        // Make blocking temporarily to wait for key
        int flags = fcntl(STDIN_FILENO, F_GETFL);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
        
        // Wait for any key
        char ch;
        read(STDIN_FILENO, &ch, 1);
        
        // Restore non-blocking
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
};

class GameMenu {
private:
    ScoreTracker scoreTracker;
    MenuInput input;
    int selectedOption;
    
    void clearScreen() {
        system("clear");
    }
    
    void displayMenu() {
        clearScreen();
        cout << "\n";
        cout << "  ============================================\n";
        cout << "  |         SNAKE GAME - MAIN MENU          |\n";
        cout << "  ============================================\n";
        cout << "\n";
        
        cout << "  " << (selectedOption == 0 ? ">> " : "   ") << "1. Start New Game\n";
        cout << "  " << (selectedOption == 1 ? ">> " : "   ") << "2. View Leaderboard\n";
        cout << "  " << (selectedOption == 2 ? ">> " : "   ") << "3. Quit\n";
        
        cout << "\n";
        cout << "  High Score: " << setw(6) << scoreTracker.getHighScore() << "\n";
        cout << "\n";
        cout << "  Use Arrow Keys or W/S to navigate, Enter to select\n";
    }
    
    void displayLeaderboard() {
        clearScreen();
        scoreTracker.loadScores(); // Reload to get latest scores
        scoreTracker.displayLeaderboard();
        cout << "\n  Press any key to return to menu...\n";
        input.waitForKey();
    }
    
    int runGame() {
        clearScreen();
        cout << "\n";
        cout << "  ============================================\n";
        cout << "  |         Starting Game...                |\n";
        cout << "  ============================================\n";
        cout << "  Launching Snake Game...\n\n";
        usleep(500000); // Brief pause
        
        // Restore terminal settings before launching game
        struct termios old;
        tcgetattr(STDIN_FILENO, &old);
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        int flags = fcntl(STDIN_FILENO, F_GETFL);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
        
        // Launch the game
        int result = system("./snake_game");
        
        // Restore non-blocking mode for menu
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        
        // Return a placeholder score (the game saves its own score)
        return 0;
    }
    
public:
    GameMenu() : selectedOption(0) {
        scoreTracker.loadScores();
    }
    
    void run() {
        while (true) {
            displayMenu();
            
            // Wait for input - use blocking mode for better responsiveness
            char key = 0;
            while (key == 0) {
                if (input.kbhit()) {
                    key = input.getKey();
                    break;
                }
                usleep(50000); // 50ms delay
            }
            
            if (key == '\033') {
                // Arrow key sequence
                if (input.kbhit()) {
                    input.getKey(); // Skip '['
                    if (input.kbhit()) {
                        char arrow = input.getKey();
                        switch (arrow) {
                            case 'A': // Up
                                selectedOption = (selectedOption - 1 + 3) % 3;
                                break;
                            case 'B': // Down
                                selectedOption = (selectedOption + 1) % 3;
                                break;
                        }
                    }
                }
            } else if (key == 'w' || key == 'W') {
                selectedOption = (selectedOption - 1 + 3) % 3;
            } else if (key == 's' || key == 'S') {
                selectedOption = (selectedOption + 1) % 3;
            } else if (key == '\n' || key == '\r' || key == ' ') {
                // Enter or Space
                switch (selectedOption) {
                    case 0: // Start Game
                        {
                            int finalScore = runGame();
                            if (finalScore > 0) {
                                scoreTracker.saveScore(finalScore);
                            }
                        }
                        break;
                    case 1: // Leaderboard
                        displayLeaderboard();
                        break;
                    case 2: // Quit
                        clearScreen();
                        cout << "\n  Thanks for playing!\n\n";
                        return;
                }
            } else if (key == '1') {
                int finalScore = runGame();
                if (finalScore > 0) {
                    scoreTracker.saveScore(finalScore);
                }
            } else if (key == '2') {
                displayLeaderboard();
            } else if (key == '3' || key == 'q' || key == 'Q') {
                clearScreen();
                cout << "\n  Thanks for playing!\n\n";
                return;
            }
        }
    }
};

int main() {
    GameMenu menu;
    menu.run();
    return 0;
}

