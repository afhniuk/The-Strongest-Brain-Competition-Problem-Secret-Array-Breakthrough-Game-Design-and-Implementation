#include "StringUtils.h"
#include <cstring>

int StringUtils::getDisplayWidth(const std::string& str) {
    int width = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c < 0x80) {
            // ASCII 字符
            width += 1;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            // 2字节 UTF-8
            width += 1;  // 大多数2字节UTF-8字符（如拉丁扩展）显示宽度为1
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            // 3字节 UTF-8（中文）
            width += 2;  // 中文字符显示宽度为2
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            // 4字节 UTF-8（表情符号等）
            width += 2;
            i += 4;
        }
        else {
            // 无效字符
            i += 1;
        }
    }
    return width;
}

std::string StringUtils::padToWidth(const std::string& str, int maxWidth) {
    if (maxWidth <= 0) return "";

    int currentWidth = 0;
    std::string result;

    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        int charWidth;
        int bytesToCopy;

        if (c < 0x80) {
            charWidth = 1;
            bytesToCopy = 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            charWidth = 1;
            bytesToCopy = 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            charWidth = 2;
            bytesToCopy = 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            charWidth = 2;
            bytesToCopy = 4;
        }
        else {
            charWidth = 1;
            bytesToCopy = 1;
        }

        if (currentWidth + charWidth <= maxWidth) {
            result.append(str, i, bytesToCopy);
            currentWidth += charWidth;
            i += bytesToCopy;
        }
        else {
            // 需要截断，但只在实际需要时才添加省略号
            if (currentWidth + 3 <= maxWidth && i < str.length()) {
                result += "...";
            }
            break;
        }
    }

    // 填充空格
    if (currentWidth < maxWidth) {
        result += std::string(maxWidth - currentWidth, ' ');
    }

    return result;
}