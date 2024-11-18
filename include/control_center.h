#pragma once
#include <unistd.h>
#include <iostream>
#include <mpi.h>
#include <string>
#include <cstring>
// #include <common_struct.h>
#include <Message.h>
#include <vector>
#include <queue>
#include <utils.h>
#include <Master_struct.h>
#include <DataReceiver.h>

#define cfio2_data_type_int 0
#define cfio2_data_type_float 1
#define cfio2_data_type_double 2

using namespace std;

extern int IO_process_per_node;

class Control
{

public:
    void gather_process_names(MPI_Comm all_comm);
    void startScheduling(MPI_Comm all_comm);

private:
    struct_request receiveMessages(MPI_Comm all_comm);
};
extern "C"
{
    void cfio2_init_fortran(MPI_Fint Fcomm, int IO_process_per_node_, int IO_process_per_var_);
    void cfio2_init(MPI_Comm all_comm, int IO_process_per_node_, int IO_process_per_var_);

    void cfio2_put_vara_fortran(MPI_Fint Fcomm, char *filename_in, char *var_name, int datatype, char *dim_name1, char *dim_name2, char *dim_name3, int global[3], int start[3], int count[3], void *buf, int append);
    void cfio2_put_vara(MPI_Comm all_comm, string filename_in, string var_name, int datatype, string dim_name[3], int global[3], int start[3], int count[3], void *buf, int append);

    void cfio2_wait_output_fortran(MPI_Fint Fcomm);
    void cfio2_wait_output(MPI_Comm all_comm);

    void cfio2_finalize_fortran(MPI_Fint Fcomm);
    void cfio2_finalize(MPI_Comm all_comm);
}