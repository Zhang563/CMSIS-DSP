// -*- C++ -*-
/** @file */ 
#pragma once 

#include <dsppp/arch.hpp>
#include <type_traits>
#include <dsppp/number.hpp>

#ifdef DOXYGEN
#define ARM_MATH_MVEI
#define ARM_MATH_MVEF
#endif

/** \addtogroup ARCHALG
 *  \addtogroup HELIUMALG Helium specific algorithm
 *  \ingroup ARCHALG
 *  @{
 */

#if defined(ARM_MATH_MVEI) || defined(ARM_MATH_MVEF)
template<typename T,typename DST,
typename std::enable_if<has_vector_inst<DST>() &&
          IsVector<DST>::value &&
         SameElementType<DST,T>::value,bool>::type = true>
inline void _Fill(DST &v,
                  const T val, 
                  const vector_length_t l,
                  const Helium* = nullptr)
{
      constexpr int nb_lanes = vector_traits<T>::nb_lanes;
      index_t i=0;
      UNROLL_LOOP
      for(i=0;i < l; i += nb_lanes) 
      {
        v.vector_store_tail(i,l-i,inner::vconst_tail(val,inner::vctpq<T>::mk(l-i)));
      }
}

template<typename T,typename DST,
typename std::enable_if<has_vector_inst<DST>() &&
         must_use_matrix_idx<DST>() &&
         SameElementType<DST,T>::value,bool>::type = true>
inline void _Fill2D(DST &v,
                  const T val, 
                  const vector_length_t rows,
                  const vector_length_t cols,
                  const Helium* = nullptr)
{
      constexpr int nb_lanes = vector_traits<T>::nb_lanes;

      // Outer unroll factor in case inner loop does not have
      // enough arithmetic instructions.
      // In future version this may be estimated from the
      // complexity of the AST to evaluate
      constexpr int U = 1;
      index_t row=0;

      UNROLL_LOOP
      for(; row <= rows-U;row += U)
      {

          UNROLL_LOOP
          for(index_t col=0; col < cols;col += nb_lanes)
          {
              for(int k=0;k<U;k++)
              {
                  v.matrix_store_tail(row+k,col,cols-col,inner::vconst_tail(val,inner::vctpq<T>::mk(cols-col)));
              }
          }
      }

      for(; row < rows;row ++)
      {

          UNROLL_LOOP
          for(index_t col=0; col < cols;col += nb_lanes)
          {
              v.matrix_store_tail(row,col,cols-col,inner::vconst_tail(val,inner::vctpq<T>::mk(cols-col)));
          }
      }
}

template<typename DA,typename DB,
typename std::enable_if<has_vector_inst<DA>() && 
                        vector_idx_pair<DA,DB>(),bool>::type = true>
inline void eval(DA &v,
                 const DB& other,
                 const vector_length_t l,
                 const Helium* = nullptr)
{
      using T = typename traits<DA>::Scalar;
      constexpr int nb_lanes = vector_traits<T>::nb_lanes;
      
      index_t i=0;

      UNROLL_LOOP    
      for(i=0;i < l; i += nb_lanes) 
      {
          v.vector_store_tail(i,l-i,other.vector_op_tail(i,l-i));
      }
}


template<typename DA,typename DB,
typename std::enable_if<has_vector_inst<DA>() &&
                        must_use_matrix_idx_pair<DA,DB>(),bool>::type = true>
inline void eval2D(DA &v,
                   const DB& other,
                   const vector_length_t rows,
                   const vector_length_t cols,
                   const Helium* = nullptr)
{
      using T = typename traits<DA>::Scalar;
      constexpr int nb_lanes = vector_traits<T>::nb_lanes;
      // Attempt at computing the unrolling factor
      // depending on the complexity of the AST
      // (will have to rework this estimation)
      constexpr int RU = 5 - Complexity<DB>::value;
      constexpr int U = (RU <= 0) || (RU>=5) ? 1 : RU;
      index_t row=0;

      UNROLL_LOOP
      for(; row <= rows-U;row += U)
      {

          UNROLL_LOOP
          for(index_t col=0; col < cols;col += nb_lanes)
          {
              for(int k=0;k<U;k++)
              {
                  v.matrix_store_tail(row+k,col,cols-col,other.matrix_op_tail(row+k,col,cols-col));
              }
          }
      }

      UNROLL_LOOP
      for(; row < rows;row ++)
      {

          UNROLL_LOOP
          for(index_t col=0; col < cols;col += nb_lanes)
          {
              v.matrix_store_tail(row,col,cols-col,other.matrix_op_tail(row,col,cols-col));
          }
      }
}


static std::ostream& operator<< (std::ostream& stream, const float32x4_t& other) 
{
   stream << "(" << other[0] << "," <<other[1] << "," <<other[2] << "," <<other[3] << ")";
   return(stream);
}

template<class TupType, size_t... I>
void printt(const TupType& _tup, std::index_sequence<I...>)
{
    std::cout << "(";
    (..., (std::cout << (I == 0? "" : ", ") << std::get<I>(_tup)));
    std::cout << ")\n";
}

template<class... T>
void printt (const std::tuple<T...>& _tup)
{
    printt(_tup, std::make_index_sequence<sizeof...(T)>());
}

template<typename DA,typename DB,
         typename std::enable_if<has_vector_inst<DA>() &&
         vector_idx_pair<DA,DB>(),bool>::type = true>
inline DotResult<DA> _dot(const DA& a,
                          const DB& b,
                          const vector_length_t l,
                          const Helium* = nullptr)
{
   //using Res = DotResult<DA>;
   // Vector scalar datatype

   using T = typename traits<DA>::Scalar;
   using Temp = typename vector_traits<T>::temp_accumulator;

   constexpr int nb_lanes = vector_traits<T>::nb_lanes;

   Temp acc = vector_traits<T>::temp_acc_zero();

    UNROLL_LOOP
    for(index_t i=0; i<l; i+=nb_lanes)
    {
        acc = inner::vmacc(acc,a.vector_op_tail(i,l-i),b.vector_op_tail(i,l-i),inner::vctpq<T>::mk(l-i));
    }

     return(inner::vreduce(acc));
}

template<typename DA,typename DB,
         typename std::enable_if<has_vector_inst<DA>() &&
                                 vector_idx_pair<DA,DB>(),bool>::type = true>
inline void _swap(DA&& a,
                  DB&& b,
                  const vector_length_t l,
                  const Helium* = nullptr)
{
      using Scalar = typename ElementType<DA>::type;
      using Vector = typename vector_traits<Scalar>::vector;

      constexpr int nb_lanes = vector_traits<typename ElementType<DA>::type>::nb_lanes;
      index_t i=0;
      Vector tmpa,tmpb;

      UNROLL_LOOP     
      for(i=0;i < l; i += nb_lanes) 
      {
        tmpa = a.vector_op_tail(i,l-i);
        tmpb = b.vector_op_tail(i,l-i);
        b.vector_store_tail(i,l-i,tmpa);
        a.vector_store_tail(i,l-i,tmpb);
      }
}
#endif

/*! @} */

