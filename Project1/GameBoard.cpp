/// @file GameBoard.cpp
/// @brief 游戏盘面类实现，包含核心算法
/// @details 合并规则中数字相同时字母五进制加法：A=1,B=2,C=3,D=4,E=5

#include "GameBoard.h"
#include "LetterUtils.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <set>

// ==================== 构造与初始化 ====================

GameBoard::GameBoard()
    : a(0), size(0), mergeCount(0), totalMoves(0),
      initialized(false), gameStarted(false) {}

void GameBoard::initBoard(int aValue) {
    if (aValue < 1 || aValue > 10) {
        throw std::invalid_argument("难度系数 a 必须在 1-10 之间");
    }

    a = aValue;
    size = 5 * a;
    mergeCount = 0;
    totalMoves = 0;
    initialized = false;
    gameStarted = false;

    board.clear();
    board.resize(size, std::vector<Grid>(size));
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            board[r][c] = Grid::random();
        }
    }

    controlledCodes.clear();
    initialized = true;
}

void GameBoard::initBoardFromData(int aValue, const std::vector<std::string>& gridData) {
    if (aValue < 1 || aValue > 10) {
        throw std::invalid_argument("难度系数 a 必须在 1-10 之间");
    }

    a = aValue;
    size = 5 * a;
    mergeCount = 0;
    totalMoves = 0;
    initialized = false;
    gameStarted = false;

    board.clear();
    board.resize(size, std::vector<Grid>(size));

    int dataIdx = 0;
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (dataIdx < static_cast<int>(gridData.size())) {
                const std::string& cell = gridData[dataIdx];
                if (cell == ".." || cell.empty()) {
                    board[r][c] = Grid();
                } else if (cell.length() >= 2) {
                    char letter = cell[0];
                    int number = cell[1] - '0';
                    board[r][c] = Grid(letter, number);
                }
                ++dataIdx;
            } else {
                board[r][c] = Grid::random();
            }
        }
    }

    controlledCodes.clear();
    initialized = true;
}

bool GameBoard::selectInitialCodes(const std::vector<Position>& positions) {
    if (!initialized) return false;
    if (static_cast<int>(positions.size()) != a) return false;

    for (const auto& pos : positions) {
        if (!isValidPosition(pos)) return false;
        if (board[pos.row][pos.col].isEmpty) return false;
    }

    std::set<std::pair<int, int>> posSet;
    for (const auto& pos : positions) {
        auto [it, inserted] = posSet.insert({pos.row, pos.col});
        if (!inserted) return false;
    }

    controlledCodes = positions;
    gameStarted = true;
    return true;
}

// ==================== 核心算法：联动移动 ====================

MoveResult GameBoard::move(Direction dir) {
    if (!gameStarted) return MoveResult::FAILED;
    auto intents = collectIntents(dir);
    resolveConflicts(intents, dir);
    resolveTargetConflicts(intents);

    // 检查是否至少有一个操纵代码能够合并，否则不移动
    bool hasMerge = false;
    for (const auto& intent : intents) {
        if (intent.result != MoveResult::FAILED && intent.willMerge) {
            hasMerge = true;
            break;
        }
    }
    if (!hasMerge) return MoveResult::FAILED;

    ++totalMoves;
    executeIntents(intents);
    updateControlledCodes(intents);

    // 统计实际合并次数
    for (const auto& intent : intents) {
        if (intent.willMerge && intent.result != MoveResult::FAILED) {
            mergeCount++;
        }
    }
    return MoveResult::SUCCESS_MERGE;
}

