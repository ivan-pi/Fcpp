! int alltwo(CFI_cdesc_t *b);
integer(c_int) function alltwo(b) bind(c)
use, intrinsic :: iso_c_binding, only: c_int
implicit none
integer(c_int), intent(in) :: b(:)
alltwo = merge(1,0,all(b == 2_c_int))
end function