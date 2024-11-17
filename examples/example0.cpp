#include <iostream>
#include <mpi.h>
#include <string>
#include <cstring>
#include <thread>
#include <Message.h>
#include <control_center.h>
#include <unistd.h>
using namespace std;

#define NZ 5
#define NY 6
#define NX 7
void getCoords(int rank, int size, int *coords, int *dims, MPI_Comm comm_compute)
{
    MPI_Dims_create(size, 2, dims);

    MPI_Comm cart_comm;
    int periods[2] = {0, 0}; // 非周期性边界
    MPI_Cart_create(comm_compute, 2, dims, periods, 0, &cart_comm);

    MPI_Cart_coords(cart_comm, rank, 2, coords);

    MPI_Comm_free(&cart_comm);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm all_comm;
    MPI_Comm_split(MPI_COMM_WORLD, rank >= 0, rank, &all_comm);

    MPI_Comm compute_comm;
    MPI_Comm_split(MPI_COMM_WORLD, rank > 0, rank, &compute_comm);

    int IO_process_per_node = 10; // Number of I/O processes on each node
    int IO_process_per_var = 2;   // Number of IO processes per variable

    cfio2_init(all_comm, IO_process_per_node, IO_process_per_var);

    if (rank > 0)
    {
        int compute_rank, compute_size;
        MPI_Comm_rank(compute_comm, &compute_rank);
        MPI_Comm_size(compute_comm, &compute_size);

        int coords[2] = {0, 0};
        int dims[2] = {0, 0};

        getCoords(compute_rank, compute_size, coords, dims, compute_comm);

        int rows = dims[0];
        int cols = dims[1];

        int global[3];
        global[0] = NZ;
        global[1] = cols * NY;
        global[2] = rows * NX;

        int start[3], count[3];

        start[0] = 0;
        start[1] = coords[1] * NY;
        start[2] = coords[0] * NX;

        count[0] = NZ;
        count[1] = NY;
        count[2] = NX;
        string dim_name[3];
        dim_name[0] = "Z";
        dim_name[1] = "Y";
        dim_name[2] = "X";

#define DATATYPE double
        DATATYPE *data = (DATATYPE *)malloc(sizeof(DATATYPE) * count[0] * count[1] * count[2]);
        for (int i = 0; i < count[0] * count[1] * count[2]; i++)
        {
            data[i] = NZ * NY * NX * (rank - 1) + i;
        }

        double starttime, endtime;

        MPI_Barrier(compute_comm);
        starttime = MPI_Wtime();

        for (int i = 0; i < 1; i++)
        {
            int append = 0;
            string filename;
            string varname = "temperature_" + to_string(i);

            filename = to_string(i) + "_0.nc";
            for (int j = 0; j < count[0] * count[1] * count[2]; j++)
                data[j] = 0; // 每个进程写入不同的数据
            cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            for (int j = 0; j < count[0] * count[1] * count[2]; j++)
                data[j] = 1; // 每个进程写入不同的数据
            filename = to_string(i) + "_1.nc";
            cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_2.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_3.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_4.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_5.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_6.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_7.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_8.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            // filename = to_string(i) + "_9.nc";
            // cfio2_put_vara(all_comm, filename, varname, cfio2_data_type_double, dim_name, global, start, count, data, append);

            cfio2_wait_output(all_comm);

            // for (int j = 0; j < count[0] * count[1] * count[2]; j++)
            // {
            //     data[j] += 1000; // 每个进程写入不同的数据
            // }
        }
        endtime = MPI_Wtime();
        printf("Control tooks %f secodes\n", endtime - starttime);

        cfio2_finalize(all_comm);
    }
    MPI_Finalize();
    return 0;
}