std::vector<GameBoard::MoveIntent> GameBoard::collectIntents(Direction dir) {
    std::vector<MoveIntent> intents;

    for (int i = 0; i < static_cast<int>(controlledCodes.size()); ++i) {
        MoveIntent intent;
        intent.codeIndex = i;
        intent.from = controlledCodes[i];

        Position target = getNeighbor(intent.from, dir);

        if (!target.isValid()) {
            intent.result = MoveResult::FAILED;
            intents.push_back(intent);
            continue;
        }

        intent.to = target;
        Grid& targetGrid = board[target.row][target.col];
        Grid& currentGrid = board[intent.from.row][intent.from.col];

        if (targetGrid.isEmpty) {
            intent.result = MoveResult::SUCCESS_MOVE;
            intent.willMerge = false;
            intents.push_back(intent);
            continue;
        }

        int targetCtrlIdx = findControlledCodeIndex(target);

        if (targetCtrlIdx >= 0) {
            if (targetCtrlIdx == i) {
                intent.result = MoveResult::FAILED;
                intents.push_back(intent);
                continue;
            }

            if (canMerge(currentGrid, targetGrid)) {
                intent.result = MoveResult::SUCCESS_MERGE;
                intent.willMerge = true;
                intent.mergeTarget = target;
                intent.mergeResultGrid = performMerge(currentGrid, targetGrid);
            } else {
                intent.result = MoveResult::FAILED;
                intent.dependsOnIndex = targetCtrlIdx;
            }
        } else {
            if (canMerge(currentGrid, targetGrid)) {
                intent.result = MoveResult::SUCCESS_MERGE;
                intent.willMerge = true;
                intent.mergeTarget = target;
                intent.mergeResultGrid = performMerge(currentGrid, targetGrid);
            } else {
                intent.result = MoveResult::FAILED;
            }
        }

        intents.push_back(intent);
    }

    return intents;
}

void GameBoard::resolveConflicts(std::vector<MoveIntent>& intents, Direction dir) {
    int n = static_cast<int>(intents.size());
    bool changed;
    int maxIterations = n * 2;

    do {
        changed = false;

        for (int i = 0; i < n; ++i) {
            MoveIntent& intent = intents[i];

            if (intent.result != MoveResult::FAILED) continue;
            if (intent.dependsOnIndex < 0) continue;

            int depIdx = intent.dependsOnIndex;
            if (depIdx >= n) continue;

            MoveIntent& depIntent = intents[depIdx];

            if (depIntent.result == MoveResult::FAILED && depIntent.dependsOnIndex < 0) {
                Grid& currentGrid = board[intent.from.row][intent.from.col];
                Grid& targetGrid = board[depIntent.from.row][depIntent.from.col];

                if (canMerge(currentGrid, targetGrid)) {
                    intent.result = MoveResult::SUCCESS_MERGE;
                    intent.willMerge = true;
                    intent.to = depIntent.from;
                    intent.mergeTarget = depIntent.from;
                    intent.mergeResultGrid = performMerge(currentGrid, targetGrid);
                    intent.dependsOnIndex = -1;

                    depIntent.result = MoveResult::FAILED;
                    depIntent.dependsOnIndex = -2;
                    changed = true;
                } else {
                    intent.dependsOnIndex = -1;
                    changed = true;
                }
            } else if (depIntent.result == MoveResult::SUCCESS_MOVE ||
                       depIntent.result == MoveResult::SUCCESS_MERGE) {
                intent.result = MoveResult::SUCCESS_MOVE;
                intent.willMerge = false;
                intent.to = depIntent.from;
                intent.dependsOnIndex = -1;
                changed = true;
            }
        }

        --maxIterations;
    } while (changed && maxIterations > 0);

    for (auto& intent : intents) {
        if (intent.dependsOnIndex >= 0 && intent.result == MoveResult::FAILED) {
            intent.dependsOnIndex = -1;
        }
    }
}

