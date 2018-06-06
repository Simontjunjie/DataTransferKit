/****************************************************************************
 * Copyright (c) 2012-2018 by the DataTransferKit authors                   *
 * All rights reserved.                                                     *
 *                                                                          *
 * This file is part of the DataTransferKit library. DataTransferKit is     *
 * distributed under a BSD 3-clause license. For the licensing terms see    *
 * the LICENSE file in the top-level directory.                             *
 *                                                                          *
 * SPDX-License-Identifier: BSD-3-Clause                                    *
 ****************************************************************************/

#include <DTK_DetailsDistributedSearchTreeImpl.hpp> // sendAcrossNetwork
#include <DTK_DetailsNearestNeighborOperatorImpl.hpp> // fetch, allReduceBooleanLogicalAnd

#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_UnitTestHarness.hpp>
#include <Tpetra_Distributor.hpp>

template <
    typename View,
    typename std::enable_if<
        std::is_same<
            typename View::traits::memory_space,
            typename View::traits::host_mirror_space::memory_space>::value,
        int>::type = 0>
Teuchos::ArrayView<typename View::const_value_type> toArray( View const &v )
{
    return Teuchos::ArrayView<typename View::const_value_type>( v.data(),
                                                                v.size() );
}

template <
    typename View,
    typename std::enable_if<
        !std::is_same<
            typename View::traits::memory_space,
            typename View::traits::host_mirror_space::memory_space>::value,
        int>::type = 0>
Teuchos::Array<typename View::non_const_value_type> toArray( View const &v )
{
    auto v_h = Kokkos::create_mirror_view( v );
    Kokkos::deep_copy( v_h, v );

    return Teuchos::Array<typename View::non_const_value_type>(
        Teuchos::ArrayView<typename View::const_value_type>( v_h.data(),
                                                             v_h.size() ) );
}

// NOTE Shameless hack to get DeviceType from test driver because I don't want
// to deduce it
template <typename DeviceType>
struct Helper
{
    template <typename View1, typename View2>
    static void
    checkSendAcrossNetwork( Teuchos::RCP<Teuchos::Comm<int> const> const &comm,
                            View1 const &ranks, View2 const &v_exp,
                            View2 const &v_ref, bool &success,
                            Teuchos::FancyOStream &out )
    {
        Tpetra::Distributor distributor( comm );
        distributor.createFromSends( toArray( ranks ) );

        // NOTE here we assume that the reference solution is sized properly
        auto v_imp =
            Kokkos::create_mirror( typename View2::memory_space(), v_ref );

        DataTransferKit::Details::DistributedSearchTreeImpl<
            DeviceType>::sendAcrossNetwork( distributor, v_exp, v_imp );

        // FIXME not sure why I need that guy but I do get a bus error when it
        // is not here...
        Kokkos::fence();

        TEST_COMPARE_ARRAYS( toArray( v_imp ), toArray( v_ref ) );
    }

    template <typename View1, typename View2>
    static void checkFetch( Teuchos::RCP<Teuchos::Comm<int> const> const &comm,
                            View1 const &ranks, View1 const &indices,
                            View2 const &v_exp, View2 const &v_ref,
                            bool &success, Teuchos::FancyOStream &out )
    {
        auto v_imp = DataTransferKit::Details::NearestNeighborOperatorImpl<
            DeviceType>::fetch( comm, ranks, indices, v_exp );

        TEST_COMPARE_ARRAYS( toArray( v_imp ), toArray( v_ref ) );
    }
};

TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( DetailsDistributedSearchTreeImpl,
                                   send_across_network, DeviceType )
{
    using ExecutionSpace = typename DeviceType::execution_space;

    Teuchos::RCP<const Teuchos::Comm<int>> comm =
        Teuchos::DefaultComm<int>::getComm();
    int const comm_rank = comm->getRank();
    int const comm_size = comm->getSize();

    int const DIM = 3;

    // send 1 packet to rank k
    // receive comm_size packets
    Kokkos::View<int **, DeviceType> u_exp( "u_exp", comm_size, DIM );
    Kokkos::parallel_for( Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size ),
                          KOKKOS_LAMBDA( int i ) {
                              for ( int j = 0; j < DIM; ++j )
                                  u_exp( i, j ) = i + j * comm_rank;
                          } );
    Kokkos::fence();

    Kokkos::View<int *, DeviceType> ranks_u( "", comm_size );
    DataTransferKit::iota( ranks_u, 0 );

    Kokkos::View<int **, DeviceType> u_ref( "u_ref", comm_size, DIM );
    Kokkos::parallel_for( Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size ),
                          KOKKOS_LAMBDA( int i ) {
                              for ( int j = 0; j < DIM; ++j )
                                  u_ref( i, j ) = comm_rank + i * j;
                          } );
    Kokkos::fence();

    Helper<DeviceType>::checkSendAcrossNetwork( comm, ranks_u, u_exp, u_ref,
                                                success, out );

    // send k packets to rank k
    // receive k*comm_size packets
    Kokkos::View<int *, DeviceType> tn( "tn", comm_size + 1 );
    Kokkos::parallel_for(
        Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size + 1 ),
        KOKKOS_LAMBDA( int i ) { tn( i ) = ( i * ( i - 1 ) ) / 2; } );
    Kokkos::fence();

    Kokkos::View<int **, DeviceType> v_exp(
        "v_exp", DataTransferKit::lastElement( tn ), DIM );
    Kokkos::parallel_for( Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size ),
                          KOKKOS_LAMBDA( int i ) {
                              for ( int j = tn( i ); j < tn( i + 1 ); ++j )
                                  for ( int k = 0; k < DIM; ++k )
                                      v_exp( j, k ) = i * k;
                          } );
    Kokkos::fence();

    Kokkos::View<int *, DeviceType> ranks_v(
        "", DataTransferKit::lastElement( tn ) );
    Kokkos::parallel_for( Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size ),
                          KOKKOS_LAMBDA( int i ) {
                              for ( int j = tn( i ); j < tn( i + 1 ); ++j )
                                  ranks_v( j ) = i;
                          } );
    Kokkos::fence();

    Kokkos::View<int **, DeviceType> v_ref( "v_ref", comm_size * comm_rank,
                                            DIM );
    Kokkos::parallel_for(
        Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size * comm_rank ),
        KOKKOS_LAMBDA( int i ) {
            for ( int j = 0; j < DIM; ++j )
                v_ref( i, j ) = j * comm_rank;
        } );
    Kokkos::fence();

    Helper<DeviceType>::checkSendAcrossNetwork( comm, ranks_v, v_exp, v_ref,
                                                success, out );
}

TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( DetailsNearestNeighborOperatorImpl, fetch,
                                   DeviceType )
{
    using ExecutionSpace = typename DeviceType::execution_space;

    Teuchos::RCP<const Teuchos::Comm<int>> comm =
        Teuchos::DefaultComm<int>::getComm();
    int const comm_rank = comm->getRank();
    int const comm_size = comm->getSize();

    // make communicaton plan
    Kokkos::View<int *, DeviceType> indices( "indices", comm_size );
    Kokkos::View<int *, DeviceType> ranks( "ranks", comm_size );
    Kokkos::parallel_for( Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size ),
                          KOKKOS_LAMBDA( int i ) {
                              indices( i ) = ( comm_rank + i ) % comm_size;
                              ranks( i ) = comm_size - 1 - comm_rank;
                          } );
    Kokkos::fence();

    // v(i) <-- k*comm_size+i (index i, rank k)
    Kokkos::View<int *, DeviceType> v_exp( "v", comm_size );
    DataTransferKit::iota( v_exp, comm_rank * comm_size );

    Kokkos::View<int *, DeviceType> v_ref( "v_ref", comm_size );
    Kokkos::parallel_for( Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size ),
                          KOKKOS_LAMBDA( int i ) {
                              v_ref( i ) =
                                  ranks( i ) * comm_size + indices( i );
                          } );
    Kokkos::fence();

    Helper<DeviceType>::checkFetch( comm, ranks, indices, v_exp, v_ref, success,
                                    out );

    // w(i, j) <-- k*comm_size*DIM+i+j*comm_size (index i, index j, rank k)
    int const DIM = 2;
    Kokkos::View<int **, DeviceType> w_exp( "w", comm_size, DIM );
    for ( int i = 0; i < DIM; ++i )
        DataTransferKit::iota( Kokkos::subview( w_exp, Kokkos::ALL, i ),
                               i * comm_size + comm_rank * comm_size * DIM );

    Kokkos::View<int **, DeviceType> w_ref( "w_ref", comm_size, DIM );
    Kokkos::parallel_for( Kokkos::RangePolicy<ExecutionSpace>( 0, comm_size ),
                          KOKKOS_LAMBDA( int i ) {
                              for ( int j = 0; j < DIM; ++j )
                                  w_ref( i, j ) = ranks( i ) * comm_size * DIM +
                                                  indices( i ) + j * comm_size;
                          } );
    Kokkos::fence();

    Helper<DeviceType>::checkFetch( comm, ranks, indices, w_exp, w_ref, success,
                                    out );
}

// FIXME templating on DeviceType for this test makes little sense for
// allReduceBooleanLogicalAnd()
// might want to change this to regular TEUCHOS_UNIT_TEST() whenever
// communication helpers headers actually get created
TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( DetailsNearestNeighborOperatorImpl,
                                   allReduceBooleanLogicalAnd, DeviceType )
{
    Teuchos::RCP<const Teuchos::Comm<int>> comm =
        Teuchos::DefaultComm<int>::getComm();
    int const comm_rank = comm->getRank();
    int const comm_size = comm->getSize();

    TEST_ASSERT( DataTransferKit::Details::NearestNeighborOperatorImpl<
                 DeviceType>::allReduceBooleanLogicalAnd( *comm, true ) );

    TEST_ASSERT(
        !DataTransferKit::Details::NearestNeighborOperatorImpl<
            DeviceType>::allReduceBooleanLogicalAnd( *comm, ( comm_rank ==
                                                              comm_size - 1 )
                                                                ? false
                                                                : true ) );
    TEST_ASSERT( !DataTransferKit::Details::NearestNeighborOperatorImpl<
                 DeviceType>::allReduceBooleanLogicalAnd( *comm, false ) );
}

// Include the test macros.
#include "DataTransferKitSearch_ETIHelperMacros.h"

// Create the test group
#define UNIT_TEST_GROUP( NODE )                                                \
    using DeviceType##NODE = typename NODE::device_type;                       \
    TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( DetailsDistributedSearchTreeImpl,    \
                                          send_across_network,                 \
                                          DeviceType##NODE )                   \
    TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( DetailsNearestNeighborOperatorImpl,  \
                                          fetch, DeviceType##NODE )            \
    TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( DetailsNearestNeighborOperatorImpl,  \
                                          allReduceBooleanLogicalAnd,          \
                                          DeviceType##NODE )

// Demangle the types
DTK_ETI_MANGLING_TYPEDEFS()

// Instantiate the tests
DTK_INSTANTIATE_N( UNIT_TEST_GROUP )
