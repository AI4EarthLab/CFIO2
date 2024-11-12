#include <mpi.h>
#include <vector>
#include <iostream>
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
#include <pnetcdf_func.h>
#include <DataReceiver.h>

using namespace std;

void DataReceiver::Partner_recv(int source_rank, MPI_Comm all_comm, int datatype)
{
    // return;
    int slave_num;
    MPI_Comm_size(all_comm, &slave_num);
    slave_num--;
    std::array<int, 10> recv_buffer;
    MPI_Recv(recv_buffer.data(), 10, MPI_INT, source_rank, 3, all_comm, MPI_STATUS_IGNORE);
    meta.emplace_back();
    for (int i = 0; i < 3; ++i)
    {
        meta.back().global[i] = recv_buffer[i];    // First 3 integers for global
        meta.back().start[i] = recv_buffer[i + 3]; // Next 3 integers for start
        meta.back().count[i] = recv_buffer[i + 6]; // Last 3 integers for count
    }
    meta.back().datatype = recv_buffer[9];

    size_t bufNum = static_cast<size_t>(meta.back().count[0] * meta.back().count[1] * meta.back().count[2]);

    if (!pool_p1.isMemoryAllocated())
    {
        cout << "err" << endl;
    }
    void *recvaddress;

    if (datatype == 0 || datatype == 1)
        recvaddress = pool_p1.getMemoryBlock(bufNum * 4);
    if (datatype == 2)
        recvaddress = pool_p1.getMemoryBlock(bufNum * 8);

    if (meta.back().datatype == 0)
        MPI_Recv(recvaddress, static_cast<int>(bufNum), MPI_INT, source_rank, 3, all_comm, MPI_STATUS_IGNORE);
    if (meta.back().datatype == 1)
        MPI_Recv(recvaddress, static_cast<int>(bufNum), MPI_FLOAT, source_rank, 3, all_comm, MPI_STATUS_IGNORE);
    if (meta.back().datatype == 2)
        MPI_Recv(recvaddress, static_cast<int>(bufNum), MPI_DOUBLE, source_rank, 3, all_comm, MPI_STATUS_IGNORE);
}
// 发送数据
void DataReceiver::Partner_send(int dest_rank, int global[3], int start[3], int count[3], int data_type, void *buf, MPI_Comm all_comm)
{
    // return;
    std::array<int, 10> send_buffer; // Buffer to hold the data to be sent

    for (int i = 0; i < 3; ++i)
    {
        send_buffer[i] = global[i];    // First 3 integers for global
        send_buffer[i + 3] = start[i]; // Next 3 integers for start
        send_buffer[i + 6] = count[i]; // Last 3 integers for count
    }
    send_buffer[9] = data_type;
    // Send all 9 integers in one go
    MPI_Send(send_buffer.data(), 10, MPI_INT, dest_rank, 3, all_comm);

    if (data_type == 0)
        MPI_Send(buf, count[0] * count[1] * count[2], MPI_INT, dest_rank, 3, all_comm);
    if (data_type == 1)
        MPI_Send(buf, count[0] * count[1] * count[2], MPI_FLOAT, dest_rank, 3, all_comm);
    if (data_type == 2)
        MPI_Send(buf, count[0] * count[1] * count[2], MPI_DOUBLE, dest_rank, 3, all_comm);
}

void DataReceiver::Recv_myself(int global[3], int start[3], int count[3], void *buf, MPI_Comm all_comm, string filename, string varname, int datatype, string dimname0, string dimname1, string dimname2, int append)
{
    // 计算从者数量
    int slave_num;
    int myrank;
    MPI_Comm_size(all_comm, &slave_num);
    MPI_Comm_rank(all_comm, &myrank);

    slave_num--;

    meta.emplace_back();
    for (int i = 0; i < 3; ++i)
    {
        meta.back().global[i] = global[i];
        meta.back().start[i] = start[i];
        meta.back().count[i] = count[i];
    }
    strcpy(meta.back().filename, filename.c_str());
    strcpy(meta.back().varname, varname.c_str());
    strcpy(meta.back().dimname0, dimname0.c_str());
    strcpy(meta.back().dimname1, dimname1.c_str());
    strcpy(meta.back().dimname2, dimname2.c_str());
    meta.back().append = append;
    meta.back().datatype = datatype;

    size_t bufNum = static_cast<size_t>(count[0] * count[1] * count[2]);

    int bytes;
    if (datatype == 0 || datatype == 1)
        bytes = 4;
    if (datatype == 2)
        bytes = 8;

    if (!pool_p1.isMemoryAllocated())
    {
        // auto start = std::chrono::high_resolution_clock::now(); // Start timing
        pool_p1.allocateMemory((size_t)bytes * bufNum * slave_num / p_size * 2);
        // auto end = std::chrono::high_resolution_clock::now(); // End timing

        // std::chrono::duration<double> duration = end - start; // Calculate duration
        // cout << bytes << " " << " " << bufNum << " " << slave_num << " " << p_size;
        // std::cout << "Resize time: " << duration.count() << " seconds" << (size_t)bytes * bufNum * slave_num / p_size * 2 << std::endl;
    }

    auto recvaddress = pool_p1.getMemoryBlock(bufNum * bytes);
    if (datatype == 0 || datatype == 1)
        std::memcpy(recvaddress, buf, bufNum * 4);
    if (datatype == 2)
        std::memcpy(recvaddress, buf, bufNum * 8);
}

