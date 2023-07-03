#include <numeric>

#include "Fcpp.h"

extern "C"
void iota(CFI_cdesc_t *x_, const int *lw) {
    Fcpp::cdesc_ptr<int,1> x(x_);
    std::iota(x.begin(),x.end(), lw ? *lw : (int) 0);    
}