/// @file ReplayManager.cpp
/// @brief 回放管理类实现

#include "ReplayManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include "StringUtils.h"

ReplayManager::ReplayManager() {
    // 使用当前工作目录统一路径，与 saveReplay/loadReplays 保持一致
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path dataPath = currentPath / "data";
    replayFilePath = (dataPath / "replay.txt").string();

    // 确保 data 目录存在
    std::error_code ec;
    std::filesystem::create_directories(dataPath, ec);

    loadReplays();

}

void ReplayManager::ensureDataDir(const std::string & dataDir) {
    std::error_code ec;
    if (!std::filesystem::exists(dataDir)) {
        std::filesystem::create_directories(dataDir, ec);
    }
}

void ReplayManager::recordStep(ReplayRecord& record, Direction dir,
                                MoveResult result, int mergeCount) {
    ReplayStep step;
    step.stepNumber = static_cast<int>(record.steps.size()) + 1;
    step.direction = dir;
    step.result = result;
    step.mergeCount = mergeCount;
    record.steps.push_back(step);
}


void ReplayManager::showReplayList() const {
    std::cout << "\n";
    std::cout << "╔═════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    回 放 列 表                              ║\n";
    std::cout << "╠════╤══════════════╤══════╤══════╤════════╤══════════════════╣\n";
    std::cout << "║序号│玩家名        │a 值  │总步数│最终合并│得分率            ║\n";
    std::cout << "╟────┼──────────────┼──────┼──────┼────────┼──────────────────╢\n";

    if (replays.empty()) {
        std::cout << "║                   暂无回放记录                              ║\n";
    } else {
        for (size_t i = 0; i < replays.size(); ++i) {
            const auto& rec = replays[i];
            double rate = (rec.totalSteps > 0) ?
                          static_cast<double>(rec.finalMergeCount) / rec.totalSteps : 0.0;
            std::string playerName = StringUtils::padToWidth(rec.playerName, 14);
            std::cout << "║"
                      << std::setw(3) << (i + 1) << " │"
                      <<playerName << "│"
                      << std::setw(5) << rec.aValue << " │"
                      << std::setw(5) << rec.totalSteps << " │"
                      << std::setw(7) << rec.finalMergeCount << " │"
                      << std::setw(17) << std::fixed << std::setprecision(3)
                      << rate << " ║\n";
        }
    }
    std::cout << "╚════╧══════════════╧══════╧══════╧════════╧══════════════════╝\n\n";
}

const ReplayRecord* ReplayManager::getReplayByIndex(int index) const {
    if (index >= 0 && index < static_cast<int>(replays.size())) {
        return &replays[index];
    }
    return nullptr;
}

std::string ReplayStep::toString() const {
    std::ostringstream oss;
    oss << "步骤 " << stepNumber << ": "
        << directionToString(direction) << " → "
        << (result == MoveResult::SUCCESS_MERGE ? "合并成功" :
            result == MoveResult::SUCCESS_MOVE ? "移动成功" : "失败")
        << " (累计合并: " << mergeCount << ")";
    return oss.str();
}

std::string ReplayRecord::toHeaderString() const {
    std::ostringstream oss;
    oss << "NAME:" << playerName << ":" << aValue << ":" << "DATE";
    return oss.str();
}

void ReplayManager::saveReplay(const ReplayRecord& record) {

    // 使用当前工作目录
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path dataPath = currentPath / "data";
    std::filesystem::path filePath = dataPath / "replay.txt";
    std::string finalPath = filePath.string();


    // 确保目录存在
    std::error_code ec;
    std::filesystem::create_directories(dataPath, ec);

    // 以追加模式打开文件（保留已有回放）
    std::ofstream file(finalPath, std::ios::out | std::ios::app);
    if (!file.is_open()) {
        std::cout << "[saveReplay] 错误：无法打开文件 " << finalPath << "\n";
        return;
    }

    // 写入回放记录
    file << "=====REPLAY=====\n";
    file << "NAME:" << record.playerName << ":" << record.aValue << "\n";
    file << "BOARD\n";

    int size = 5 * record.aValue;
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            int idx = r * size + c;
            if (idx < static_cast<int>(record.initialBoard.size())) {
                file << record.initialBoard[idx];
            }
            else {
                file << "..";
            }
            if (c < size - 1) file << " ";
        }
        file << "\n";
    }

    file << "CODES\n";
    for (size_t i = 0; i < record.initialCodes.size(); ++i) {
        if (i > 0) file << ",";
        file << record.initialCodes[i].row << "-" << record.initialCodes[i].col;
    }
    file << "\n";

    file << "STEPS\n";
    for (const auto& step : record.steps) {
        file << step.stepNumber << ":"
            << directionToChar(step.direction) << ":"
            << (step.result == MoveResult::SUCCESS_MERGE ? "M" :
                step.result == MoveResult::SUCCESS_MOVE ? "V" : "F") << ":"
            << step.mergeCount << "\n";
    }
    file << "=====END=====\n\n";

    file.close();
    loadReplays();
}

