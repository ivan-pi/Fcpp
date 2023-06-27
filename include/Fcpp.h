#pragma once

#include <complex>
#include <vector>
#include <array>

#include <type_traits>

#include <span> // C++ 20
#include <iostream>

#include <cassert>

#include <ISO_Fortran_binding.h> // F2018
// for a peak into the internals of a particular vendor:
// https://github.com/gcc-mirror/gcc/blob/master/libgfortran/ISO_Fortran_binding.h

namespace Fcpp {

namespace Fcpp_impl_ {

/**
 * Function templates to convert a template argument 
 * into an integer type code
 */ 
template<typename T>
constexpr CFI_type_t type();

// Non-interoperable structure (default)
template<typename T>
constexpr CFI_type_t type(){ return CFI_type_other; } 

// Specializations for supported types
template<>
constexpr CFI_type_t type<char>(){ return CFI_type_char; }
template<>
constexpr CFI_type_t type<float>(){ return CFI_type_float; } 
template<>
constexpr CFI_type_t type<double>(){ return CFI_type_double; } 
template<>
constexpr CFI_type_t type<std::complex<float>>(){ return CFI_type_float_Complex; } 
template<>
constexpr CFI_type_t type<std::complex<double>>(){ return CFI_type_double_Complex; }
template<>
constexpr CFI_type_t type<int>(){ return CFI_type_int; } 
template<>
constexpr CFI_type_t type<long>(){ return CFI_type_long; } 
template<>
constexpr CFI_type_t type<long long>(){ return CFI_type_long_long; } 
template<>
constexpr CFI_type_t type<std::size_t>(){ return CFI_type_size_t; }
template<>
constexpr CFI_type_t type<std::int8_t>(){ return CFI_type_int8_t; }
template<>
constexpr CFI_type_t type<std::int16_t>(){ return CFI_type_int16_t; }
//template<>
//constexpr CFI_type_t type<std::int32_t>(){ return CFI_type_int32_t; }
//template<>
//constexpr CFI_type_t type<std::int64_t>(){ return CFI_type_int64_t; }
template<>
constexpr CFI_type_t type<void*>(){ return CFI_type_cptr; }


} // namespace Fcpp_internal

/**
 *  Enumerator class for attributes
 */
enum class attr : CFI_attribute_t {
    other       = CFI_attribute_other,
    allocatable = CFI_attribute_allocatable,
    pointer     = CFI_attribute_pointer
};


/**
 *  C++-descriptor class encapsulating Fortran array
 */
template<typename T, int rank_ = 1, attr attr_ = Fcpp::attr::other>
class cdesc_t {
public:

    static_assert(rank_ >= 0, "Rank must be non-negative");
    static_assert(rank_ <= CFI_MAX_RANK, 
        "The maximum allowed rank is 15");

    using value_type = T;
    using size_type = std::size_t;

    using reference = T&;
    using const_reference = const T&;

    using pointer = T*;
    using const_pointer = const T*;

    constexpr CFI_type_t type() const { return Fcpp_impl_::type<T>(); };
    constexpr CFI_rank_t rank() const { return rank_; };

    // Version of ISO_Fortran_binding.h 
    constexpr int version() const { return this->get()->version; }

    // Element length in bytes
    std::size_t elem_len() const { return this->get()->elem_len; }


    // Base constructor for assumed-shape arrays
    cdesc_t(T* ptr, size_type n) : ptr_(this->get_descptr()) {

        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from pointer and length");

        CFI_index_t extents[1] = { static_cast<CFI_index_t>(n) };

        [[maybe_unused]] int status = CFI_establish(
            this->get(),
            ptr,
            static_cast<attribute_type>(attr_),
            this->type(),
            sizeof(T),
            rank_,
            extents
        );

        assert(status == CFI_SUCCESS);

    }

    // Constructor for static array
    template<std::size_t N>
    cdesc_t(T (&ref)[N]) : cdesc_t(ref,N) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from static array");
    }

