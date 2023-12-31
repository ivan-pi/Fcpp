
#include <algorithm>
#include <numeric>

#include <gtest/gtest.h>

#include "Fcpp.h"
using namespace Fcpp;

TEST(cdesc_class, fromPointer) {

  float *a = new float[3];
  cdesc fa(a,3);

  EXPECT_EQ(fa.rank(),1);
  EXPECT_EQ(fa.extent(0),3);
  EXPECT_EQ(fa.elem_len(),sizeof(float));
  EXPECT_EQ(fa.get()->base_addr,a);
  EXPECT_TRUE(fa.is_contiguous());

  delete [] a;
}


TEST(cdesc_class, fromVector) {

  std::vector<float> a(5);
  cdesc<float> fa(a);

  EXPECT_EQ(fa.rank(),1);
  EXPECT_EQ(fa.extent(0),5);
  EXPECT_EQ(fa.elem_len(),sizeof(float));
  EXPECT_EQ(fa.get()->base_addr,a.data());
  EXPECT_TRUE(fa.is_contiguous());

}

TEST(cdesc_class, fromArray) {

  std::array<float,7> a;
  cdesc<float> fa(a);

  EXPECT_EQ(fa.rank(),1);
  EXPECT_EQ(fa.extent(0),7);
  EXPECT_EQ(fa.elem_len(),sizeof(float));
  EXPECT_EQ(fa.get()->base_addr,a.data());
  EXPECT_TRUE(fa.is_contiguous());

}

TEST(cdesc_class, fromStaticArray) {

  float a[9];
  cdesc<float> fa(a);

  EXPECT_EQ(fa.rank(),1);
  EXPECT_EQ(fa.extent(0),9);
  EXPECT_EQ(fa.elem_len(),sizeof(float));
  EXPECT_EQ(fa.get()->base_addr,&a);
  EXPECT_TRUE(fa.is_contiguous());

}

#if __cpp_lib_span
TEST(cdesc_class, fromSpan) {

  double *a = new double[6];
  std::span sa{a,6};

  cdesc<double> fa(sa);

  EXPECT_EQ(fa.rank(),1);
  EXPECT_EQ(fa.extent(0),6);
  EXPECT_EQ(fa.elem_len(),sizeof(double));
  EXPECT_EQ(fa.get()->base_addr,a);
  EXPECT_TRUE(fa.is_contiguous());

  delete [] a;
}
#endif

TEST(cdesc_class, subscript1D) {

  std::array<int,7> a = {0,1,2,3,4,5,6};
  cdesc<int> fa_(a);

  cdesc_ptr<int,1> fa(fa_.get()); 

  for (int i = 0; i < fa.extent(0); ++i) {
    EXPECT_EQ(i, fa[i]);
    fa[i] += 1;
  }

  for (int i = 0; i < a.size(); ++i) {
    EXPECT_EQ(i+1,a[i]);
  }

}

TEST(cdesc_class, rangeBasedFor) {

  std::array<int,7> a = {0,1,2,3,4,5,6};
  cdesc<int> fa(a);

  int i = 0;
  for (auto item : fa) {
    EXPECT_EQ(i, item);
    ++item;
    EXPECT_GT(item,a[i]);
    EXPECT_EQ(i,a[i]);
    ++i;
  }

  i = 0;
  for (auto& item : fa) {
    EXPECT_EQ(i, item);
    ++item;
    EXPECT_EQ(item,a[i]);
    EXPECT_LT(i,a[i]);
    ++i;
  }

}

TEST(cdesc_class, iteratorsSortInPlace) {

  std::vector<double> a;
  a.emplace_back(3.0);
  a.emplace_back(2.0);
  a.emplace_back(1.0);

  cdesc fp(a);

  std::sort(fp.begin(),fp.end());

  EXPECT_EQ(1.0,a[0]);
  EXPECT_EQ(2.0,a[1]);
  EXPECT_EQ(3.0,a[2]);

}

TEST(cdesc_class, iteratorsIndirectSort) {

  // Sort the indexes in p based upon the values in a

  std::vector<double> a;
  a.emplace_back(3.0);
  a.emplace_back(2.0);
  a.emplace_back(1.0);

  std::vector<int> p = {0,1,2};
  cdesc fp(p);
  
  auto compare = [&a](const int& il, const int& ir) {
    return a[il] < a[ir];
  };

  std::sort(fp.begin(),fp.end(),compare);

  EXPECT_EQ(2,p[0]);  
  EXPECT_EQ(1,p[1]);  
  EXPECT_EQ(0,p[2]);  

}

TEST(cdesc_class, minElementUsingConstantIterators) {

  std::vector<double> v = {3.,2.,1.,4.};
  cdesc fv(v);
  
  auto min_el = std::min_element(fv.cbegin(),fv.cend()); 
  EXPECT_EQ(1.0,*min_el);
  EXPECT_EQ(min_el,&fv[2]);  

}

// Returns 1 if all elements of b are equal to 2, or 0 otherwise.
extern "C" int alltwo(CFI_cdesc_t *b);

TEST(cdesc_class, implicitCastCdesc) {

  std::vector<int> b(10,2);
  cdesc fb(b);

  // Implicit cast on assignment
  CFI_cdesc_t *b_ = fb;
  EXPECT_EQ(b_,fb.get());

  // Implicit cast when passing to the procedure alltwo
  EXPECT_TRUE( alltwo(fb) > 0 );

}

#if __cpp_lib_span
TEST(cdesc_class, implicitCastSpan) {

  std::vector<int> b(10);
  cdesc fb(b);

  // Implicit cast in copy constructor of span
  std::span sb(fb);

  // Consistency of Fortran descriptor and span
  EXPECT_EQ(fb.extent(0),sb.size());
  EXPECT_EQ(fb.get()->base_addr,sb.data());

  // Consistency of original vector with span produced indirectly
  EXPECT_EQ(b.size(),sb.size());
  EXPECT_EQ(b.data(),sb.data());
  
}
#endif