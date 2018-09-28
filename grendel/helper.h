#ifndef HELPER_H
#define HELPER_H

#include <deal.II/base/tensor.h>

namespace grendel
{
  /*
   * It's magic
   */
  template <typename T1, std::size_t k1, int k2, typename T2>
  inline DEAL_II_ALWAYS_INLINE void
  scatter(std::array<T1, k1> &U, const dealii::Tensor<1, k2> result, const T2 i)
  {
    for (unsigned int j = 0; j < k1; ++j)
      U[j][i] = result[j];
  }


  /*
   * It's magic
   */
  template <typename T1, std::size_t k, typename T2>
  inline DEAL_II_ALWAYS_INLINE dealii::Tensor<1, k>
  gather(const std::array<T1, k> &U, const T2 i)
  {
    dealii::Tensor<1, k> result;
    for (unsigned int j = 0; j < k; ++j)
      result[j] = U[j][i];
    return result;
  }


  /*
   * It's magic
   */
  template <typename T1, std::size_t k, typename T2, typename T3>
  inline DEAL_II_ALWAYS_INLINE dealii::Tensor<1, k>
  gather(const std::array<T1, k> &U, const T2 i, const T3 l)
  {
    dealii::Tensor<1, k> result;
    for (unsigned int j = 0; j < k; ++j)
      result[j] = U[j](i, l);
    return result;
  }

} // namespace grendel


#endif /* HELPER_H */
