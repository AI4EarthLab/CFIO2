#pragma once

#include <iostream>
#include <mpi.h>
#include <string>
#include <cstring>
#include <common_struct.h>

// 定义一个复杂的结构体
class RequestMessage
{
public:
    MPI_Datatype mpi_struct_type;

    // 构造函数中初始化MPI派生数据类型
    RequestMessage()
    {
        const int nitems = 13;
        int blocklengths[13] = {1, 1, charlength, charlength, charlength, charlength, charlength, 1, 3, 3, 3, 1, 1};
        MPI_Datatype types[13] = {MPI_INT, MPI_INT, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
        MPI_Aint offsets[13];

        // 计算偏移量
        offsets[0] = offsetof(struct_request, rank);
        offsets[1] = offsetof(struct_request, state);
        offsets[2] = offsetof(struct_request, filename);
        offsets[3] = offsetof(struct_request, varname);
        offsets[4] = offsetof(struct_request, dimname0);
        offsets[5] = offsetof(struct_request, dimname1);
        offsets[6] = offsetof(struct_request, dimname2);
        offsets[7] = offsetof(struct_request, step);
        offsets[8] = offsetof(struct_request, global);
        offsets[9] = offsetof(struct_request, start);
        offsets[10] = offsetof(struct_request, count);
        offsets[11] = offsetof(struct_request, append);
        offsets[12] = offsetof(struct_request, datatype);

        // 创建MPI派生数据类型
        MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_struct_type);
        MPI_Type_commit(&mpi_struct_type);
    }

    void sendStruct(const struct_request &data, int dest_rank, int tag, MPI_Comm all_comm)
    {
        MPI_Send(&data, 1, mpi_struct_type, dest_rank, tag, all_comm);
    }

    void recvStruct(struct_request &data, int source_rank, int tag, MPI_Comm all_comm)
    {
        MPI_Recv(&data, 1, mpi_struct_type, source_rank, tag, all_comm, MPI_STATUS_IGNORE);
    }

    // 设置结构体成员的值
    void setStructValues(struct_request &data, int rank, int state, const char *filename, /*const char *varname, const char *dimname0, const char *dimname1, const char *dimname2,*/ int step, int global[3], int start[3], int count[3] /*, int append*/)
    {
        data.rank = rank;
        data.state = state;
        strncpy(data.filename, filename, sizeof(data.filename));
        // strncpy(data.varname, varname, sizeof(data.varname));
        // strncpy(data.dimname0, dimname0, sizeof(data.dimname0));
        // strncpy(data.dimname1, dimname1, sizeof(data.dimname1));
        // strncpy(data.dimname2, dimname2, sizeof(data.dimname2));

        data.step = step;
        memcpy(data.global, global, sizeof(data.global));
        memcpy(data.start, start, sizeof(data.start));
        memcpy(data.count, count, sizeof(data.count));
        // data.append = append;
    }

    ~RequestMessage()
    {
        MPI_Type_free(&mpi_struct_type);
    }
};

class CommandMessage
{
public:
    MPI_Datatype mpi_struct_type;

    CommandMessage()
    {
        const int nitems = 4;
        int blocklengths[4] = {1, 1, 1, 1};
        MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
        MPI_Aint offsets[4];

        offsets[0] = offsetof(struct_command, rank);
        offsets[1] = offsetof(struct_command, command);
        offsets[2] = offsetof(struct_command, partner_rank);
        offsets[3] = offsetof(struct_command, subcommand);

        MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_struct_type);
        MPI_Type_commit(&mpi_struct_type);
    }

    void sendStruct(const struct_command &data, int dest_rank, int tag, MPI_Comm all_comm)
    {
        MPI_Send(&data, 1, mpi_struct_type, dest_rank, tag, all_comm);
    }

    void recvStruct(struct_command &data, int source_rank, int tag, MPI_Comm all_comm)
    {
        MPI_Recv(&data, 1, mpi_struct_type, source_rank, tag, all_comm, MPI_STATUS_IGNORE);
    }

    void setStructValues(struct_command &data, int rank, int command, int partner_rank, int subcommand)
    {
        data.rank = rank;
        data.command = command;
        data.partner_rank = partner_rank;
        data.subcommand = subcommand;
    }

    void getStructValues(const struct_command &data)
    {
        std::cout << "rank = " << data.rank << ", command = " << data.command
                  << ", partner_rank = " << data.partner_rank << "\n";
    }

    ~CommandMessage()
    {
        MPI_Type_free(&mpi_struct_type);
    }
};
