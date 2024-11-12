#pragma once
#include <string>

#define charlength 128
// 向master发送

#define state_other -1
#define state_ioing 0
#define state_needio 1
#define state_iofinish 2
#define state_finalize 3
#define state_write_out 4

struct struct_request
{
    int rank;                  // 本进程rank号
    int state;                 // 0:计算  1:计算完成，请求IO；  2：IO完成 3:结束 finalize
    char filename[charlength]; // 输出文件及变量名称
    char varname[charlength];
    char dimname0[charlength];
    char dimname1[charlength];
    char dimname2[charlength];
    int step; // 当前是第几次输出，不一定从0或1开始
    int global[3];
    int start[3];
    int count[3];
    int append;
    int datatype;
};

// 将 std::string 转换为 char 数组
void stringToCharArray(const std::string &str, char arr[charlength]);

// 将 char 数组转换为 std::string
std::string charArrayToString(const char arr[charlength]);

#define command_directwrite 0
#define command_gooncompute 1
#define command_sendandcompute 2
#define command_helppartnerio 3
#define command_write_out 4

// master向进程发送
struct struct_command
{
    int rank;         // 向谁发送
    int command;      // 命令 0：直接写文件 1：继续下一轮计算 2：将数据交给x号进程,然后继续计算 3：帮助x号进程写数据
    int partner_rank; // 交给谁 或 替谁
    int subcommand;
};
