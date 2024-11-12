#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <set>
#include <queue>
#include <common_struct.h>
#include <Master_struct.h>
using namespace std;

// 通信域内进程的个数，即一个变量由几个进程来接收和写文件

/*

几个class：
//ProcessNameCollector 进程名
// NodeChecker    用来记录IO进程的负载程度
// LoaderChecker    用来记录IO进程的负载程度
// IOMission        IO进程待完成的任务列表
// IOstate          主要是记录了哪些IO进程在等待任务分配
// ScheduleHolder   记录某个变量由哪些进程在负责

*/

std::vector<std::string> ProcessNameCollector::processNames;

std::unordered_map<string, int> NodeChecker::data;

std::unordered_map<int, int> LoaderChecker::data;
std::unordered_map<std::string, std::vector<int>> ScheduleHolder::data;
std::unordered_map<std::string, int> ScheduleHolder::currentIndex;
std::unordered_map<int, std::queue<int>> IOMission::data;
std::unordered_map<int, int> IOstate::data;
