#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <set>
#include <queue>
#include <common_struct.h>
#include <mpi.h>
using namespace std;

// 通信域内进程的个数，即一个变量由几个进程来接收和写文件

class ProcessNameCollector
{
private:
    static std::vector<std::string> processNames;

public:
    static void gatherNames(int slave_num, MPI_Comm all_comm)
    {
        char processorName[MPI_MAX_PROCESSOR_NAME];
        int nameLen;
        MPI_Get_processor_name(processorName, &nameLen);

        std::string machineName(processorName, nameLen);

        processNames.push_back(processorName);

        processNames.resize(slave_num + 1);

        for (auto &name : processNames)
            name.resize(MPI_MAX_PROCESSOR_NAME); // 预先分配足够的空间

        for (int i = 1; i < slave_num + 1; ++i)
        {
            MPI_Recv(&processNames[i][0], MPI_MAX_PROCESSOR_NAME, MPI_CHAR, i, 0, all_comm, MPI_STATUS_IGNORE);
        }
        // 广播每个节点名称
        for (int i = 1; i < slave_num + 1; ++i)
        {
            MPI_Bcast(&processNames[i][0], MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, all_comm);
        }
    }
    static string get_rank_name(int rank)
    {
        if (rank > processNames.size() - 1)
            cout << "get_rank_name出错" << endl;
        return processNames[rank];
    }
};

// 用来记录IO进程的负载程度
class NodeChecker
{
private:
    static std::unordered_map<string, int> data;

public:
    // 某个进程处理的IO个数+1
    static void AddNode(string nodename)
    {
        auto it = data.find(nodename);
        if (it != data.end())
        {
            // 如果键已存在，则增加其值
            it->second++;
        }
        else
        {
            // 如果键不存在，则插入新键值对
            data[nodename] = 1;
        }
    }
    // 查询某个进程处理的IO个数目前是多少
    static int LoadNode(string nodename)
    {
        auto it = data.find(nodename);
        if (it != data.end())
        {
            return it->second;
        }
        else
        {
            return 0; // 如果键不存在，则返回默认值
        }
    }
    // 某进程处理个数清0
    static void RemoveNode(string nodename)
    {
        data.erase(nodename); // 从 map 中完全删除键值对
    }
    static void clear()
    {
        data.clear(); // 清空哈希表
    }
};

// 用来记录IO进程的负载程度
class LoaderChecker
{
private:
    static std::unordered_map<int, int> data;

public:
    // 某个进程处理的IO个数+1
    static void AddRank(int num)
    {
        auto it = data.find(num);
        if (it != data.end())
        {
            // 如果键已存在，则增加其值
            it->second++;
        }
        else
        {
            // 如果键不存在，则插入新键值对
            data[num] = 1;
        }
    }
    // 查询某个进程处理的IO个数目前是多少
    static int LoadRank(int num)
    {
        auto it = data.find(num);
        if (it != data.end())
        {
            return it->second;
        }
        else
        {
            return 0; // 如果键不存在，则返回默认值
        }
    }
    // 某进程处理个数清0
    static void RemoveRank(int num)
    {
        data.erase(num); // 从 map 中完全删除键值对
    }
    static void clear()
    {
        data.clear(); // 清空哈希表
    }
};

// 用来记录IO进程的任务是什么，需要接收什么数据，相当于IO进程待完成的任务是什么
class IOMission
{
private:
    static std::unordered_map<int, std::queue<int>> data;

public:
    // 记录一下IO 进程对应应该接受几号进程的数据
    static void addmission(int io_rank, int request_rank)
    {
        data[io_rank].push(request_rank);
    }
    // 从IO进程待完成的任务中选取一个
    static int getmission(int io_rank)
    {
        if (!data[io_rank].empty())
        {
            int value = data[io_rank].front(); // 获取队首元素
            data[io_rank].pop();               // 移除队首元素
            return value;
        }
        else
            return -1;
    }
    // 清空所有IO进程的任务
    static void clear()
    {
        data.clear(); // 清空哈希表及其所有队列
    }
};

