#ifndef SDCA_PROX_KNAPSACK_LE_BIASED_H
#define SDCA_PROX_KNAPSACK_LE_BIASED_H

#include <functional>

#include "knapsack_eq.h"

namespace sdca {

template <typename Iterator,
          typename Result>
thresholds<Iterator, Result>
thresholds_knapsack_le_biased_search(
    Iterator first,
    Iterator last,
    const Result lo,
    const Result hi,
    const Result rhs,
    const Result rho
    ) {
  // At this point, rho must be positive
  assert(rho > 0);

  // Sort data to search efficiently
  typedef typename std::iterator_traits<Iterator>::value_type Data;
  std::sort(first, last, std::greater<Data>());

  // Precompute some constants
  Result rho_rhs = rho * rhs;
  Result rho_inverse = static_cast<Result>(1) / rho;
  Result num_X = static_cast<Result>(std::distance(first, last));
  Result num_U = 0, min_U = +std::numeric_limits<Result>::infinity();

  // Grow U starting with empty
  for (auto m_first = first;;) {

    Result min_M = +std::numeric_limits<Result>::infinity();
    Result max_M = -std::numeric_limits<Result>::infinity();
    Result num_M = 0, sum_M = 0, sum_M_comp = 0;
    Result num_L = num_X - num_U;

    // Grow M starting with empty
    for (auto m_last = m_first;;) {
      // Compute t as follows:
      //   t = (lo * num_L + hi * num_U + sum_M) / (1/rho + num_M)
      // and check that
      //  (1)  lo + t  >= max_L = (m_last) or (-Inf)
      //  (2)  lo + t  <= min_M = (m_last - 1) or (+Inf)
      //  (3)  hi + t  >= max_M = (m_first) or (-Inf)
      //  (4)  hi + t  <= min_U = (m_first - 1) or (+Inf)
      //  (5)       t  <= rho * rhs

      Result t = (lo * num_L + hi * num_U + sum_M) / (rho_inverse + num_M);
      if (t <= rho_rhs) {
        Result tt = hi + t;
        if (max_M <= tt && tt <= min_U) {
          tt = lo + t;
          if (tt <= min_M &&
              ((m_last == last) || static_cast<Result>(*m_last) <= tt)) {
            return make_thresholds(t, lo, hi, m_first, m_last);
          }
        }
      }

      // Increment the set M
      if (m_last == last) {
        break;
      }
      min_M = static_cast<Result>(*m_last);
      max_M = static_cast<Result>(*m_first);
      kahan_add(min_M, sum_M, sum_M_comp); // sum_M += min_M;
      --num_L;
      ++num_M;
      ++m_last;
    }

    // Increment the set U
    if (m_first == last) {
      break;
    }
    min_U = static_cast<Result>(*m_first);
    ++num_U;
    ++m_first;
  }

  // Should never reach here
  assert(false);
  return thresholds<Iterator, Result>(0, lo, hi, first, first);
}

template <typename Iterator,
          typename Result = double,
          typename Summation = std_sum<Iterator, Result>>
thresholds<Iterator, Result>
thresholds_knapsack_le_biased(
    Iterator first,
    Iterator last,
    const Result lo = 0,
    const Result hi = 1,
    const Result rhs = 1,
    const Result rho = 1,
    Summation sum = Summation()
    ) {
  // First, check if the inequality constraint is active (sum > rhs)
  auto m_first = std::partition(first, last,
    [=](const Result x){ return x >= hi; });
  auto m_last = std::partition(m_first, last,
    [=](const Result x){ return x > lo; });

  Result s = sum(m_first, m_last, static_cast<Result>(0))
    + hi * static_cast<Result>(std::distance(first, m_first))
    + lo * static_cast<Result>(std::distance(m_last, last));

  // Special cases: 1) equality constraint; 2) t = 0
  if (s > rhs) {
    auto t = thresholds_knapsack_eq(first, last, lo, hi, rhs, sum);
    if (t.t >= rho * rhs) {
      return t;
    }
  } else if (rho * s == static_cast<Result>(0)) {
    return thresholds<Iterator, Result>(0, lo, hi, m_first, m_last);
  }

  // General case
  return thresholds_knapsack_le_biased_search(first, last, lo, hi, rhs, rho);
}

template <typename Iterator,
          typename Result = double,
          typename Summation = std_sum<Iterator, Result>>
inline
void
project_knapsack_le_biased(
    Iterator first,
    Iterator last,
    const Result lo = 0,
    const Result hi = 1,
    const Result rhs = 1,
    const Result rho = 1,
    Summation sum = Summation()
    ) {
  project(first, last,
    thresholds_knapsack_le_biased<Iterator, Result, Summation>,
    lo, hi, rhs, rho, sum);
}

template <typename Iterator,
          typename Result = double,
          typename Summation = std_sum<Iterator, Result>>
inline
void
project_knapsack_le_biased(
    Iterator first,
    Iterator last,
    Iterator aux_first,
    Iterator aux_last,
    const Result lo = 0,
    const Result hi = 1,
    const Result rhs = 1,
    const Result rho = 1,
    Summation sum = Summation()
    ) {
  project(first, last, aux_first, aux_last,
    thresholds_knapsack_le_biased<Iterator, Result, Summation>,
    lo, hi, rhs, rho, sum);
}

template <typename Iterator,
          typename Result = double,
          typename Summation = std_sum<Iterator, Result>>
inline
void
project_knapsack_le_biased(
    const typename std::iterator_traits<Iterator>::difference_type dim,
    Iterator first,
    Iterator last,
    Iterator aux_first,
    Iterator aux_last,
    const Result lo = 0,
    const Result hi = 1,
    const Result rhs = 1,
    const Result rho = 1,
    Summation sum = Summation()
    ) {
  project(dim, first, last, aux_first, aux_last,
    thresholds_knapsack_le_biased<Iterator, Result, Summation>,
    lo, hi, rhs, rho, sum);
}

}

#endif
