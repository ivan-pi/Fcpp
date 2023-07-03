program main
    use, intrinsic :: iso_c_binding, only: c_int

    interface
        subroutine iota(x,lw) bind(c)
            import c_int
            integer(c_int), intent(out) :: x(:)
            integer(c_int), intent(in), optional :: lw
        end subroutine
    end interface


    integer(c_int) :: a(8)


    call iota(a)
    print *, a

    call iota(a(2::2),5_c_int)
    print *, a

end program