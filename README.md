# CFIO V2.0.0
A fast input/output library for high-resolution climate models Version 2

## Installation Guide

1.  Getting Started
2.  Testing the CFIO2 installation
3.  Reporting Installation or Usage Problems


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


### 3. Reporting Installation or Usage Problems

Please report the problems on our github: https://github.com/AI4EarthLab/CFIO2/issues