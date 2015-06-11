#include <algorithm>
#include <cassert>
#include <numeric>
#include <utility>
#include <vector>

#include "topk_simplex_biased_projector.hpp"

namespace sdca {

template <typename RealType>
void TopKSimplexBiasedProjector<RealType>::ComputeThresholds(
    std::vector<RealType> &x,
    RealType &t,
    RealType &lo,
    RealType &hi) const {

  typename std::vector<RealType>::iterator m_begin, l_begin;
  switch (cone_.CheckSpecialCases(x, t, lo, hi)) {
    case Projection::Zero:
      break;
    case Projection::Constant:
      if (cone_.get_k_real() * hi > knapsack_.get_rhs()) {
        knapsack_.PartitionAndComputeThresholds(x, t, lo, hi, m_begin, l_begin);
      }
      break;
    case Projection::General:
      knapsack_.PartitionAndComputeThresholds(x, t, lo, hi, m_begin, l_begin);
      if (CheckProjectOntoCone(x, t, m_begin)) {
        cone_.ComputeGeneralCase(x, t, lo, hi);
      }
      break;
  }

}

template <typename RealType>
bool TopKSimplexBiasedProjector<RealType>::CheckProjectOntoCone(
    std::vector<RealType> &x,
    RealType &t,
    typename std::vector<RealType>::iterator &m_begin) const {

  // Check if the corresponding lambda is negative
  RealType s = knapsack_.get_rhs();
  auto size = std::distance(x.begin(), m_begin);
  if (size) {
    RealType u = static_cast<RealType>(size);
    RealType sum_k_largest = std::accumulate(x.begin(), m_begin,
      static_cast<RealType>(0));
    RealType k = cone_.get_k_real();
    RealType rho_k_k = cone_.get_rho_k_k();
    return k * ( sum_k_largest + (k - u) * t) < s * (u + rho_k_k);
  } else {
    return t < rho_rhs_;
  }
}

template class TopKSimplexBiasedProjector<float>;
template class TopKSimplexBiasedProjector<double>;

}