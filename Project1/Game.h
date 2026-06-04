/// @file Game.h
/// @brief 游戏主控制类声明

#ifndef GAME_H
#define GAME_H

#include "GameBoard.h"
#include "ScoreManager.h"
#include "QuestionBank.h"
#include "ReplayManager.h"
#include <string>
#include <ctime>

class Game {
private:
    GameBoard board;
    ScoreManager scoreManager;
    QuestionBank questionBank;
    ReplayManager replayManager;
    std::string playerName;
    time_t gameStartTime;
    ReplayRecord currentReplay;
    bool isReplayMode;

public:
    Game();

    void run();

private:
    void showMainMenu();
    void startNewGame();
    void startRandomGame();
    void startQuestionGame();
    void runGameLoop();
    bool processInput();
    void selectInitialCodes();
    void showGameOver();
    void showLeaderboard();
    void customQuestion();
    void replayGame();
    void playReplay(int replayIndex);
    void clearScreen();
    int getIntInput(const std::string& prompt, int min, int max);
    std::string getStringInput(const std::string& prompt);
    void pauseAndWait();
};

#endif // GAME_H