void GameBoard::resolveTargetConflicts(std::vector<MoveIntent>& intents) {
    int n = static_cast<int>(intents.size());

    for (int i = 0; i < n; ++i) {
        if (intents[i].dependsOnIndex == -2) continue;
        if (intents[i].result == MoveResult::FAILED) continue;

        for (int j = i + 1; j < n; ++j) {
            if (intents[j].dependsOnIndex == -2) continue;
            if (intents[j].result == MoveResult::FAILED) continue;

            if (intents[i].to == intents[j].to) {
                bool iIsMerge = intents[i].willMerge;
                bool jIsMerge = intents[j].willMerge;

                if (iIsMerge && !jIsMerge) {
                    intents[j].result = MoveResult::FAILED;
                } else if (!iIsMerge && jIsMerge) {
                    intents[i].result = MoveResult::FAILED;
                } else {
                    intents[j].result = MoveResult::FAILED;
                }
            }
        }
    }
}

void GameBoard::executeIntents(const std::vector<MoveIntent>& intents) {
    // 分两个阶段执行，避免读写冲突
    // 第一阶段：记录所有需要写入的结果
    std::vector<std::pair<Position, Grid>> writeResults;
    std::vector<Position> positionsToClear;

    for (const auto& intent : intents) {
        if (intent.dependsOnIndex == -2) {
            // 被其他操纵代码合并消费，原位置清空
            positionsToClear.push_back(intent.from);
            continue;
        }
        if (intent.result == MoveResult::FAILED) {
            // 失败意图不改变盘面
            continue;
        }
        // 成功移动或合并：原位置清空
        positionsToClear.push_back(intent.from);
        if (intent.willMerge) {
            // 合并：目标位置写入合并结果
            writeResults.push_back({intent.mergeTarget, intent.mergeResultGrid});
        } else {
            // 纯移动：把原格子内容搬到目标位置
            writeResults.push_back({intent.to, board[intent.from.row][intent.from.col]});
        }
    }

    // 第二阶段：清空原位置，写入新内容
    for (const auto& pos : positionsToClear) {
        board[pos.row][pos.col].clear();
    }
    for (const auto& pr : writeResults) {
        board[pr.first.row][pr.first.col] = pr.second;
    }
}

bool GameBoard::canMerge(const Grid& a, const Grid& b) {
    if (a.isEmpty || b.isEmpty) return false;
    return a.hasSameLetter(b) || a.hasSameNumber(b) || a.isIdentical(b);
}

Grid GameBoard::performMerge(const Grid& mover, const Grid& target) {
    Grid result;
    result.isEmpty = false;

    if (mover.isIdentical(target)) {
        // 字母数字均相同：保持不变
        result.letter = mover.letter;
        result.number = mover.number;
    } else if (mover.hasSameLetter(target)) {
        // 字母相同：数字相加保留个位，字母不变
        result.letter = mover.letter;
        result.number = (mover.number + target.number) % 10;
    } else if (mover.hasSameNumber(target)) {
        // 数字相同：字母按五进制(1-5)相加，数字不变
        // 例如：A3+C3→D3（A=1,C=3,1+3=4→D）
        result.letter = LetterUtils::addLetters(mover.letter, target.letter);
        result.number = mover.number;
    } else {
        result.letter = mover.letter;
        result.number = mover.number;
    }

    return result;
}

void GameBoard::updateControlledCodes(const std::vector<MoveIntent>& intents) {
    controlledCodes.clear();

    for (const auto& intent : intents) {
        if (intent.result == MoveResult::FAILED) {
            // 被标记为 -2 的表示被其他代码合并消费了，不再保留
            if (intent.dependsOnIndex == -2) continue;
            // 其他失败意图：保持在原位置
            controlledCodes.push_back(intent.from);
        } else {
            // 成功移动或合并：新位置成为操纵代码
            controlledCodes.push_back(intent.to);
        }
    }
}

// ==================== 游戏状态判断 ====================

