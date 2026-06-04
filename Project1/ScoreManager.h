/// @file ScoreManager.h
/// @brief 成绩管理类声明

#ifndef SCOREMANAGER_H
#define SCOREMANAGER_H

#include <string>
#include <vector>

struct ScoreRecord {
    std::string playerName;
    int aValue;
    int totalTimeSeconds;
    int mergeCount;
    int totalMoves;
    double scoreRate;

    void calculateScoreRate() {
        scoreRate = (totalMoves > 0) ?
            static_cast<double>(mergeCount) / totalMoves : 0.0;
    }

    std::string toString() const;
};

class ScoreManager {
public:
    explicit ScoreManager();

    void addRecord(const ScoreRecord& record);
    void showLeaderboard(int filterByA = -1) const;

private:
    std::string getScoresFilePath() const;
    std::vector<ScoreRecord> loadFromFile() const;
    void saveToFile(const std::vector<ScoreRecord>& records) const;
};

#endif