void DataReceiver::exchange_data(MPI_Comm all_comm, MPI_Comm sub_comm)
{
    int local_size;
    MPI_Comm_size(sub_comm, &local_size);

    int all_rank;

    MPI_Comm_rank(all_comm, &all_rank);
    int gathered_numbers[1000];
    int rank_correspondence[local_size];
    for (int i = 0; i < local_size; i++)
    {
        rank_correspondence[i] = all_rank;
    }
    MPI_Allgather(&all_rank, 1, MPI_INT, gathered_numbers, 1, MPI_INT, sub_comm);

    // for pnetcdf filename varname ...
    meta_output = meta[0];
    int datatype = meta_output.datatype;

    for (int i = 0; i < meta.size(); i++)
    {
        meta[i].current_location = all_rank;
    }

    // Step 1: Create MPI datatype for Piece_of_data
    MPI_Datatype mpi_piece_of_data;
    int blocklengths[] = {3, 3, 3, 1, 1, charlength, charlength, charlength, charlength, charlength, 1, 1};
    MPI_Aint offsets[12];

    MPI_Datatype types[] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_UNSIGNED_LONG_LONG, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT};

    offsets[0] = offsetof(Piece_of_data, global);
    offsets[1] = offsetof(Piece_of_data, start);
    offsets[2] = offsetof(Piece_of_data, count);
    offsets[3] = offsetof(Piece_of_data, current_location);
    offsets[4] = offsetof(Piece_of_data, offset);
    offsets[5] = offsetof(Piece_of_data, filename);
    offsets[6] = offsetof(Piece_of_data, varname);
    offsets[7] = offsetof(Piece_of_data, dimname0);
    offsets[8] = offsetof(Piece_of_data, dimname1);
    offsets[9] = offsetof(Piece_of_data, dimname2);
    offsets[10] = offsetof(Piece_of_data, append);
    offsets[11] = offsetof(Piece_of_data, datatype);
    MPI_Type_create_struct(12, blocklengths, offsets, types, &mpi_piece_of_data);
    MPI_Type_commit(&mpi_piece_of_data);

    // Step 2: Gather all metadata to all processes
    int local_meta_size = meta.size(); // 注意这里包含自己本身的数据

    std::vector<int> recv_counts(local_size);
    MPI_Allgather(&local_meta_size, 1, MPI_INT, recv_counts.data(), 1, MPI_INT, sub_comm);

    // 总共多少个小数据块
    int total_pieces = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);

    auto address = pool_p1.getAddresses();
    if (address.size() != meta.size())
        cout << "err~~~~~~~~~~~~~~~~~" << endl;
    for (int i = 0; i < meta.size(); i++)
    {
        meta[i].offset = address[i];
    }

    std::vector<Piece_of_data> all_meta(total_pieces);

    std::vector<int> displacements(local_size, 0);
    for (int i = 1; i < local_size; ++i)
    {
        displacements[i] = displacements[i - 1] + recv_counts[i - 1];
    }

    // 所有进程都有了全部的meta信息
    MPI_Allgatherv(meta.data(), local_meta_size, mpi_piece_of_data,
                   all_meta.data(), recv_counts.data(), displacements.data(),
                   mpi_piece_of_data, sub_comm);

    MPI_Type_free(&mpi_piece_of_data);

    // Step 2: Sort all pieces by their y-start position
    std::sort(all_meta.begin(), all_meta.end(),
              [](const Piece_of_data &a, const Piece_of_data &b)
              {
                  return a.start[1] < b.start[1];
              });

    // 下面这一段，根据y的start尽量平均分，
    map<int, int> which_rank;
    std::set<int> orderedSet;
    for (const auto &piece : all_meta)
    {
        orderedSet.insert(piece.start[1]); // y 的 start
    }
    // 每个进程y方向上被分配几个
    int pre_block = (orderedSet.size() - 1) / local_size + 1;
    int cc = 0;
    for (const auto &start : orderedSet)
    {
        which_rank[start] = gathered_numbers[cc / pre_block];
        cc++;
    }

    // Step 4: Prepare for data exchange
    std::vector<MPI_Request>
        requests;
    // 删除其中target_rank ！= local_rank
    meta.erase(std::remove_if(meta.begin(), meta.end(), [&](const Piece_of_data &piece)
                              {
                                  int piece_y_start = piece.start[1];
                                  int target_rank = which_rank[piece_y_start];
                                  return target_rank != all_rank; // Condition for removal
                              }),
               meta.end());

    // deal with pool
    size_t need_space = 0;
    for (int i = 0; i < all_meta.size(); i++)
    {
        // target_rank:      应该在哪个进程
        // local_rank:       当前进程号
        // current_location: 现在数据在哪个进程

        auto piece = all_meta[i];
        int piece_y_start = piece.start[1];

        int target_rank = which_rank[piece_y_start];
        int current_location = piece.current_location;

        if (target_rank != all_rank)
            continue;

        if (current_location != all_rank)
        {
            // Need to receive this piece
            int data_size = piece.count[0] * piece.count[1] * piece.count[2];
            int bytes;
            if (datatype == 0 || datatype == 1)
                bytes = 4;
            if (datatype == 0 || datatype == 2)
                bytes = 8;

            need_space += data_size * bytes;
        }
    }
    if (pool_p1.getRemainingSpace() < need_space)
        pool_p1.ensureCapacity(need_space);

    // recv
    for (int i = 0; i < all_meta.size(); i++)
    {
        // target_rank:      应该在哪个进程
        // local_rank:       当前进程号
        // current_location: 现在数据在哪个进程

        auto piece = all_meta[i];
        int piece_y_start = piece.start[1];

        int target_rank = which_rank[piece_y_start];
        int current_location = piece.current_location;

        if (target_rank != all_rank)
            continue;

        if (current_location != all_rank)
        {
            // Need to receive this piece
            int data_size = piece.count[0] * piece.count[1] * piece.count[2];
            int bytes;
            if (datatype == 0 || datatype == 1)
                bytes = 4;
            if (datatype == 0 || datatype == 2)
                bytes = 8;

            auto recv_buffer = pool_p1.getMemoryBlock(data_size * bytes);
            MPI_Request req;
            // cout << current_location << " -> " << all_rank << " recv " << i << endl;
            if (datatype == 0)
                MPI_Irecv(recv_buffer, data_size, MPI_INT, current_location, i, all_comm, &req);
            if (datatype == 1)
                MPI_Irecv(recv_buffer, data_size, MPI_FLOAT, current_location, i, all_comm, &req);
            if (datatype == 2)
                MPI_Irecv(recv_buffer, data_size, MPI_DOUBLE, current_location, i, all_comm, &req);

            requests.push_back(req);

            Piece_of_data new_piece = piece;
            new_piece.offset = pool_p1.getOffsetFromBase(recv_buffer);
            meta.push_back(new_piece);
        }
    }
    // Step 5: Send data
    for (int i = 0; i < all_meta.size(); i++)
    {
        auto piece = all_meta[i];
        int piece_y_start = piece.start[1];

        int target_rank = which_rank[piece_y_start];
        int current_location = piece.current_location;

        // target_rank:      应该在哪个进程
        // local_rank:       当前进程号
        // current_location: 现在数据在哪个进程

        if (current_location != all_rank)
            continue;

        if (target_rank != all_rank)
        {
            int data_size = piece.count[0] * piece.count[1] * piece.count[2];
            MPI_Request req;
            // cout << all_rank << " -> " << target_rank << " send " << i << endl;
            if (datatype == 0)
                MPI_Isend(pool_p1.getAddressOffset(piece.offset), data_size, MPI_INT, target_rank, i, all_comm, &req);
            if (datatype == 1)
                MPI_Isend(pool_p1.getAddressOffset(piece.offset), data_size, MPI_FLOAT, target_rank, i, all_comm, &req);
            if (datatype == 2)
                MPI_Isend(pool_p1.getAddressOffset(piece.offset), data_size, MPI_DOUBLE, target_rank, i, all_comm, &req);
            requests.push_back(req);
        }
    }

    // Step 6: Wait for all communications to complete
    MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);
}
void DataReceiver::combine_matrix_and_output(MPI_Comm all_comm, MPI_Comm sub_comm, int color)
{

    int global[3];
    int start[3] = {0, 0, 0};
    int count[3] = {0, 0, 0};
    void *large_matrix = nullptr;
    if (meta.size())
    {

        // 假设 meta 已经填充了数据
        int maxZ = 0, maxY = 0, maxX = 0;
        int minZ = std::numeric_limits<int>::max(), minY = std::numeric_limits<int>::max(), minX = std::numeric_limits<int>::max();

        // 最小值的界
        for (const auto &piece : meta)
        {
            minZ = std::min(minZ, piece.start[0]);
            minY = std::min(minY, piece.start[1]);
            minX = std::min(minX, piece.start[2]);
        }

        // 最大值的界
        for (const auto &piece : meta)
        {
            maxZ = std::max(maxZ, piece.start[0] + piece.count[0]);
            maxY = std::max(maxY, piece.start[1] + piece.count[1]);
            maxX = std::max(maxX, piece.start[2] + piece.count[2]);
        }

        // 计算 large_matrix 的总大小
        int sizeZ = maxZ - minZ;
        int sizeY = maxY - minY;
        int sizeX = maxX - minX;

        // 使用一维数组分配内存
        if (meta_output.datatype == 0)
            large_matrix = new int[sizeZ * sizeY * sizeX](); // 初始化为0
        if (meta_output.datatype == 1)
            large_matrix = new float[sizeZ * sizeY * sizeX](); // 初始化为0
        if (meta_output.datatype == 2)
            large_matrix = new double[sizeZ * sizeY * sizeX](); // 初始化为0

        // 第三步：遍历每个数据片段并复制数据
        for (const auto &piece : meta)
        {
            int startZ = piece.start[0];
            int startY = piece.start[1];
            int startX = piece.start[2];

            int countZ = piece.count[0];
            int countY = piece.count[1];
            int countX = piece.count[2];

            if (meta_output.datatype == 0)
            {
                int *submatrix = static_cast<int *>(pool_p1.getAddressOffset(piece.offset));
#pragma omp parallel for
                for (int z = 0; z < countZ; ++z)
                    for (int y = 0; y < countY; ++y)
                        for (int x = 0; x < countX; ++x)
                            ((int *)large_matrix)[(startZ + z - minZ) * (sizeY * sizeX) + (startY + y - minY) * sizeX + (startX + x - minX)] = submatrix[z * countY * countX + y * countX + x];
            }
            if (meta_output.datatype == 1)
            {
                float *submatrix = static_cast<float *>(pool_p1.getAddressOffset(piece.offset));
#pragma omp parallel for
                for (int z = 0; z < countZ; ++z)
                    for (int y = 0; y < countY; ++y)
                        for (int x = 0; x < countX; ++x)
                            ((float *)large_matrix)[(startZ + z - minZ) * (sizeY * sizeX) + (startY + y - minY) * sizeX + (startX + x - minX)] = submatrix[z * countY * countX + y * countX + x];
            }
            if (meta_output.datatype == 2)
            {
                double *submatrix = static_cast<double *>(pool_p1.getAddressOffset(piece.offset));
#pragma omp parallel for
                for (int z = 0; z < countZ; ++z)
                    for (int y = 0; y < countY; ++y)
                        for (int x = 0; x < countX; ++x)
                        {
                            // cout << submatrix[z * countY * countX + y * countX + x] << endl;
                            ((double *)large_matrix)[(startZ + z - minZ) * (sizeY * sizeX) + (startY + y - minY) * sizeX + (startX + x - minX)] = submatrix[z * countY * countX + y * countX + x];
                        }
            }
        }

        start[0] = minZ;
        start[1] = minY;
        start[2] = minX;

        count[0] = sizeZ;
        count[1] = sizeY;
        count[2] = sizeX;
    }
    global[0] = meta_output.global[0];
    global[1] = meta_output.global[1];
    global[2] = meta_output.global[2];

    // 打印三维数组
    // print3DArray(large_matrix, sizeX, sizeY, sizeZ, minX, minY, minZ);
    write_to_3d_nc_file_int(sub_comm, global, start, count,
                            meta_output.filename, meta_output.varname, meta_output.dimname0, meta_output.dimname1, meta_output.dimname2,
                            large_matrix, meta_output.datatype, meta_output.append);
    delete[] large_matrix;
}
void DataReceiver::print3DArray(int *array, int sizeX, int sizeY, int sizeZ, int minX, int minY, int minZ)
{
    for (int x = 0; x < sizeX; ++x)
    {
        for (int y = 0; y < sizeY; ++y)
        {
            for (int z = 0; z < sizeZ; ++z)
            {
                // 计算一维数组索引
                int index = x * (sizeY * sizeZ) + y * sizeZ + z;
                std::cout << "large_matrix[" << (x + minX) << "][" << (y + minY) << "][" << (z + minZ) << "] = "
                          << array[index] << std::endl;
            }
        }
    }
}

void DataReceiver::write_out(int global_rank, int color, MPI_Comm all_comm, MPI_Comm sub_comm)
{
    exchange_data(all_comm, sub_comm);
    combine_matrix_and_output(all_comm, sub_comm, color);
    meta.clear();
    pool_p1.clear();
}

vector<Piece_of_data> meta;
MemoryManager pool_p1;
Piece_of_data meta_output;
