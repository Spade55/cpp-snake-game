#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>

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
                file.ignore(); // Skip space
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
        
        // Keep only top 10 scores
        if (scores.size() > 10) {
            scores.resize(10);
        }
        
        // Save to file
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
    
    vector<ScoreEntry> getTopScores(int count = 10) {
        if (scores.size() > count) {
            return vector<ScoreEntry>(scores.begin(), scores.begin() + count);
        }
        return scores;
    }
};

// Example usage and testing
int main() {
    ScoreTracker tracker;
    
    cout << "Score Tracker Test\n";
    cout << "==================\n\n";
    
    // Display current leaderboard
    tracker.displayLeaderboard();
    
    // Test adding scores
    cout << "\nAdding test scores...\n";
    tracker.saveScore(150);
    tracker.saveScore(80);
    tracker.saveScore(200);
    tracker.saveScore(120);
    
    cout << "\nUpdated Leaderboard:\n";
    tracker.displayLeaderboard();
    
    cout << "\nHigh Score: " << tracker.getHighScore() << "\n";
    
    return 0;
}

