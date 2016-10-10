//---------------------------------------------------------------------------//
/*
  Copyright (c) 2012, Stuart R. Slattery
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  *: Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  *: Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  *: Neither the name of the University of Wisconsin - Madison nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//---------------------------------------------------------------------------//
/*!
 * \brief DTK_MoabHelpers.hpp
 * \author Stuart R. Slattery
 * \brief Moab helper functions.
 */
//---------------------------------------------------------------------------//

#ifndef DTK_MOABHELPERS_HPP
#define DTK_MOABHELPERS_HPP

#include <vector>

#include "DTK_Types.hpp"
#include "DTK_Entity.hpp"

#include <Teuchos_Ptr.hpp>
#include <Teuchos_Array.hpp>

#include <moab/ParallelComm.hpp>

namespace DataTransferKit
{
//---------------------------------------------------------------------------//
/*!
  \class MoabHelpers
  \brief Moab helper functions.
*/
//---------------------------------------------------------------------------//
class MoabHelpers
{
  public:

    /*!
     * \brief Extract the Moab entity from a DTK entity.
     */
    static moab::EntityHandle extractEntity( const Entity dtk_entity );

    /*!
     * \brief Get the global id of a list of entities.
     */
    static void getGlobalIds( const moab::ParallelComm& mesh,
                              const moab::EntityHandle* entities,
                              const int num_entities,
                              EntityId* entity_ids );

    /*!
     * \brief Given a Moab EntityType, get the topological dimension.
     */
    static int
    getTopologicalDimensionFromMoabType( const moab::EntityType moab_type );

    /*!
     * \brief Given a Moab EntityType, get the name.
     */
    static std::string
    getNameFromMoabType( const moab::EntityType moab_type );

    /*!
     * \brief Get the coordinates of the entity nodes in canonical order.
     */
    static void getEntityNodeCoordinates(
        const moab::EntityHandle& moab_entity,
        const Teuchos::Ptr<moab::ParallelComm>& moab_mesh,
        Teuchos::Array<double>& coordinates );
};

//---------------------------------------------------------------------------//

} // end namespace DataTransferKit

//---------------------------------------------------------------------------//

#endif // end DTK_MOABHELPERS_HPP

//---------------------------------------------------------------------------//
// end DTK_MoabHelpers.hpp
//---------------------------------------------------------------------------//