bool GameBoard::isGameOver() {
    if (!gameStarted) return false;
    if (controlledCodes.empty()) return true;
    static const Direction allDirs[] = {
        Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT
    };
    for (const auto& codePos : controlledCodes) {
        const Grid& currentGrid = board[codePos.row][codePos.col];
        for (Direction dir : allDirs) {
            Position target = getNeighbor(codePos, dir);
            if (!target.isValid()) continue;
            const Grid& targetGrid = board[target.row][target.col];
            // 空格子不能合并，忽略
            if (targetGrid.isEmpty) continue;
            // 只要有一个可合并的邻居，游戏未结束
            if (canMerge(currentGrid, targetGrid)) return false;
        }
    }
    return true;
}

// ==================== 辅助方法 ====================

Position GameBoard::getNeighbor(const Position& pos, Direction dir) const {
    int r = pos.row;
    int c = pos.col;

    switch (dir) {
        case Direction::UP:    r--; break;
        case Direction::DOWN:  r++; break;
        case Direction::LEFT:  c--; break;
        case Direction::RIGHT: c++; break;
    }

    if (r >= 0 && r < size && c >= 0 && c < size) {
        return Position(r, c);
    }
    return Position();
}

bool GameBoard::isValidPosition(const Position& pos) const {
    return pos.row >= 0 && pos.row < size && pos.col >= 0 && pos.col < size;
}

int GameBoard::findControlledCodeIndex(const Position& pos) const {
    for (int i = 0; i < static_cast<int>(controlledCodes.size()); ++i) {
        if (controlledCodes[i] == pos) return i;
    }
    return -1;
}

// ==================== 显示与导出 ====================

std::string GameBoard::toString(bool highlightControlled) const {
    std::ostringstream oss;

    oss << "    ";
    for (int c = 0; c < size; ++c) {
        oss << "  " << (c < 10 ? " " : "") << c << "   ";
    }
    oss << "\n";

    oss << "    +";
    for (int c = 0; c < size; ++c) oss << "------+";
    oss << "\n";

    for (int r = 0; r < size; ++r) {
        oss << " " << (r < 10 ? " " : "") << r << " |";
        for (int c = 0; c < size; ++c) {
            const Grid& g = board[r][c];
            if (g.isEmpty) {
                oss << " [  ] |";
            } else {
                bool isCtrl = false;
                if (highlightControlled) {
                    for (const auto& ctrl : controlledCodes) {
                        if (ctrl.row == r && ctrl.col == c) {
                            isCtrl = true;
                            break;
                        }
                    }
                }
                if (isCtrl) {
                    oss << " *" << g.letter << g.number << "* |";
                } else {
                    oss << " [" << g.letter << g.number << "] |";
                }
            }
        }
        oss << "\n";
        oss << "    +";
        for (int c = 0; c < size; ++c) oss << "------+";
        oss << "\n";
    }

    oss << "\n图例：*X#* = 操纵代码，[X#] = 普通代码，[  ] = 空格\n";
    oss << "操纵代码数量：" << controlledCodes.size() << "\n";

    return oss.str();
}

std::vector<std::string> GameBoard::exportBoardData() const {
    std::vector<std::string> data;
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            const Grid& g = board[r][c];
            if (g.isEmpty) {
                data.push_back("..");
            } else {
                data.push_back(std::string(1, g.letter) + std::to_string(g.number));
            }
        }
    }
    return data;
}

std::string GameBoard::controlledCodesToString() const {
    std::ostringstream oss;
    for (size_t i = 0; i < controlledCodes.size(); ++i) {
        if (i > 0) oss << ",";
        oss << controlledCodes[i].row << "-" << controlledCodes[i].col;
    }
    return oss.str();
}

void GameBoard::controlledCodesFromString(const std::string& str) {
    controlledCodes.clear();
    if (str.empty()) return;

    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, ',')) {
        auto dashPos = token.find('-');
        if (dashPos != std::string::npos) {
            int r = std::stoi(token.substr(0, dashPos));
            int c = std::stoi(token.substr(dashPos + 1));
            if (isValidPosition(Position(r, c)) && !board[r][c].isEmpty) {
                controlledCodes.push_back(Position(r, c));
            }
        }
    }
    gameStarted = !controlledCodes.empty();
}
