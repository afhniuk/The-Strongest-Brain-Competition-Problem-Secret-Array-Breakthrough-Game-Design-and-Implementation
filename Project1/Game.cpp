/// @file Game.cpp
/// @brief 游戏主控制类实现

#include "Game.h"
#include "LetterUtils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>
#include <limits>
#include <locale>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#undef max
#endif

Game::Game()
    : scoreManager()
    , questionBank()
    , replayManager()
    , isReplayMode(false) {
    gameStartTime = 0;
}

void Game::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void Game::pauseAndWait() {
    std::cout << "\n按回车键继续...";
    std::cin.get();
}

int Game::getIntInput(const std::string& prompt, int min, int max) {
    int value;
    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);
        if (iss >> value && value >= min && value <= max) {
            return value;
        }
        std::cout << "输入无效，请输入 " << min << " 到 " << max << " 之间的整数。\n";
    }
}

std::string Game::getStringInput(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

void Game::run() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    std::ios::sync_with_stdio(false);
    std::setlocale(LC_ALL, ".65001");
    std::locale::global(std::locale(".65001"));
    std::cout.imbue(std::locale(".65001"));
    std::cin.imbue(std::locale(".65001"));
#endif

    clearScreen();
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║           《密阵突围》 - Matrix Breakout                     ║\n";
    std::cout << "║           基于《最强大脑》第13季经典挑战项目                 ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║           作者：欧典松  学号：202530902356                   ║\n";
    std::cout << "║           软件工程（卓越班）                                 ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    pauseAndWait();

    while (true) {
        clearScreen();
        showMainMenu();

        int choice = getIntInput("请选择操作 [1-6]：", 1, 6);

        switch (choice) {
            case 1: startRandomGame(); break;
            case 2: startQuestionGame(); break;
            case 3: showLeaderboard(); break;
            case 4: customQuestion(); break;
            case 5: replayGame(); break;
            case 6:
                std::cout << "\n感谢游玩《密阵突围》！再见！\n";
                return;
        }

        if (choice >= 1 && choice <= 3) {
            pauseAndWait();
        }
    }
}

void Game::showMainMenu() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      主  菜  单                              ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║  1. 开始新游戏（随机盘面）                                   ║\n";
    std::cout << "║  2. 选择题库题目                                             ║\n";
    std::cout << "║  3. 查看排行榜                                               ║\n";
    std::cout << "║  4. 自定义题目                                               ║\n";
    std::cout << "║  5. 对局回放                                                 ║\n";
    std::cout << "║  6. 退出游戏                                                 ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void Game::startRandomGame() {
    clearScreen();
    std::cout << "\n=== 开始新游戏（随机盘面） ===\n\n";

    playerName = getStringInput("请输入您的名称：");
    int aValue = getIntInput("请输入难度系数 a (1-10)：", 1, 10);

    std::cout << "\n正在生成 " << (5 * aValue) << "x" << (5 * aValue) << " 的盘面...\n";

    try {
        board.initBoard(aValue);
    } catch (const std::exception& e) {
        std::cout << "初始化失败：" << e.what() << "\n";
        return;
    }

    currentReplay = ReplayRecord();
    currentReplay.playerName = playerName;
    currentReplay.aValue = aValue;
    currentReplay.initialBoard = board.exportBoardData();

    selectInitialCodes();

    currentReplay.initialCodes = board.getControlledCodes();
    gameStartTime = std::time(nullptr);

    runGameLoop();
}

void Game::startQuestionGame() {
    clearScreen();
    std::cout << "\n=== 选择题库题目 ===\n\n";
    questionBank.loadFromFile();
    questionBank.showQuestionList();

    if (questionBank.isEmpty()) {
        std::cout << "题库中没有题目，请先创建或使用随机盘面。\n";
        return;
    }

    int qid = getIntInput("请输入题号：", 1, 999);
    const Question* q = questionBank.getQuestionById(qid);

    if (q == nullptr) {
        std::cout << "未找到该题号！\n";
        return;
    }

    std::cout << "已选择：" << q->toString() << "\n";
    playerName = getStringInput("请输入您的名称：");

    try {
        board.initBoardFromData(q->aValue, q->gridData);
    } catch (const std::exception& e) {
        std::cout << "加载题目失败：" << e.what() << "\n";
        return;
    }

    currentReplay = ReplayRecord();
    currentReplay.playerName = playerName;
    currentReplay.aValue = q->aValue;
    currentReplay.initialBoard = board.exportBoardData();

    selectInitialCodes();

    currentReplay.initialCodes = board.getControlledCodes();
    gameStartTime = std::time(nullptr);

    runGameLoop();
}