// 主要是记录了哪些IO进程在等待任务分配
class IOstate
{
private:
    static std::unordered_map<int, int> data;

public:
    // 判断该IO进程是否正在等待任务
    static int get_state(int io_rank)
    {
        auto it = data.find(io_rank);
        if (it != data.end())
        {
            // 键存在
            return it->second;
        }
        else
        {
            // 键不存在，返回默认值或者抛出异常等
            return -1; // 或者抛出一个异常，表示键不存在
        }
    }
    // 设置IO进程的状态
    static void set_state(int io_rank, int state)
    {
        data[io_rank] = state;
    }
    // 清空所有IO进程的状态
    static void clear()
    {
        data.clear(); // 清空哈希表
    }
};

// 记录某个变量由哪些进程在负责
class ScheduleHolder
{
private:
    static std::unordered_map<std::string, std::vector<int>> data;
    static std::unordered_map<std::string, int> currentIndex;

public:
    // key这个变量增加一个进程来IO
    static void addValue(const std::string &key, int value)
    {
        data[key].push_back(value);
    }

    // 从key对应的IO进程中随机选一个返回（似乎是循环的，不是随机的）
    static int getRandomValue(const std::string &key)
    {
        if (data.find(key) != data.end() && !data[key].empty())
        {
            int index = currentIndex[key] % data[key].size();
            int value = data[key][index];
            currentIndex[key]++;
            return value;
        }
        else
        {
            std::cout << "获取I/O进程出错" << std::endl;
            return -1; // 如果不存在对应的vector或者vector为空，返回-1表示未找到
        }
    }

    // 删除key这个变量
    static void deleteKey(const std::string &key)
    {

        data.erase(key);
    }

    // 查询key这个变量现在由几个进程负责IO
    static int getVectorSize(const std::string &key)
    {
        if (data.find(key) != data.end())
        {
            return data[key].size();
        }
        else
            return 0; // 如果键不存在，则返回0表示vector为空
    }

    // 查询现在记录了多少个key
    static int getKeyCount()
    {
        return data.size(); // 返回map中键值对的数量
    }

    // 查询某个变量已经完成了几次数据收集了
    static int get_IO_done_num(const std::string &key)
    {
        if (data.find(key) != data.end() && !data[key].empty())
        {
            int sum = 0;
            for (int i = 0; i < data[key].size(); i++)
            {
                // LoaderChecker::LoadRank(data[key][i]);
                sum += LoaderChecker::LoadRank(data[key][i]);
            }
            return sum;
        }
        else
        {
            std::cout << "获取变量已IO数量出错" << std::endl;
            return -1; // 如果不存在对应的vector或者vector为空，返回-1表示未找到
        }
    }
    // 获取一个key对应的IO process ，哪些还在wait
    static vector<int> get_wait_IO_processes(const std::string &key)
    {
        vector<int> result;
        if (data.find(key) != data.end() && !data[key].empty())
        {

            int sum = 0;
            for (int i = 0; i < data[key].size(); i++)
            {
                LoaderChecker::LoadRank(data[key][i]);
                int temprank = data[key][i];
                if (IOstate::get_state(temprank) == state_iofinish)
                    result.push_back(temprank);
            }
        }
        else
        {
            std::cout << "获取变量已IO数量出错" << std::endl;
        }
        return result;
    }

    // 获取指定进程负责的所有key
    static int getKeysForProcess(int processId)
    {
        int c = 0;
        for (const auto &pair : data)
        {
            const std::string &key = pair.first;
            const std::vector<int> &processes = pair.second;
            if (std::find(processes.begin(), processes.end(), processId) != processes.end())
            {
                return c;
            }
            c++;
        }
        return -1;
    }
    static void clear()
    {
        data.clear();         // 清空数据哈希表
        currentIndex.clear(); // 清空当前索引哈希表
    }
};
/*

几个class：
//ProcessNameCollector 进程名
// NodeChecker    用来记录IO进程的负载程度
// LoaderChecker    用来记录IO进程的负载程度
// IOMission        IO进程待完成的任务列表
// IOstate          主要是记录了哪些IO进程在等待任务分配
// ScheduleHolder   记录某个变量由哪些进程在负责

*/

/*
#define state_other -1
#define state_ioing 0
#define state_needio  1
#define state_iofinish 2
#define state_finalize 3

#define command_directwrite 0
#define command_gooncompute 1
#define command_sendandcompute 2
#define command_helppartnerio 3
*/