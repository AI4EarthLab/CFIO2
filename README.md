# CFIO V2.0.0
A fast input/output library for high-resolution climate models Version 2

## Installation Guide

1.  Getting Started
2.  Testing the CFIO2 installation
3.  Some suggestions
4.  Reporting Installation or Usage Problems


### 1. Getting Started

The following instructions take you through a sequence of steps to get the default configuration of CFIO2 up and running. **Important note: Please use the same set of compilers to build PnetCDF and CFIO2.** 

(a) You will need the following prerequisites.

```shell
    * The gcc/g++/gfortran compiler, version 7.1 or later

    * An MPI C/C++/Fortran compiler

    * Parallel netCDF version 1.11.2 (http://cucis.ece.northwestern.edu/projects/PnetCDF/) or later

    * Some basic development tools, including cmake, make. 
      These are usually part of your operating system's development tools.
```

(b) Specify the MPI compiler.
    For MPICH and Openmpi:

      export MPICC=mpicc  
      export MPICXX=mpicxx  
      export MPIF90=mpif90  
      export MPIF77=mpif77  

   For Intel compiler and Intel MPI library:

      export MPICC=mpiicc  
      export MPICXX=mpiicpc  
      export MPIF90=mpiifort  
      export MPIF77=mpiifort  


(c) Install Parallel netCDF. The default installation directory of PnetCDF is `${HOME}/install`:
     
      cd
      wget http://cucis.ece.northwestern.edu/projects/PnetCDF/Release/pnetcdf-1.11.2.tar.gz
      tar xf pnetcdf-1.11.2.tar.gz
      cd pnetcdf-1.11.2
      ./configure --prefix=${HOME}/install  
      make 
      make install 


(d) Install CFIO2:

      cd CFIO2
      Edit the following lines in the CMakeLists.txt file to select the correct compiler, consistent with the compiler used in Parallel netCDF:
        set(CMAKE_C_COMPILER mpicc)
        set(CMAKE_CXX_COMPILER mpicxx)
        set(CMAKE_Fortran_COMPILER mpif90)

      mkdir build
      cd build
      cmake ..
      make


   This executable program `./bin/example0` and `./bin/example1` is two demos of output nc files with C++ and Fortran written based on CFIO2.
   If you have completed all of the above steps, you have successfully installed CFIO2.
      

### 2. Testing the CFIO2 installation

For testing CFIO2, the command is:
      
     mpirun -n 5 ./bin/example0
     mpirun -n 5 ./bin/example1

After execution, you will see the generated nc files.

### 3. Some suggestions

(a) Choosing how many I/O processes to use for caching each variable, as well as the maximum number of I/O processes allowed per node, are two important parameters. Properly setting these parameters can maximize hardware resource utilization and accelerate output speed.

(b) During the output phase, each different communication domain outputs NC files simultaneously. To maximize the output bandwidth of different communication domains and reduce mutual interference, it is best to avoid writing to the same NC file within a close time frame during the `cfio2_put_vara` stage.

(c) Choosing the right moment to call `cfio2_wait_output` is also important. This function is similar to the `MPI_Wait` operation, ensuring that data is actually written to disk. Although data will automatically be output to disk at the appropriate time even without using this function, calling it can guarantee that the data is written in advance, potentially improving output speed.

### 4. Reporting Installation or Usage Problems

Please report the problems on our github: https://github.com/AI4EarthLab/CFIO2/issues