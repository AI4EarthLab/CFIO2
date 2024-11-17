
program main
    use iso_c_binding
    use cfio2
    implicit none
    include "mpif.h"

    integer, parameter :: NZ = 5
    integer, parameter :: NY = 6
    integer, parameter :: NX = 7

    integer :: rank, size, ierr
    integer :: all_comm, compute_comm
    integer :: IO_process_per_node = 10 ! Number of I/O processes on each node
    integer :: IO_process_per_var = 2   ! Number of IO processes per variable
    integer, dimension(3) :: global, start, count
    integer, dimension(2) :: dims,coords
    character(len=128) :: dim_name(3)
    real(8), allocatable :: data(:)
    double precision :: starttime, endtime

    integer :: compute_rank, compute_size

    integer :: i
    
    character(len=128) :: filename
    character(len=128) :: varname


    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)

    call MPI_Comm_split(MPI_COMM_WORLD, rank >= 0, rank, all_comm, ierr)
    call MPI_Comm_split(MPI_COMM_WORLD, rank > 0, rank, compute_comm, ierr)

    call cfio2_init(all_comm, IO_process_per_node, IO_process_per_var)

    if (rank > 0) then
        call MPI_Comm_rank(compute_comm, compute_rank, ierr)
        call MPI_Comm_size(compute_comm, compute_size, ierr)

        dims = (/0, 0/)
        call getCoords(compute_rank, compute_size, coords, dims, compute_comm);

        global(1) = NZ    
        global(2) = dims(2) * NY 
        global(3) = dims(1) * NX  

        start(1) = 1
        start(2) = 1 + coords(2) * NY  
        start(3) = 1 + coords(1) * NX 

        count(1) = 5   ! NZ
        count(2) = NY  
        count(3) = NX  

        dim_name(1) = "Z"
        dim_name(2) = "Y"
        dim_name(3) = "X"

        allocate(data(count(1)*count(2)*count(3)))

        data = rank

        call MPI_Barrier(compute_comm)
        
        starttime = MPI_Wtime()
        data = 0.0d0

        do i = 0, 3  ! Loop for writing data (only once as per original code)

            
            write(varname, '(A,I0)') "pressure_", i
            write(filename, '(I0,A)') i, "_0.nc"


            data = data + 1  ! Reset data to zero for the first file write.
            ! print *,data
            call cfio2_put_vara(all_comm, filename, varname, cfio2_double, dim_name, global, start, count, c_loc(data), 0)

            data = data + 1 
            ! print *,data

            write(filename, '(I0,A)') i, "_1.nc"

            call cfio2_put_vara(all_comm, filename, varname, cfio2_double, dim_name, global, start, count, c_loc(data), 0)

            call cfio2_wait_output(all_comm)
            
        end do
        
        endtime = MPI_Wtime()
        
        print *, "Control took ", endtime - starttime , " seconds"

       
        
    end if
    call cfio2_finalize(all_comm)
    call MPI_Finalize(ierr)
end program main

subroutine getCoords(rank, size, coords, dims, comm_compute)
    use mpi
    implicit none
    integer :: ierr
    integer, intent(in) :: rank      
    integer, intent(in) :: size      
    integer, intent(in) :: comm_compute
    integer, dimension(2), intent(inout) :: coords 
    integer, dimension(2), intent(inout) :: dims  
    integer :: comm_cart             
    LOGICAL :: periods(2)
    call MPI_Dims_create(size, 2, dims, ierr)
    periods = (/.false., .false./)
    call MPI_Cart_create(comm_compute, 2, dims, periods, .false. , comm_cart, ierr)
    call MPI_Cart_coords(comm_cart, rank, 2, coords, ierr)
    call MPI_Comm_free(comm_cart, ierr)
end subroutine
