//
// SPDX-License-Identifier: MIT
// Copyright (C) 2020 by the ryujin authors
//

#ifndef MULTICOMPONENT_VECTOR_H
#define MULTICOMPONENT_VECTOR_H

#include "simd.h"

#include <deal.II/base/partitioner.h>
#include <deal.II/lac/la_parallel_vector.h>

namespace ryujin
{
  /**
   * @todo write documentation
   *
   * @ingroup SIMD
   */
  template <int n_comp>
  std::shared_ptr<const dealii::Utilities::MPI::Partitioner>
  create_vector_partitioner(
      const std::shared_ptr<const dealii::Utilities::MPI::Partitioner>
          &scalar_partitioner)
  {
    dealii::IndexSet vector_owned_set(n_comp * scalar_partitioner->size());
    for (auto it = scalar_partitioner->locally_owned_range().begin_intervals();
         it != scalar_partitioner->locally_owned_range().end_intervals();
         ++it)
      vector_owned_set.add_range(*it->begin() * n_comp,
                                 (it->last() + 1) * n_comp);
    vector_owned_set.compress();
    dealii::IndexSet vector_ghost_set(n_comp * scalar_partitioner->size());
    for (auto it = scalar_partitioner->ghost_indices().begin_intervals();
         it != scalar_partitioner->ghost_indices().end_intervals();
         ++it)
      vector_ghost_set.add_range(*it->begin() * n_comp,
                                 (it->last() + 1) * n_comp);
    vector_ghost_set.compress();
    const auto vector_partitioner =
        std::make_shared<const dealii::Utilities::MPI::Partitioner>(
            vector_owned_set,
            vector_ghost_set,
            scalar_partitioner->get_mpi_communicator());

    return vector_partitioner;
  }


  /**
   * @todo write documentation
   *
   * @ingroup SIMD
   */
  template <typename Number, int n_comp>
  class MultiComponentVector
      : public dealii::LinearAlgebra::distributed::Vector<Number>
  {
  public:
    using scalar_type =  dealii::LinearAlgebra::distributed::Vector<Number>;

    using dealii::LinearAlgebra::distributed::Vector<Number>::reinit;

    /**
     * @todo write documentation
     */
    void reinit_with_scalar_partitioner(
        const std::shared_ptr<const dealii::Utilities::MPI::Partitioner>
            &scalar_partitioner);

    /**
     * @todo write documentation
     *
     * ... updates ghost values
     */
    void export_component(scalar_type &scalar_vector,
                          unsigned int component) const;

    /**
     * @todo write documentation
     *
     * ... ghost values have to be updated by hand
     */
    void import_component(const scalar_type &scalar_vector,
                          unsigned int component);
  };


  template <typename Number, int n_comp>
  void MultiComponentVector<Number, n_comp>::reinit_with_scalar_partitioner(
      const std::shared_ptr<const dealii::Utilities::MPI::Partitioner>
          &scalar_partitioner)
  {
    auto vector_partitioner =
        create_vector_partitioner<n_comp>(scalar_partitioner);

    dealii::LinearAlgebra::distributed::Vector<Number>::reinit(
        vector_partitioner);
  }


  template <typename Number, int n_comp>
  void MultiComponentVector<Number, n_comp>::export_component(
      scalar_type &scalar_vector, unsigned int component) const
  {
    const auto local_size = scalar_vector.get_partitioner()->local_size();
    for (unsigned int i = 0; i < local_size; ++i)
      scalar_vector.local_element(i) =
          this->local_element(i * n_comp + component);
    scalar_vector.update_ghost_values();
  }


  template <typename Number, int n_comp>
  void MultiComponentVector<Number, n_comp>::import_component(
      const scalar_type &scalar_vector, unsigned int component)
  {
    const auto local_size = scalar_vector.get_partitioner()->local_size();
    for (unsigned int i = 0; i < local_size; ++i)
      this->local_element(i * n_comp + component) =
          scalar_vector.local_element(i);
  }


} // namespace ryujin


#endif /* MULTICOMPONENT_VECTOR_H */
