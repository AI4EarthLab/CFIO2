#include <common_struct.h>

// 将 std::string 转换为 char 数组
void stringToCharArray(const std::string &str, char arr[charlength])
{
    // 确保不会超出数组边界
    size_t length = str.copy(arr, charlength - 1); // 复制字符串到字符数组
    arr[length] = '\0';                            // 添加字符串结束符
}

// 将 char 数组转换为 std::string
std::string charArrayToString(const char arr[charlength])
{
    return std::string(arr); // 使用 std::string 构造函数直接转换
}
