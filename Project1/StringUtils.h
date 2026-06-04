/// @file StringUtils.h
/// @brief 字符串工具类声明

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>

class StringUtils {
public:
    /// @brief 计算字符串的实际显示宽度（中文占2，英文占1）
    static int getDisplayWidth(const std::string& str);

    /// @brief 按显示宽度填充空格，超出最大宽度则截断并添加省略号
    /// @param str 原始字符串
    /// @param maxWidth 最大显示宽度（包含省略号）
    /// @return 格式化后的字符串
    static std::string padToWidth(const std::string& str, int maxWidth);
};

#endif // STRINGUTILS_H#pragma once
