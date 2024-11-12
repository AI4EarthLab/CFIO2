#pragma once
#include <mpi.h>
#include <vector>
#include <iostream>
#include <pnetcdf_func.h>

#include <MemoryManage.h>
#include <pnetcdf.h>
#include <string>
#include <cstring>
#include <array>
#include <set>
#include <map>
#include <Master_struct.h>
#include <string>
// #include <common_struct.h>

using namespace std;

struct Piece_of_data
{
    int global[3];
    int start[3];
    int count[3];
    int current_location;
    size_t offset;
    char filename[charlength];
    char varname[charlength];
    char dimname0[charlength];
    char dimname1[charlength];
    char dimname2[charlength];
    int append;
    int datatype;
};

extern vector<Piece_of_data> meta;
extern MemoryManager pool_p1;
extern Piece_of_data meta_output;
extern int p_size;
class DataReceiver
{
public:
    // 接收数据
    static void Partner_recv(int source_rank, MPI_Comm all_comm, int datatype);

    // 发送数据
    static void Partner_send(int dest_rank, int global[3], int start[3], int count[3], int data_type, void *buf, MPI_Comm all_comm);

    static void Recv_myself(int global[3], int start[3], int count[3], void *buf, MPI_Comm all_comm, string filename, string varname, int datatype, string dimname0, string dimname1, string dimname2, int append);

    static void exchange_data(MPI_Comm all_comm, MPI_Comm sub_comm);

    static void combine_matrix_and_output(MPI_Comm all_comm, MPI_Comm sub_comm, int color);

    static void print3DArray(int *array, int sizeX, int sizeY, int sizeZ, int minX, int minY, int minZ);

    static void write_out(int global_rank, int color, MPI_Comm all_comm, MPI_Comm sub_comm);
};

// 静态成员变量的定义
// std::vector<ReceivedData> DataReceiver::receivedDataList;!sq
