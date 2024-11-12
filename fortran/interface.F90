module cfio2
    use iso_c_binding
    implicit none

    integer, parameter :: cfio2_int    = 0
    integer, parameter :: cfio2_float  = 1
    integer, parameter :: cfio2_double = 2

    interface
        function add(a, b) bind(C, name="add")
            import :: c_double
            real(c_double), value :: a, b
            real(c_double) :: add
        end function add

        subroutine add_arrays(arr, n) bind(C, name="add_arrays")
            import :: c_double, c_int, c_ptr
            real(c_double), dimension(*), intent(in) :: arr  ! 输入数组
            integer(c_int), value :: n                       ! 数组大小
        end subroutine 

        subroutine cfio2_init(all_comm, IO_process_per_node_, IO_process_per_var_) bind(C, name="cfio2_init_fortran")
            import :: c_int            
            integer(c_int), value :: all_comm  ! Treating communicator as an integer
            integer(c_int), value :: IO_process_per_node_
            integer(c_int), value :: IO_process_per_var_
        end subroutine cfio2_init

        subroutine c_cfio2_put_vara(all_comm, filename_in, var_name, datatype, dim_name1,dim_name2,dim_name3, global, start, count, buf, append) bind(C, name="cfio2_put_vara_fortran")
            use iso_c_binding
            integer(c_int), value :: all_comm  ! Treating communicator as an integer
            character(kind=c_char), intent(in) :: filename_in
            character(kind=c_char), intent(in) :: var_name
            integer(c_int), value :: datatype
            character(kind=c_char), intent(in) :: dim_name1
            character(kind=c_char), intent(in) :: dim_name2
            character(kind=c_char), intent(in) :: dim_name3
            integer(c_int), intent(in) :: global(3)
            integer(c_int), intent(in) :: start(3)
            integer(c_int), intent(in) :: count(3)
            type(c_ptr), value :: buf
            integer(c_int), value :: append
        end subroutine c_cfio2_put_vara

        subroutine cfio2_wait_output(all_comm) bind(C, name="cfio2_wait_output_fortran")
            use iso_c_binding
            integer(c_int), value :: all_comm  ! Treating communicator as an integer
        end subroutine cfio2_wait_output

        subroutine cfio2_finalize(all_comm) bind(C, name="cfio2_finalize_fortran")
            use iso_c_binding
            integer(c_int), value :: all_comm  ! Treating communicator as an integer
        end subroutine cfio2_finalize
    end interface


contains
        subroutine cfio2_put_vara(all_comm, filename_in, var_name, datatype, dim_name, global, start, count, buf, append)
            use iso_c_binding
            integer(c_int), value :: all_comm  ! Treating communicator as an integer
            character(len=*), intent(in) :: filename_in
            character(len=*), intent(in) :: var_name
            integer(c_int), intent(in) :: datatype
            character(len=*), intent(in) :: dim_name(3)
            integer(c_int), intent(in) :: global(3)
            integer(c_int), intent(in) :: start(3)
            integer(c_int), intent(in) :: count(3)
            type(c_ptr), value :: buf
            integer(c_int), intent(in) :: append
            call c_cfio2_put_vara(all_comm, string_f2c(filename_in), string_f2c(var_name), datatype, string_f2c(dim_name(1)), string_f2c(dim_name(2)), string_f2c(dim_name(3)), global, start, count, buf, append)
        end subroutine cfio2_put_vara

        function string_f2c(f_string) result(c_string)
            use iso_c_binding
            character(len=*):: f_string
            CHARACTER(LEN=LEN_TRIM(f_string)+1,KIND=C_CHAR) :: c_string
            c_string = trim(f_string) // C_NULL_CHAR
        end function
end module cfio2