/// @file QuestionBank.cpp
/// @brief 题库管理类实现

#include "QuestionBank.h"
#include "LetterUtils.h"
#include "StringUtils.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

QuestionBank::QuestionBank() {
    // 确保 data 目录存在
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path dataPath = currentPath / "data";
    std::filesystem::create_directories(dataPath);

    loadFromFile();

    // 如果题库为空，生成内置题目
    if (questions.empty()) {
        std::cout << "[QuestionBank] 题库为空，生成内置题目...\n";

        // 题目1：a=2（10x10盘面）
        {
            Question q;
            q.id = 1;
            q.aValue = 2;
            q.description = "入门关卡 - 10x10盘面";
            int size = 10;
            for (int r = 0; r < size; ++r) {
                for (int c = 0; c < size; ++c) {
                    if ((r + c) % 3 == 0) {
                        q.gridData.push_back("..");
                    }
                    else {
                        char letter = LetterUtils::valueToLetter((r + c) % 5 + 1);
                        int number = (r * c) % 10;
                        q.gridData.push_back(std::string(1, letter) + std::to_string(number));
                    }
                }
            }
            questions.push_back(q);
        }

        // 题目2：a=4（20x20盘面）
        {
            Question q;
            q.id = 2;
            q.aValue = 4;
            q.description = "进阶关卡 - 20x20盘面";
            int size = 20;
            for (int r = 0; r < size; ++r) {
                for (int c = 0; c < size; ++c) {
                    char letter = LetterUtils::valueToLetter((r * 3 + c * 2) % 5 + 1);
                    int number = (r * 7 + c * 3) % 10;
                    q.gridData.push_back(std::string(1, letter) + std::to_string(number));
                }
            }
            questions.push_back(q);
        }

        // 题目3：a=6（30x30盘面）
        {
            Question q;
            q.id = 3;
            q.aValue = 6;
            q.description = "困难关卡 - 30x30盘面";
            int size = 30;
            for (int r = 0; r < size; ++r) {
                for (int c = 0; c < size; ++c) {
                    char letter = LetterUtils::valueToLetter((r * c) % 5 + 1);
                    int number = (r + c * 2) % 10;
                    q.gridData.push_back(std::string(1, letter) + std::to_string(number));
                }
            }
            questions.push_back(q);
        }

        saveToFile();
        loadFromFile();
        std::cout << "[QuestionBank] 已生成 " << questions.size() << " 道内置题目\n";
    }
}

std::string QuestionBank::getQuestionsFilePath() const {
    std::filesystem::path currentPath = std::filesystem::current_path();
    return (currentPath / "data" / "questions.txt").string();
}

void QuestionBank::loadFromFile() {
    questions.clear();

    std::string filePath = getQuestionsFilePath();
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string line;
    Question currentQ;
    bool inQuestion = false;
    int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;

        // 去除行尾回车
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }

        if (line.empty()) continue;
        if (line[0] == '#') continue;

        if (line[0] == '[') {
            // 保存上一题
            if (inQuestion && currentQ.id > 0 && !currentQ.gridData.empty()) {
                questions.push_back(currentQ);
            }

            // 开始新题目
            inQuestion = true;
            currentQ = Question();
            currentQ.gridData.clear();

            // 解析: [id:a值:描述]
            std::string content = line.substr(1, line.length() - 2);

            size_t firstColon = content.find(':');
            if (firstColon != std::string::npos) {
                size_t secondColon = content.find(':', firstColon + 1);
                if (secondColon != std::string::npos) {
                    currentQ.id = std::stoi(content.substr(0, firstColon));
                    currentQ.aValue = std::stoi(content.substr(firstColon + 1, secondColon - firstColon - 1));
                    currentQ.description = content.substr(secondColon + 1);
                }
            }
        }
        else if (inQuestion) {
            // 读取盘面数据
            std::istringstream iss(line);
            std::string cell;
            while (iss >> cell) {
                currentQ.gridData.push_back(cell);
            }
        }
    }

    // 保存最后一题
    if (inQuestion && currentQ.id > 0 && !currentQ.gridData.empty()) {
        questions.push_back(currentQ);
    }

    file.close();
}

void QuestionBank::saveToFile() const {
    std::string filePath = getQuestionsFilePath();

    // 确保目录存在
    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cout << "[saveToFile] 错误：无法打开文件 " << filePath << "\n";
        return;
    }

    file << "# 密阵突围 题库文件\n";
    file << "# 格式：[题号:a值:描述]\n";
    file << "# 然后每行列出该题的盘面数据（..=空格，字母数字=代码）\n\n";

    for (const auto& q : questions) {
        file << "[" << q.id << ":" << q.aValue << ":" << q.description << "]\n";

        int size = 5 * q.aValue;
        for (int r = 0; r < size; ++r) {
            for (int c = 0; c < size; ++c) {
                int idx = r * size + c;
                if (idx < static_cast<int>(q.gridData.size())) {
                    file << q.gridData[idx];
                }
                else {
                    file << "..";
                }
                if (c < size - 1) file << " ";
            }
            file << "\n";
        }
        file << "\n";
    }

    file.close();
}

void QuestionBank::addQuestion(const Question& q) {
    // 计算最大ID
    int maxId = 0;
    for (const auto& existing : questions) {
        maxId = std::max(maxId, existing.id);
    }

    Question newQ = q;
    newQ.id = maxId + 1;

    // 验证盘面数据完整性
    int expectedSize = 5 * newQ.aValue;
    int expectedCells = expectedSize * expectedSize;
    if (static_cast<int>(newQ.gridData.size()) != expectedCells) {
        std::cout << "[addQuestion] 警告：盘面数据大小不匹配！"
            << " 实际=" << newQ.gridData.size()
            << ", 期望=" << expectedCells << "\n";
        // 补充缺失的数据
        while (static_cast<int>(newQ.gridData.size()) < expectedCells) {
            newQ.gridData.push_back("..");
        }
    }

    questions.push_back(newQ);
    saveToFile();
}

const Question* QuestionBank::getQuestionById(int id) const {
    for (const auto& q : questions) {
        if (q.id == id) return &q;
    }
    return nullptr;
}

void QuestionBank::showQuestionList() const {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                  题 库 列 表                         ║\n";
    std::cout << "╠════╤══════╤══════════════════════════════════════════╣\n";
    std::cout << "║题号│a 值  │描述                                      ║\n";
    std::cout << "╟────┼──────┼──────────────────────────────────────────╢\n";

    if (questions.empty()) {
        std::cout << "║              暂无题目                                ║\n";
    }
    else {
        for (const auto& q : questions) {
            std::string desc = StringUtils::padToWidth(q.description, 41);
            std::cout << "║" << std::left
                << std::setw(3) << q.id << " │"
                << std::setw(5) << q.aValue << " │"
                << desc << " ║\n";
        }
    }
    std::cout << "╚════╧══════╧══════════════════════════════════════════╝\n\n";
}

bool QuestionBank::isEmpty() const {
    return questions.empty();
}

std::string Question::toString() const {
    std::ostringstream oss;
    oss << "题目 #" << id << " (a=" << aValue << ", "
        << (5 * aValue) << "x" << (5 * aValue) << ") - " << description;
    return oss.str();
}