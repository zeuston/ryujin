//
// SPDX-License-Identifier: MIT
// Copyright (C) 2020 by the ryujin authors
//

#ifndef OPENMP_H
#define OPENMP_H

#include <compile_time_options.h>

#include <deal.II/base/config.h>

#include <atomic>
#include <omp.h>

/**
 * @name OpenMP parallel for macros
 */
//@{

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_PRAGMA(x) _Pragma(#x)

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_PARALLEL_REGION_BEGIN                                          \
  RYUJIN_PRAGMA(omp parallel default(shared))                                 \
  {

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_PARALLEL_REGION_END }

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_OMP_FOR RYUJIN_PRAGMA(omp for)

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_OMP_FOR_NOWAIT RYUJIN_PRAGMA(omp for nowait)

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_OMP_BARRIER RYUJIN_PRAGMA(omp barrier)

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_LIKELY(x) (__builtin_expect(!!(x), 1))

/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
#define RYUJIN_UNLIKELY(x) (__builtin_expect(!!(x), 0))


/**
 * @todo write documentation
 *
 * @ingroup Miscellaneous
 */
template <typename Payload>
class SynchronizationDispatch
{
public:
  SynchronizationDispatch(const Payload &payload)
      : payload_(payload)
      , executed_payload_(false)
      , n_threads_ready_(0)
  {
  }

  ~SynchronizationDispatch()
  {
    if (!executed_payload_)
      payload_();
  }

  DEAL_II_ALWAYS_INLINE inline void check(bool &thread_ready,
                                          const bool condition)
  {
#ifdef USE_COMMUNICATION_HIDING
    if (RYUJIN_UNLIKELY(thread_ready == false && condition)) {
#else
    (void)thread_ready;
    (void)condition;
    if constexpr (false) {
#endif
      thread_ready = true;
      if (++n_threads_ready_ == omp_get_num_threads()) {
        executed_payload_ = true;
        payload_();
      }
    }
  }

private:
  const Payload payload_;
  bool executed_payload_;
  std::atomic_int n_threads_ready_;
};

//@}

#endif /* OPENMP_H */