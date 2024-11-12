#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include "pnetcdf.h"
#include <DataReceiver.h>
#include <iostream>
#include <netcdf.h>
#include <mpi.h>
#include <sys/stat.h> // 用于检查文件是否存在

int write_to_3d_nc_file_int(MPI_Comm comm, const int global[3], const int start[3], const int count[3],
                            const char *filename, const char *varname, const char *dimname0,
                            const char *dimname1, const char *dimname2, const void *buf, int datatype, int append);
