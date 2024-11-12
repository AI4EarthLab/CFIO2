
#include <pnetcdf_func.h>
#include <mpi.h>
int write_to_3d_nc_file_int(MPI_Comm comm, const int global[3], const int start[3], const int count[3],
                            const char *filename, const char *varname, const char *dimname0,
                            const char *dimname1, const char *dimname2, const void *buf, int datatype, int append)
{
    int ncid, varid;
    int status;

    // 检查文件是否存在
    struct stat buffer;
    bool file_exists = (stat(filename, &buffer) == 0);

    if (append)
    {
        if (file_exists)
        {
            // 如果文件存在，尝试以写入模式打开
            status = ncmpi_open(comm, filename, NC_WRITE, MPI_INFO_NULL, &ncid);
            if (status != NC_NOERR)
            {
                std::cout << "Error opening file for appending" << std::endl;
                return status;
            }
            else
                cout << "open file" << endl;
        }
        else
        {
            // 如果文件不存在，则创建新文件
            status = ncmpi_create(comm, filename, NC_CLOBBER, MPI_INFO_NULL, &ncid);
            if (status != NC_NOERR)
            {
                std::cout << "Create error" << std::endl;
                return status;
            }
        }
        // 定义维度
        int dimids[3];
        status = ncmpi_def_dim(ncid, dimname0, global[0], &dimids[0]);
        status = ncmpi_def_dim(ncid, dimname1, global[1], &dimids[1]);
        status = ncmpi_def_dim(ncid, dimname2, global[2], &dimids[2]);

        // 定义变量
        if (datatype == 0)
            status = ncmpi_def_var(ncid, varname, NC_INT, 3, dimids, &varid);
        if (datatype == 1)
            status = ncmpi_def_var(ncid, varname, NC_FLOAT, 3, dimids, &varid);
        if (datatype == 2)
            status = ncmpi_def_var(ncid, varname, NC_DOUBLE, 3, dimids, &varid);

        if (status != NC_NOERR)
        {
            std::cout << "ncmpi_def_var error" << std::endl;
            return status;
        }

        // 结束定义模式
        status = ncmpi_enddef(ncid);
        if (status != NC_NOERR)
        {
            std::cout << "ncmpi_enddef error" << std::endl;
            return status;
        }
    }
    else
    {
        // 如果不是追加模式，直接创建新文件
        status = ncmpi_create(comm, filename, NC_CLOBBER, MPI_INFO_NULL, &ncid);
        if (status != NC_NOERR)
        {
            std::cout << "Create error" << std::endl;
            return status;
        }

        // 定义维度
        int dimids[3];
        status = ncmpi_def_dim(ncid, dimname0, global[0], &dimids[0]);
        status = ncmpi_def_dim(ncid, dimname1, global[1], &dimids[1]);
        status = ncmpi_def_dim(ncid, dimname2, global[2], &dimids[2]);

        // 定义变量
        if (datatype == 0)
            status = ncmpi_def_var(ncid, varname, NC_INT, 3, dimids, &varid);
        if (datatype == 1)
            status = ncmpi_def_var(ncid, varname, NC_FLOAT, 3, dimids, &varid);
        if (datatype == 2)
            status = ncmpi_def_var(ncid, varname, NC_DOUBLE, 3, dimids, &varid);
        if (status != NC_NOERR)
        {
            std::cout << "ncmpi_def_var error" << std::endl;
            return status;
        }

        // 结束定义模式
        status = ncmpi_enddef(ncid);
        if (status != NC_NOERR)
        {
            std::cout << "ncmpi_enddef error" << std::endl;
            return status;
        }
    }

    // 写数据
    MPI_Offset mpi_start[3] = {start[0], start[1], start[2]};
    MPI_Offset mpi_count[3] = {count[0], count[1], count[2]};

    // 如果是追加模式，可能需要调整起始位置（这里假设你要从 start 开始写入）
    if (append && file_exists)
    {
        // 这里可以根据需要调整 mpi_start 的值，例如：
        // mpi_start[0] += existing_data_size;  // 根据已有数据的大小调整起始位置
    }
    if (datatype == 0)
    {
        status = ncmpi_put_vara_int_all(ncid, varid, mpi_start, mpi_count, (int *)buf);
        if (status != NC_NOERR)
        {
            std::cout << "ncmpi_put_vara_int_all error" << std::endl;
            return status;
        }
    }
    if (datatype == 1)
    {
        status = ncmpi_put_vara_float_all(ncid, varid, mpi_start, mpi_count, (float *)buf);
        if (status != NC_NOERR)
        {
            std::cout << "ncmpi_put_vara_float_all error" << std::endl;
            return status;
        }
    }
    if (datatype == 2)
    {
        status = ncmpi_put_vara_double_all(ncid, varid, mpi_start, mpi_count, (double *)buf);
        if (status != NC_NOERR)
        {
            std::cout << "ncmpi_put_vara_double_all error" << std::endl;
            cout << ncmpi_strerror(status) << endl;

            return status;
        }
    }

    // 关闭文件
    status = ncmpi_close(ncid);
    if (status != NC_NOERR)
    {
        std::cout << "ncmpi_close error" << std::endl;
        return status;
    }

    return NC_NOERR; // 成功返回
}