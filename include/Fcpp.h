#pragma once

#include <complex>
#include <vector>
#include <array>

#include <type_traits>
#include <iterator>
#include <cstdint>

#if __cpp_lib_span
#include <span> // C++ 20
#endif

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
constexpr CFI_type_t type<int8_t>(){ return CFI_type_int8_t; }
template<>
constexpr CFI_type_t type<int16_t>(){ return CFI_type_int16_t; }
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
class cdesc {
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
    template<typename... Exts>
    cdesc(T* ptr, int n0, Exts&&... exts) {
        static_assert(sizeof...(exts) >= 0);
        static_assert(rank_ == sizeof...(exts) + 1,
            "Rank must be equal to 1 to construct descriptor from pointer and length");

        CFI_index_t extents[rank_] = { 
            static_cast<CFI_index_t>(n0),
            static_cast<CFI_index_t>(exts)... };

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
    cdesc(T (&ref)[N]) : cdesc(ref,N) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from static array");
    }

    // Constructor from std::vector
    cdesc(std::vector<T> &buffer) : cdesc(buffer.data(),buffer.size()) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from std::vector");
    }

    // Constructor from std::array
    template<std::size_t N>
    cdesc(std::array<T,N> &buffer) : cdesc(buffer.data(),N) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from std::array");
    }

#if __cpp_lib_span
    cdesc(std::span<T> buffer) : cdesc(buffer.data(),buffer.size()) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to construct descriptor from std::span.");
    }
#endif


#if __cpp_lib_mdspan
    cdesc(std::mdspan<T,std::dextents<rank_>,std::layout_left> buffer) {
        static_assert(attr_ == Fcpp::attr::other);
        static_assert(rank_ > 1, "Rank must be higher than 1 to convert to std::mdspan");
    }
#endif

    // Return pointer to the underlying descriptor
    // (the name get() is inspired by the C++ smart pointer classes)
    constexpr auto get() const { return get_descptr(); }

    inline std::size_t extent(int dim) const {
        assert(0 <= dim && dim < rank_);
        return this->get()->dim[dim].extent;
    }

    bool is_contiguous() const {
        return CFI_is_contiguous(this->get()) > 0;
    }

    // Implicit cast to C-descriptor pointer
    operator CFI_cdesc_t* () { return this->get(); }

    // FIXME: We need a proper iterator to handle the non-contiguous case
    //        when the stride memory is larger than the element size.

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

    constexpr auto get_descptr() const {
        return (CFI_cdesc_t *) &desc_;
    } 

private:
    // Descriptor containing the actual data
    CFI_CDESC_T(rank_) desc_;
};

template<typename T, int rank_, attr attr_ = Fcpp::attr::other>
class cdesc_ptr {
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

    // Return pointer to the underlying descriptor
    // (the name get() is inspired by the C++ smart pointer classes)
    constexpr auto get() const { return get_descptr(); }

    // Version of ISO_Fortran_binding.h 
    constexpr int version() const { return this->get()->version; }

    // Element length in bytes
    std::size_t elem_len() const { return this->get()->elem_len; }

    inline std::size_t extent(int d) const {
        assert(0 <= d && d < rank_);
        return this->get()->dim[d].extent;
    }

    template<int d>
    inline std::size_t extent() const {
        static_assert(0 <= d && d < rank_);
        return this->get()->dim[d].extent;
    }

    // Constructor
    cdesc_ptr(CFI_cdesc_t *ptr) : ptr_(ptr) {
        // Runtime assertions
        assert(ptr_->type == type());
        assert(ptr_->rank == rank());
        assert(ptr_->attribute == (CFI_attribute_t) attr_);
    }

    // Assumed-size constructor
    //cdesc_ptr(CFI_cdesc_t *ptr, int n) {
    //    assert(ptr_->rank == rank_);
    //    assert(ptr_->extents[rank_-1] == -1);
    //    // TODO: check array is really assumed size
    //}

    bool is_contiguous() const {
        return CFI_is_contiguous(this->get()) > 0;
    }

    constexpr pointer data() const {
        assert(this->is_contiguous());
        return static_cast<pointer>(ptr_->base_addr); 
    }

    // Array subscript operators
    // TODO: multidimensional-access operator
    // TODO: make variadic
    T& operator[](std::size_t idx) {
        static_assert(rank_ == 1,
            "Rank must be 1 to use array subscript operator");
            return *(base_addr() + idx*(stride<0>()/sizeof(T)));
        }
    const T& operator[](std::size_t idx) const {
        static_assert(rank_ == 1,
            "Rank must be 1 to use array subscript operator");
        return *(base_addr() + idx*(stride<0>()/sizeof(T)));
    }

    struct Iterator {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        Iterator(pointer ptr, CFI_index_t sm) : ptr_(ptr), stride(sm/sizeof(T)) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() { return ptr_; }
        Iterator& operator++() { 
            ptr_ = ptr_ + stride; 
            return *this; 
        }
        Iterator operator++(int) { 
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator==(const Iterator& rhs) const {
            return ptr_ == rhs.ptr_;
        }
        bool operator!=(const Iterator& rhs) const {
            return ptr_ != rhs.ptr_;
        }

    private:
        pointer ptr_;
        const size_t stride;
    };

    // Iterator support
    Iterator begin() { 
        static_assert(rank_ == 1, "Rank must be one to use iterator");
        return Iterator(base_addr(), this->stride<0>()); 
    }
    Iterator end() { 
        static_assert(rank_ == 1, "Rank must be one to use iterator");
        std::size_t nbytes = extent<0>()*(stride<0>()/sizeof(T));
        return Iterator(base_addr() + nbytes, this->stride<0>()); 
    }    

#if __cpp_lib_span
    // Implicit cast to std::span (only for rank-1 arrays, 
    // otherwise use .flatten())
    //
    // TODO: compile-time contiguity check
    operator std::span<T>() const {
        static_assert(rank_ == 1,
            "Rank must be equal to 1 to convert implicitly to std::span");
        assert(this->is_contiguous())
        size_type n = this->get()->dim[0].extent;
        return {this->data(),n};
    }
    // Return a flattened view of the array.
    // TODO: handle assumed-size arrays
    // TODO: compile-time contiguity check
    std::span<T> flatten() {
        static_assert(rank_ > 1, "Rank must be higher than 1 to use .flatten()");
        assert(this->is_contiguous())
        size_type nelem = 1;
        for (int i = 0; i < rank_; ++i) {
            nelem *= this->extent(i);
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

private:

    template<int d>
    inline std::size_t stride() const {
        static_assert(0 <= d && d < rank_);
        return ptr_->dim[d].sm;
    }

    using attribute_type = typename std::underlying_type<attr>::type;

    constexpr auto get_descptr() const {
        return ptr_;
    }

    constexpr auto base_addr() const {
        return static_cast<T*>(ptr_->base_addr);
    }

    CFI_cdesc_t *ptr_{nullptr};
};

} // namespace Fcpp