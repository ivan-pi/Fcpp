# Fcpp

Seamless interoperability between C++ containers and Fortran arrays

We provides two classes: `cdesc` and `cdesc_ptr`. These classes
act as adaptors between C++ containers and Fortran arrays, helping to re-establish type, rank and 
attribute safety when moving between languages.

The classes also offer some of the friendlier C++ semantics for 
manipulating collections of items, including:
- iterators
- range-based for loops
- subscript operators
This way you can use algorithms from the C++ Standard Template Library.

:warning: This library is currently at prototype stage. We are working
on a making a first release soon.

## `cdesc`

The purpose of this class is to pass native C++ containers such as
`std::vector`, `std::array`, and `std::span` to routines implemented in Fortran
that use assumed-shape arrays as dummy arguments. The `cdesc` class manages the 
_descriptor_ which is used for interoperation, however the actual data or 
storage remains under the ownership of some native C++ object.

Use the `cdesc` class when you have a (contiguous) C++ container that you need 
to pass to Fortran routine marked with `bind(c)`. An example of what this may 
look like is:

```cpp
#include "Fcpp.h"
using namespace Fcpp;

extern "C" void process_floats_in_fortran(CFI_cdesc_t * /* a */ );
// ...
std::vector<float> a(21);
// ...
{
   cdesc f_a(a); // CTAD
   process_floats_in_fortran(f_a);
}
```

## `cdesc_ptr`

The purpose of this class is to help implement procedures in C++, which are 
designed to be called from Fortran. In such cases you will be passing a pointer
to a C descriptor across the language barrier.

When you need to perform some operations on Fortran arrays, but the implementation would be easier to perform in C++, use the `cdesc_ptr` class.

```fortran
interface
   subroutine process_ints(b) bind(c,name="process_ints")
      use, intrinsic :: iso_c_binding, only: c_int
      integer(c_int) :: b(:) ! <- could be non-contiguous
   end subroutine
end interface

integer :: b(21)
call process_ints(b)
```

## Calling a Fortran routine from C++

```fortran
! A wrapper of the Fortran dot_product routine
function dot(a,b) bind(c,name='f_dot')
real(c_double), intent(in) :: a(:), b(:)
real(c_double) :: dot
intrinsic :: dot_product
dot = dot_product(a,b)
end function
```

```cpp
#include <array>
#include <iostream>

#include "Fcpp.h"
using namespace Fcpp;

extern "C" [[nodiscard]]
double f_dot(CFI_cdesc_t *fa, CFI_cdesc_t *fb);

int main() {
    
   std::array<double,3> a{1.,2.,3.}; 
   std::vector<double> b;

   b.emplace_back(1.0);
   b.emplace_back(2.0);
   b.emplace_back(3.0);

   double d;
   
   {
      const cdesc_t fa(a); // CTAD
      const cdesc_t fb(b); // CTAD

      // 1^2 + 2^2 + 3^2 = 1 + 4 + 9 = 14
      d = f_dot(fa,fb);
   }

   std::cout << "dot(a,b) = " << d << '\n';
   
   return 0;
}
```

## Calling a C++ routine from Fortran

Purposes of the adapter class template:
- re-establish type, rank and attributes (interface checking)
- conversions to C++ reference classes such as std::span
- easily modify the allocation status

```cpp
#include <numeric>

#include "Fcpp.h"
using namespace Fcpp;

extern "C" {
    
void fill_increasing(CFI_cdesc_t* fa, int start) {
   cdesc_ptr<int,1> a(fa);
   std::iota(a.begin(),a.end(),start);
}

}
```

```fortran
program fill
implicit none
interface
   subroutine fill_increasing(vec,start) bind(c)
      use, intrinsic :: iso_c_binding, only: c_int
      integer(c_int), intent(inout), contiguous :: vec(:)
      integer(c_int), value :: start
   end subroutine
end interface

integer(c_int) :: a(5), b(6)

call fill_increasing(a,-2)
call fill_increasing(b(1::2),0)

write(*,*) a

end program
```