void Game::selectInitialCodes() {
    int aValue = board.getA();
    std::vector<Position> selectedPositions;

    std::cout << "\n请选择 " << aValue << " 个初始操纵代码。\n";
    std::cout << "当前盘面：\n" << board.toString(false) << "\n";
    std::cout << "输入格式：行号 列号（用空格分隔），例如：0 1\n";
    std::cout << "已选数量：" << 0 << "/" << aValue << "\n\n";

    while (static_cast<int>(selectedPositions.size()) < aValue) {
        std::cout << "选择第 " << (selectedPositions.size() + 1) << " 个：";
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);

        int row, col;
        if (!(iss >> row >> col)) {
            std::cout << "格式错误！请输入两个整数（行 列）。\n";
            continue;
        }

        if (row < 0 || row >= board.getSize() || col < 0 || col >= board.getSize()) {
            std::cout << "坐标超出范围！盘面大小为 " << board.getSize() << "x" << board.getSize()
                      << "，有效行列范围：0-" << (board.getSize() - 1) << "\n";
            continue;
        }

        Position pos(row, col);

        bool duplicate = false;
        for (const auto& existing : selectedPositions) {
            if (existing == pos) { duplicate = true; break; }
        }
        if (duplicate) {
            std::cout << "该位置已选择，请选择其他位置。\n";
            continue;
        }

        if (board.getBoard()[row][col].isEmpty) {
            std::cout << "该位置为空，请选择有代码的位置。\n";
            continue;
        }

        selectedPositions.push_back(pos);
        std::cout << "已选择位置 (" << row << ", " << col << ") - "
                  << board.getBoard()[row][col].toString() << "\n";
        std::cout << "进度：" << selectedPositions.size() << "/" << aValue << "\n";
    }

    board.selectInitialCodes(selectedPositions);
    std::cout << "\n初始操纵代码选择完成！游戏开始！\n";
    std::cout << board.toString(true) << "\n";
    pauseAndWait();
}

void Game::runGameLoop() {
    isReplayMode = false;

    while (!board.isGameOver()) {
        clearScreen();

        // 使用 stringstream 收集所有输出，一次性输出
        std::ostringstream oss;

        oss << "\n=== 密阵突围 ===\n";
        oss << "玩家：" << playerName << " | a=" << board.getA()
            << " | 盘面：" << board.getSize() << "x" << board.getSize()
            << " | 得分：" << board.getMergeCount()
            << " | 操作次数：" << board.getTotalMoves() << "\n\n";
        oss << board.toString(true);
        oss << "\n操作：W=上  S=下  A=左  D=右  Q=退出游戏\n";

        std::cout << oss.str();  // 一次性输出

        if (!processInput()) break;
    }

    if (board.isGameOver()) {
        clearScreen();
        std::cout << "\n=== 游戏结束 ===\n";
        std::cout << board.toString(false);
        showGameOver();
    }
}

