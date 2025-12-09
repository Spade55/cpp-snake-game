#ifndef SCORE_TRACKER_H
#define SCORE_TRACKER_H

#include <string>
#include <vector>

struct ScoreEntry {
    int score;
    std::string timestamp;
    
    ScoreEntry(int s, std::string t);
    bool operator>(const ScoreEntry& other) const;
};

class ScoreTracker {
private:
    std::string scoreFile;
    std::vector<ScoreEntry> scores;
    std::string getCurrentTimestamp();
    
public:
    ScoreTracker(const std::string& filename = "scores.txt");
    void loadScores();
    void saveScore(int score);
    int getHighScore();
    void displayLeaderboard();
    std::vector<ScoreEntry> getTopScores(int count = 10);
};

#endif

