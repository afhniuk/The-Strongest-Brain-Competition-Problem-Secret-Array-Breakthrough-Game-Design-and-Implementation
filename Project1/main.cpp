/// @file main.cpp
/// @brief 《密阵突围》游戏程序入口
/// @details 基于《最强大脑》第13季经典挑战项目，使用C++17面向对象编程实现。
///          合并规则之数字相同：字母按五进制(1-5)相加，数字不变。
///          映射关系：A=1, B=2, C=3, D=4, E=5。
///          例如：A3+C3→D3（A=1,C=3,1+3=4→D）
/// @author 欧典松
/// @date 2026-05

#if defined(_MSC_VER) && !defined(__clang__)
#pragma execution_character_set("utf-8")
#endif
#include "Game.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <stdexcept>


int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    try {
        Game game;
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << "程序发生异常：" << e.what() << "\n";
        std::cerr << "按回车键退出...";
        std::cin.get();
        return 1;
    }

    return 0;
}
