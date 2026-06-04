/// @file GameBoard.h
/// @brief 游戏盘面类声明，管理盘面数据和核心游戏逻辑

#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include "Types.h"
#include "Grid.h"
#include <vector>
#include <string>
#include <functional>

class GameBoard {
public:
    /// @brief 移动意向结构体
    struct MoveIntent {
        int codeIndex;
        Position from;
        Position to;
        MoveResult result;
        bool willMerge;
        Position mergeTarget;
        int dependsOnIndex;
        Grid mergeResultGrid;

        MoveIntent() : codeIndex(-1), result(MoveResult::FAILED),
                       willMerge(false), dependsOnIndex(-1) {}
    };

private:
    int a;                                      ///< 难度系数（1-10）
    int size;                                   ///< 盘面边长 = 5 * a
    std::vector<std::vector<Grid>> board;       ///< 盘面数据
    std::vector<Position> controlledCodes;      ///< 当前操纵代码位置列表
    int mergeCount;                             ///< 累计合并次数（得分）
    int totalMoves;                             ///< 总操作次数
    bool initialized;
    bool gameStarted;

public:
    GameBoard();

    int getA() const { return a; }
    int getSize() const { return size; }
    int getMergeCount() const { return mergeCount; }
    int getTotalMoves() const { return totalMoves; }
    const std::vector<std::vector<Grid>>& getBoard() const { return board; }
    const std::vector<Position>& getControlledCodes() const { return controlledCodes; }
    bool isGameStarted() const { return gameStarted; }

    /// @brief 初始化盘面（随机生成）
    void initBoard(int aValue);

    /// @brief 使用预设数据初始化盘面（用于加载题目）
    void initBoardFromData(int aValue, const std::vector<std::string>& gridData);

    /// @brief 选择初始操纵代码
    bool selectInitialCodes(const std::vector<Position>& positions);

    bool hasEnoughCodes() const { return static_cast<int>(controlledCodes.size()) == a; }

    /// @brief 执行联动移动（核心算法）
    MoveResult move(Direction dir);

    /// @brief 判断游戏是否结束
    bool isGameOver();

    /// @brief 获取盘面的字符串表示
    std::string toString(bool highlightControlled = true, bool useColor = false) const;

    /// @brief 导出盘面数据
    std::vector<std::string> exportBoardData() const;

    /// @brief 操控代码位置序列化
    std::string controlledCodesToString() const;

    /// @brief 从字符串恢复操控代码位置
    void controlledCodesFromString(const std::string& str);

private:
    Position getNeighbor(const Position& pos, Direction dir) const;
    bool isValidPosition(const Position& pos) const;
    int findControlledCodeIndex(const Position& pos) const;

    /// @brief 收集所有操纵代码的移动意向（第一阶段）
    std::vector<MoveIntent> collectIntents(Direction dir);

    /// @brief 解决操纵代码之间的冲突（第二阶段）
    void resolveConflicts(std::vector<MoveIntent>& intents, Direction dir);

    /// @brief 解决多个操纵代码目标相同的冲突（第三阶段）
    void resolveTargetConflicts(std::vector<MoveIntent>& intents);

    /// @brief 执行所有合法的移动意向（第四阶段）
    void executeIntents(const std::vector<MoveIntent>& intents);

    static bool canMerge(const Grid& a, const Grid& b);

    /// @brief 执行合并操作
    /// @details 规则：
    ///   - 字母相同：数字相加保留个位，字母不变
    ///   - 数字相同：字母按五进制(1-5)相加，数字不变。如A3+C3→D3(A=1,C=3,1+3=4→D)
    ///   - 完全相同：保持不变
    static Grid performMerge(const Grid& mover, const Grid& target);
    void updateControlledCodes(const std::vector<MoveIntent>& intents);
};

#endif // GAMEBOARD_H
