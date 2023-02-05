//
// SPDX-License-Identifier: MIT
// Copyright (C) 2020 - 2022 by the ryujin authors
//

#pragma once

#include <compile_time_options.h>

#include <hyperbolic_system.h>

#include "convenience_macros.h"
#include "initial_values.h"
#include "offline_data.h"
#include "simd.h"
#include "sparse_matrix_simd.h"

#include <deal.II/base/parameter_acceptor.h>
#include <deal.II/base/timer.h>
#include <deal.II/lac/la_parallel_block_vector.h>
#include <deal.II/lac/sparse_matrix.templates.h>
#include <deal.II/lac/vector.h>

namespace ryujin
{
  /**
   * A postprocessor class for quantities of interest.
   *
   * @ingroup TimeLoop
   */
  template <typename Description, int dim, typename Number = double>
  class Quantities final : public dealii::ParameterAcceptor
  {
  public:
    /**
     * @copydoc HyperbolicSystem
     */
    using HyperbolicSystem = typename Description::HyperbolicSystem;

    /**
     * @copydoc HyperbolicSystem::problem_dimension
     */
    static constexpr unsigned int problem_dimension =
        HyperbolicSystem::template problem_dimension<dim>;

    /**
     * @copydoc HyperbolicSystem::state_type
     */
    using state_type =
        typename HyperbolicSystem::template state_type<dim, Number>;

    /**
     * @copydoc HyperbolicSystem::primitive_state_type
     */
    using primitive_state_type =
        typename HyperbolicSystem::template primitive_state_type<dim, Number>;

    /**
     * Typedef for a MultiComponentVector storing the state U.
     */
    using vector_type = MultiComponentVector<Number, problem_dimension>;

    /**
     * Constructor.
     */
    Quantities(const MPI_Comm &mpi_communicator,
               const ryujin::HyperbolicSystem &hyperbolic_system,
               const ryujin::OfflineData<dim, Number> &offline_data,
               const std::string &subsection = "Quantities");

    /**
     * Prepare evaluation. A call to @ref prepare() allocates temporary
     * storage and is necessary before accumulate() and write_out() can be
     * called.
     *
     * Calling prepare() allocates temporary storage for various boundary
     * and interior maps. The storage requirement varies according to the
     * supplied manifold descriptions.
     *
     * The string parameter @ref name is used as base name for output files.
     */
    void prepare(const std::string &name, unsigned int cycle);

    /**
     * Takes a state vector @p U at time t (obtained at the end of a full
     * Strang step) and accumulates statistics for quantities of interests
     * for all defined manifolds.
     */
    void accumulate(const vector_type &U, const Number t);

    /**
     * Write quantities of interest to designated output files.
     */
    void write_out(const vector_type &U, const Number t, unsigned int cycle);

    //@}

  private:
    /**
     * @name Run time options
     */
    //@{

    std::vector<std::tuple<std::string, std::string, std::string>>
        interior_manifolds_;

    std::vector<std::tuple<std::string, std::string, std::string>>
        boundary_manifolds_;

    bool clear_temporal_statistics_on_writeout_;

    //@}
    /**
     * @name Internal data
     */
    //@{

    const MPI_Comm &mpi_communicator_;

    dealii::SmartPointer<const HyperbolicSystem> hyperbolic_system_;
    dealii::SmartPointer<const OfflineData<dim, Number>> offline_data_;

    /**
     * A tuple describing (local) dof index, boundary normal, normal mass,
     * boundary mass, boundary id, and position of the boundary degree of
     * freedom.
     *
     * @fixme This type only differs from the one used in OfflineData by
     * including a DoF index. It might be better to combine both.
     */
    using boundary_point =
        std::tuple<dealii::types::global_dof_index /*local dof index*/,
                   dealii::Tensor<1, dim, Number> /*normal*/,
                   Number /*normal mass*/,
                   Number /*boundary mass*/,
                   dealii::types::boundary_id /*id*/,
                   dealii::Point<dim>> /*position*/;

    /**
     * The boundary map.
     */
    std::map<std::string, std::vector<boundary_point>> boundary_maps_;

    /**
     * A tuple describing boundary values we are interested in: the
     * primitive state and its second moment, boundary stresses and normal
     * pressure force.
     */
    using boundary_value =
        std::tuple<primitive_state_type /* primitive state */,
                   primitive_state_type /* primitive state second moment */>;

    /**
     * Temporal statistics we store for each boundary manifold.
     */
    using boundary_statistic =
        std::tuple<std::vector<boundary_value> /* values old */,
                   std::vector<boundary_value> /* values new */,
                   std::vector<boundary_value> /* values sum */,
                   Number /* t old */,
                   Number /* t new */,
                   Number /* t sum */>;

    /**
     * Associated statistics for The boundary map.
     */
    std::map<std::string, boundary_statistic> boundary_statistics_;
    std::map<std::string, std::vector<std::tuple<Number, boundary_value>>>
        boundary_time_series_;

    /**
     * A tuple describing (local) dof index, mass, and position of an
     * interior degree of freedom.
     */
    using interior_point =
        std::tuple<dealii::types::global_dof_index /*local dof index*/,
                   Number /*mass*/,
                   dealii::Point<dim>> /*position*/;

    /**
     * The interior map.
     */
    std::map<std::string, std::vector<interior_point>> interior_maps_;

    /**
     * A tuple describing interior values we are interested in: the
     * primitive state and its second moment.
     */
    using interior_value =
        std::tuple<primitive_state_type /* primitive state */,
                   primitive_state_type /* primitive state second moment */>;

    /**
     * Temporal statistics we store for each interior manifold.
     */
    using interior_statistic =
        std::tuple<std::vector<interior_value> /* values old */,
                   std::vector<interior_value> /* values new */,
                   std::vector<interior_value> /* values sum */,
                   Number /* t old */,
                   Number /* t new */,
                   Number /* t sum */>;

    /**
     * Associated statistics for The interior map.
     */
    std::map<std::string, interior_statistic> interior_statistics_;
    std::map<std::string, std::vector<std::tuple<Number, interior_value>>>
        interior_time_series_;

    std::string base_name_;
    unsigned int time_series_cycle_;
    bool first_cycle_;

    //@}
    /**
     * @name Internal methods
     */
    //@{

    void clear_statistics();

    std::string header_;

    template <typename point_type, typename value_type>
    value_type internal_accumulate(const vector_type &U,
                                   const std::vector<point_type> &interior_map,
                                   std::vector<value_type> &new_val);

    template <typename value_type>
    void internal_write_out(std::ostream &output,
                            const std::vector<value_type> &values,
                            const Number scale);

    template <typename value_type>
    void internal_write_out_time_series(
        std::ostream &output,
        const std::vector<std::tuple<Number, value_type>> &values,
        bool append);

    //@}
  };

} /* namespace ryujin */