void ReplayManager::loadReplays() {
    replays.clear();

    // 使用与 saveReplay 相同的路径逻辑
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path dataPath = currentPath / "data";
    std::filesystem::path filePath = dataPath / "replay.txt";
    std::string finalPath = filePath.string();

    std::ifstream file(finalPath);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    ReplayRecord currentRec;
    int state = 0; // 0=等待REPLAY开始, 1=在REPLAY中(读头部), 2=读BOARD, 3=读CODES, 4=读STEPS

    while (std::getline(file, line)) {
        // 去除行尾回车
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }

        if (line.empty()) continue;

        if (line == "=====REPLAY=====") {
            currentRec = ReplayRecord();
            currentRec.steps.clear();
            currentRec.initialBoard.clear();
            currentRec.initialCodes.clear();
            state = 1;
            continue;
        }

        if (line == "=====END=====") {
            if (!currentRec.playerName.empty() && !currentRec.initialBoard.empty()) {
                currentRec.totalSteps = static_cast<int>(currentRec.steps.size());
                if (!currentRec.steps.empty()) {
                    currentRec.finalMergeCount = currentRec.steps.back().mergeCount;
                }
                replays.push_back(currentRec);
            }
            state = 0;
            continue;
        }

        if (state == 1) {
            // 解析头部 NAME:玩家名:a值
            if (line.substr(0, 4) == "NAME") {
                size_t pos1 = line.find(':');
                size_t pos2 = line.find(':', pos1 + 1);
                if (pos1 != std::string::npos && pos2 != std::string::npos) {
                    currentRec.playerName = line.substr(pos1 + 1, pos2 - pos1 - 1);
                    currentRec.aValue = std::stoi(line.substr(pos2 + 1));
                }
            }
            else if (line == "BOARD") {
                state = 2;
            }
        }
        else if (state == 2) {
            // 读取盘面数据
            if (line == "CODES") {
                state = 3;
                continue;
            }

            std::istringstream iss(line);
            std::string cell;
            while (iss >> cell) {
                currentRec.initialBoard.push_back(cell);
            }
        }
        else if (state == 3) {
            // 读取操纵代码
            if (line == "STEPS") {
                state = 4;
                continue;
            }

            std::istringstream iss(line);
            std::string token;
            while (std::getline(iss, token, ',')) {
                size_t dash = token.find('-');
                if (dash != std::string::npos) {
                    try {
                        int r = std::stoi(token.substr(0, dash));
                        int c = std::stoi(token.substr(dash + 1));
                        currentRec.initialCodes.push_back(Position(r, c));
                    }
                    catch (...) {
                        // 忽略解析错误
                    }
                }
            }
        }
        else if (state == 4) {
            // 读取步骤
            ReplayStep step;
            size_t pos1 = line.find(':');
            if (pos1 != std::string::npos) {
                try {
                    step.stepNumber = std::stoi(line.substr(0, pos1));
                    size_t pos2 = line.find(':', pos1 + 1);
                    if (pos2 != std::string::npos) {
                        char dirChar = line[pos1 + 1];
                        step.direction = charToDirection(dirChar);
                        size_t pos3 = line.find(':', pos2 + 1);
                        if (pos3 != std::string::npos) {
                            char resultChar = line[pos2 + 1];
                            step.result = (resultChar == 'M') ? MoveResult::SUCCESS_MERGE :
                                (resultChar == 'V') ? MoveResult::SUCCESS_MOVE :
                                MoveResult::FAILED;
                            step.mergeCount = std::stoi(line.substr(pos3 + 1));
                            currentRec.steps.push_back(step);
                        }
                    }
                }
                catch (...) {
                    // 忽略解析错误
                }
            }
        }
    }

    file.close();
}