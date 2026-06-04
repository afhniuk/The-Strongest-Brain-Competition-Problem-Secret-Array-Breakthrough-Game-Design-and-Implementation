/// @file ReplayManager.h
/// @brief 回放管理类声明

#ifndef REPLAYMANAGER_H
#define REPLAYMANAGER_H

#include "Types.h"
#include <string>
#include <vector>

struct ReplayStep {
    int stepNumber;
    Direction direction;
    MoveResult result;
    int mergeCount;

    std::string toString() const;
};

struct ReplayRecord {
    std::string playerName;
    int aValue;
    std::vector<std::string> initialBoard;
    std::vector<Position> initialCodes;
    std::vector<ReplayStep> steps;
    int finalMergeCount;
    int totalSteps;

    std::string toHeaderString() const;
};

class ReplayManager {
private:
    std::string replayFilePath;
    std::vector<ReplayRecord> replays;

public:
    explicit ReplayManager();

    void recordStep(ReplayRecord& record, Direction dir, MoveResult result, int mergeCount);

    void saveReplay(const ReplayRecord& record);

    void loadReplays();

    void showReplayList() const;

    const ReplayRecord* getReplayByIndex(int index) const;

    int getReplayCount() const { return static_cast<int>(replays.size()); }

private:
    void ensureDataDir(const std::string& dataDir);
};

#endif // REPLAYMANAGER_H
