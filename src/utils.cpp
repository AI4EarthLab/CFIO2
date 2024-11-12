#include <utils.h>
#include <mpi.h>
using namespace std;

void writeBinaryFile(int rank, char filename[], int global[3], int start[3], int count[3], int *buf)
{
    FILE *file;
    char newFilename[100];
    sprintf(newFilename, "%s_%d", filename, rank);

    file = fopen(newFilename, "wb");
    if (file == NULL)
    {
        printf("Error opening file.\n");
        exit(1);
    }
    fwrite(global, sizeof(int), 3, file);                               // Write global array to file in binary format
    fwrite(start, sizeof(int), 3, file);                                // Write start array to file in binary format
    fwrite(count, sizeof(int), 3, file);                                // Write count array to file in binary format
    fwrite(buf, count[0] * count[1] * count[2] * sizeof(int), 1, file); // Write buf to file in binary format

    fclose(file);
}

void readBinaryFile(char filename[])
{
    FILE *file;
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Error opening file.\n");
        exit(1);
    }

    int global[3];
    int start[3];
    int count[3];
    fread(global, sizeof(int), 3, file); // Read global array from file
    fread(start, sizeof(int), 3, file);  // Read start array from file
    fread(count, sizeof(int), 3, file);  // Read count array from file

    int bufSize = count[0] * count[1] * count[2] * sizeof(int);
    int *buf = (int *)malloc(bufSize);
    fread(buf, bufSize, 1, file); // Read buf from file

    fclose(file);

    // Do something with the read data
    printf("Global array: %d, %d, %d\n", global[0], global[1], global[2]);
    printf("Start array: %d, %d, %d\n", start[0], start[1], start[2]);
    printf("Count array: %d, %d, %d\n", count[0], count[1], count[2]);
    for (int i = 0; i < count[0] * count[1] * count[2]; i++)
    {
        printf("%d ", *((int *)buf + i));
    }
    free(buf);
}

void random_sleep(int min_seconds, int max_seconds, int world_rank)
{
    // 使用进程 ID 设置种子
    srand(time(0) + world_rank);

    // 生成介于最小和最大值之间的随机睡眠时间
    int random_sleep_seconds = rand() % (max_seconds - min_seconds + 1) + min_seconds;

    // printf("Process %d: Sleeping for %d seconds...\n", world_rank, random_sleep_seconds);
    sleep(random_sleep_seconds);

    // printf("Process %d: Woke up after sleeping.\n", world_rank);
}

void printStartCount(int start[], int count[])
{
    printf("start[0] = %d, start[1] = %d, start[2] = %d\n", start[0], start[1], start[2]);
    printf("count[0] = %d, count[1] = %d, count[2] = %d\n", count[0], count[1], count[2]);
}

// /* 定义函数 */
// int write_to_3d_nc_file_int(MPI_Comm comm, const int global[3], const int start[3], const int count[3], const int *buf)
// {
//     int ncid, varid;
//     int status;

//     /* 创建或覆盖NC文件 */
//     status = ncmpi_create(comm, meta_output.filename, NC_CLOBBER, MPI_INFO_NULL, &ncid);
//     if (status != NC_NOERR)
//         return status;

//     /* 定义维度 */
//     int dimids[3];
//     char dimname[20];

//     status = ncmpi_def_dim(ncid, meta_output.dimname0, global[0], &dimids[0]);
//     status = ncmpi_def_dim(ncid, meta_output.dimname1, global[1], &dimids[1]);
//     status = ncmpi_def_dim(ncid, meta_output.dimname1, global[1], &dimids[2]);

//     /* 定义变量 */
//     status = ncmpi_def_var(ncid, meta_output.varname, NC_INT, 3, dimids, &varid);
//     if (status != NC_NOERR)
//         return status;

//     /* 结束定义模式 */
//     status = ncmpi_enddef(ncid);
//     if (status != NC_NOERR)
//         return status;

//     /* 写数据 */
//     MPI_Offset mpi_start[3] = {start[0], start[1], start[2]};
//     MPI_Offset mpi_count[3] = {count[0], count[1], count[2]};
//     status = ncmpi_put_vara_int_all(ncid, varid, mpi_start, mpi_count, buf);
//     if (status != NC_NOERR)
//         return status;

//     /* 关闭文件 */
//     status = ncmpi_close(ncid);
//     if (status != NC_NOERR)
//         return status;

//     return NC_NOERR; // 成功返回
// }

// /* 定义函数 */
// int write_to_3d_nc_file(MPI_Comm comm, const int global[3], const int start[3], const int count[3], const char *filename, const char *var_name, const int *buf)
// {
//     int ncid, varid;
//     int status;

//     /* 创建或覆盖NC文件 */
//     status = ncmpi_create(comm, filename, NC_CLOBBER, MPI_INFO_NULL, &ncid);
//     if (status != NC_NOERR)
//         return status;

//     /* 定义维度 */
//     int dimids[3];
//     char dimname[20];
//     for (int i = 0; i < 3; i++)
//     {
//         sprintf(dimname, "dim%d", i);
//         status = ncmpi_def_dim(ncid, dimname, global[i], &dimids[i]);
//         if (status != NC_NOERR)
//             return status;
//     }

//     /* 定义变量 */
//     status = ncmpi_def_var(ncid, var_name, NC_INT, 3, dimids, &varid);
//     if (status != NC_NOERR)
//         return status;

//     /* 结束定义模式 */
//     status = ncmpi_enddef(ncid);
//     if (status != NC_NOERR)
//         return status;

//     /* 写数据 */
//     MPI_Offset mpi_start[3] = {start[0], start[1], start[2]};
//     MPI_Offset mpi_count[3] = {count[0], count[1], count[2]};
//     status = ncmpi_put_vara_int_all(ncid, varid, mpi_start, mpi_count, buf);
//     if (status != NC_NOERR)
//         return status;

//     /* 关闭文件 */
//     status = ncmpi_close(ncid);
//     if (status != NC_NOERR)
//         return status;

//     return NC_NOERR; // 成功返回
// }