bool Game::processInput() {
    std::cout << "\n请输入移动方向：";
    std::string input;
    std::getline(std::cin, input);

    if (input.empty()) return true;

    char ch = input[0];
    Direction dir;

    switch (ch) {
        case 'W': case 'w': dir = Direction::UP;    break;
        case 'S': case 's': dir = Direction::DOWN;  break;
        case 'A': case 'a': dir = Direction::LEFT;  break;
        case 'D': case 'd': dir = Direction::RIGHT; break;
        case 'Q': case 'q':
            std::cout << "确定要退出吗？当前进度将丢失。(y/n)：";
            std::getline(std::cin, input);
            if (input == "y" || input == "Y") return false;
            return true;
        default:
            std::cout << "无效输入！请使用 W/A/S/D 移动，Q 退出。\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            return true;
    }

    MoveResult result = board.move(dir);
    replayManager.recordStep(currentReplay, dir, result, board.getMergeCount());

    switch (result) {
        case MoveResult::SUCCESS_MERGE:
            std::cout << ">>> 合并成功！得分 +1\n";
            break;
        case MoveResult::SUCCESS_MOVE:
            std::cout << ">>> 移动成功（无合并）\n";
            break;
        case MoveResult::FAILED:
            std::cout << ">>> 无法移动！\n";
            break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return true;
}

void Game::showGameOver() {
    time_t endTime = std::time(nullptr);
    int elapsedSeconds = static_cast<int>(endTime - gameStartTime);
    if (elapsedSeconds < 0) elapsedSeconds = 0;

    ScoreRecord record;
    record.playerName = playerName;
    record.aValue = board.getA();
    record.totalTimeSeconds = elapsedSeconds;
    record.mergeCount = board.getMergeCount();
    record.totalMoves = board.getTotalMoves();
    record.calculateScoreRate();

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    游 戏 结 束                               ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  玩家名称：" << std::setw(50) << std::left << playerName << "║\n";
    std::cout << "║  难度系数：a = " << std::setw(46) << std::left << board.getA() << "║\n";
    std::cout << "║  盘面大小：" << std::setw(50) << std::left
              << (std::to_string(board.getSize()) + "x" + std::to_string(board.getSize())) << "║\n";
    std::cout << "║  用时    ：" << std::setw(51) << std::left
              << (std::to_string(elapsedSeconds) + " 秒") << "║\n";
    std::cout << "║  合并次数：" << std::setw(50) << std::left << board.getMergeCount() << "║\n";
    std::cout << "║  总操作数：" << std::setw(50) << std::left << board.getTotalMoves() << "║\n";
    std::cout << "║  得分率  ：" << std::setw(50) << std::left
              << std::fixed << std::setprecision(3) << record.scoreRate << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    scoreManager.addRecord(record);
    
    if (currentReplay.initialCodes.empty()) {
        std::cout << "[错误] initialCodes 为空，无法保存回放！\n";
    }

    replayManager.saveReplay(currentReplay);
}

void Game::showLeaderboard() {
    clearScreen();

    std::cout << "\n=== 排行榜 ===\n";
    std::cout << "1. 查看全部排行榜\n";
    std::cout << "2. 按 a 值筛选\n";

    int choice = getIntInput("请选择：", 1, 2);

    if (choice == 1) {
        scoreManager.showLeaderboard();
    } else {
        int aFilter = getIntInput("请输入 a 值：", 1, 10);
        scoreManager.showLeaderboard(aFilter);
    }
}

void Game::customQuestion() {
    clearScreen();
    std::cout << "\n=== 自定义题目 ===\n\n";

    int aValue = getIntInput("请输入难度系数 a (1-10)：", 1, 10);
    int size = 5 * aValue;

    std::cout << "\n将创建一个 " << size << "x" << size << " 的盘面。\n";
    std::cout << "您可以手动输入每个格子的代码。\n";
    std::cout << "格式：字母+数字（如 A3），输入 .. 表示空格子。\n";
    std::cout << "输入 'random' 可随机填充该位置。\n\n";

    Question q;
    q.aValue = aValue;
    q.description = getStringInput("请输入题目描述：");

    // 确保清空之前的网格数据
    q.gridData.clear();
    q.gridData.reserve(size * size);

    for (int r = 0; r < size; ++r) {
        std::cout << "\n--- 第 " << r << " 行 (" << size << "列) ---\n";
        for (int c = 0; c < size; ++c) {
            std::string prompt = "  (" + std::to_string(r) + "," + std::to_string(c) + ")：";
            std::string val = getStringInput(prompt);

            if (val == "random" || val == "r") {
                char letter = LetterUtils::randomLetter();
                int number = LetterUtils::randomNumber();
                q.gridData.push_back(std::string(1, letter) + std::to_string(number));
                std::cout << "    -> 随机生成：" << letter << number << "\n";
            }
            else if (val == ".." || val == "." || val.empty()) {
                q.gridData.push_back("..");
            }
            else if (val.length() >= 2 && LetterUtils::isValidLetter(val[0])
                && val[1] >= '0' && val[1] <= '9') {
                q.gridData.push_back(val.substr(0, 2));
            }
            else {
                std::cout << "格式错误，将随机填充。\n";
                char letter = LetterUtils::randomLetter();
                int number = LetterUtils::randomNumber();
                q.gridData.push_back(std::string(1, letter) + std::to_string(number));
            }
        }
    }

    questionBank.addQuestion(q);
    std::cout << "\n题目已保存！题号：" << q.id << "\n";
    pauseAndWait();
}

void Game::replayGame() {
    clearScreen();
    std::cout << "\n=== 对局回放 ===\n\n";

    
    replayManager.showReplayList();

    if (replayManager.getReplayCount() == 0) {
        std::cout << "暂无回放记录。\n";
        return;
    }

    int index = getIntInput(
        "请输入要回放的对局序号 (1-" + std::to_string(replayManager.getReplayCount()) + ")：",
        1, replayManager.getReplayCount());

    playReplay(index - 1);
}

void Game::playReplay(int replayIndex) {
    const ReplayRecord* rec = replayManager.getReplayByIndex(replayIndex);
    if (rec == nullptr) {
        std::cout << "回放记录不存在！\n";
        std::cin.get();
        return;
    }

    clearScreen();
    std::cout << "\n=== 正在回放：" << rec->playerName
              << " (a=" << rec->aValue << ") ===\n\n";

    try {
        board.initBoardFromData(rec->aValue, rec->initialBoard);
    } catch (const std::exception& e) {
        std::cout << "回放初始化失败：" << e.what() << "\n";
        std::cin.get();
        return;
    }

    board.selectInitialCodes(rec->initialCodes);

    std::cout << "初始盘面：\n";
    std::cout << board.toString(true);
    std::cout << "\n按回车键开始逐步回放...\n";
    pauseAndWait();

    for (const auto& step : rec->steps) {
        clearScreen();
        std::cout << "\n=== 回放中 ===\n";
        std::cout << step.toString() << "\n\n";

        board.move(step.direction);
        std::cout << board.toString(true);

        std::cout << "\n步骤 " << step.stepNumber << "/" << rec->steps.size()
                  << " | 得分：" << board.getMergeCount() << "\n";
        std::cout << "按回车键继续下一步...";
        std::cin.get();
    }

    clearScreen();
    std::cout << "\n=== 回放结束 ===\n";
    std::cout << "最终盘面：\n";
    std::cout << board.toString(false);
    std::cout << "最终得分：" << board.getMergeCount() << "\n";
    std::cout << "总步数：" << rec->steps.size() << "\n";
    pauseAndWait();
    
}