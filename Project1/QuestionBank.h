/// @file QuestionBank.h
/// @brief 题库管理类声明

#ifndef QUESTIONBANK_H
#define QUESTIONBANK_H

#include <string>
#include <vector>

struct Question {
    int id;
    int aValue;
    std::vector<std::string> gridData;
    std::string description;

    std::string toString() const;
};

class QuestionBank {
public:
    explicit QuestionBank();

    // 不再使用成员变量存储路径，每次动态构建
    void addQuestion(const Question& q);
    void showQuestionList() const;
    const Question* getQuestionById(int id) const;
    bool isEmpty() const;

    // 获取所有题目（用于遍历）
    const std::vector<Question>& getQuestions() const { return questions; }

    // 公开的文件操作方法（参考 ScoreManager）
    void loadFromFile();
    void saveToFile() const;

private:
    std::string getQuestionsFilePath() const;
    std::vector<Question> questions;
};

#endif // QUESTIONBANK_H