    // Constructor from std::vector
    cdesc_t(std::vector<T> &buffer) : cdesc_t(buffer.data(),buffer.size()) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from std::vector");
    }

    // Constructor from std::array
    template<std::size_t N>
    cdesc_t(std::array<T,N> &buffer) : cdesc_t(buffer.data(),N) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from std::array");
    }

#if __cpp_lib_span
    cdesc_t( std::span<T> buffer ) : cdesc_t(buffer.data(),buffer.size()) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from std::span.");
    }
#endif

    bool is_contiguous() {
        return CFI_is_contiguous(this->get()) > 0;
    }

    // Implicit cast to C-descriptor pointer
    operator CFI_cdesc_t* () { return this->get(); }

    // Implicit cast to std::span (only for rank-1 arrays, 
    // otherwise use .flatten())
#if __cpp_lib_span
    operator std::span<T>() const {
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to convert implicitly to std::span");
        size_type n = this->get()->dim[0].extent;
        return {this->data(),n};
    }

    // Return a flattened view of the array.
    // TODO: handle assumed-size arrays
    std::span<T> flatten() {
        size_type nelem = 1;
        for (int i = 0; i < rank_; ++i) {
            nelem *= this->get()->dim[i].extent;
        }
        return {this->data(),nelem};
    }
#endif

#if __cpp_lib_mdspan
    operator std::mdspan<T,std::dextents<rank_>>() const {
        static_assert(rank_ > 1, "Rank must be higher than 1 to convert to std::mdspan");
        return {this->data_handle(), extents... };
    }
#endif

    // Return pointer to the underlying descriptor
    // (the name get() is inspired by the C++ smart pointer classes)
    constexpr auto get() const { return ptr_; }

    inline std::size_t extent(int dim) const {
        assert(0 <= dim && dim < rank_);
        return ptr_->dim[dim].extent;
    }

    // FIXME: We need a proper iterator to handle the non-contiguous case
    //        when the stride memory is larger than the element size.

    //
    // Element access
    //

    constexpr T& front() const {
        return begin();
    }

    //constexpr T& back() const {
    //    return ;
    //}


    // Array subscript operators
    T& operator[](std::size_t idx) {
        static_assert(rank_ == 1,
            "Rank must be 1 to use array subscript operator");
            return *(data() + idx);
        }
    const T& operator[](std::size_t idx) const {
        static_assert(rank_ == 1,
            "Rank must be 1 to use array subscript operator");
        return *(data() + idx);
    }

    constexpr pointer data() const { return static_cast<pointer>(get()->base_addr); }

    // Iterator support
    T* begin() { return &this->operator[](0); }
    T* end() { return &this->operator[](this->get()->dim[0].extent); }
    const T* cbegin() { return &this->operator[](0); }
    const T* cend() { return &this->operator[](this->get()->dim[0].extent); }

private:

    using attribute_type = typename std::underlying_type<attr>::type;

    // Return base address of the Fortran array
    // cast to the correct type

    constexpr auto get_descptr() {
        return reinterpret_cast<CFI_cdesc_t *>(&desc_);
    } 

private:
    // Descriptor containing the actual data
    CFI_CDESC_T(rank_) desc_;
    // Pointer to opaque descriptor object
    CFI_cdesc_t *ptr_{nullptr};
};

template<typename T, int rank_>
class AllocatableArray {
public:
    // TODO
private:
    CFI_cdesc_t *ptr{nullptr};
};

template<typename T, int rank_>
class PointerArray {
public:
    // TODO
private:
    CFI_cdesc_t *ptr{nullptr};
};

// Both pointer array, and allocatable array
// don't work with C++ memory. They
// can only have two constructors. Either
// being a new array is created, given the dimensions.
// Or they are initialized from an existing array
// of specific rank.
//
// Pointer array does not have a destructor. It 
// must be destroyed by the programmer.
//
// Allocatable arrays are de-allocated at exit from the scope.
// When constructed from dummy argument, they should
// not be deallocated.
//
// Can we detect if something is a dummy argument?
//
// Any local instances created, should be destroyed automatically.
//


} // namespace Fcpp