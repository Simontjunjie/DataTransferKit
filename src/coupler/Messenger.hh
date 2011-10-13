//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   coupler/Messenger.hh
 * \author Stuart R. Slattery
 * \date   Thu May 26 11:02:57 2011
 * \brief  Messenger class definition.
 * \note   Copyright (C) 2011 Oak Ridge National Laboratory, UT-Battelle, LLC.
 */
//---------------------------------------------------------------------------//
// $Id: template.hh,v 1.4 2008/01/02 17:18:47 9te Exp $
//---------------------------------------------------------------------------//

#ifndef coupler_Messenger_hh
#define coupler_Messenger_hh

#include <string>

#include "Base_Map.hh"
#include "Map_Node.hh"
#include "utils/SP.hh"
#include "utils/Packing_Utils.hh"
#include "comm/global.hh"

namespace coupler
{

//===========================================================================//
/*!
 * \class Receive_Buffer
 * \brief Holds a buffer and request handle.
 */
//===========================================================================//
template<class OrdinateType_T>
class Receive_Buffer
{
  public:
    //@{
    //! Useful Typedefs.
    typedef OrdinateType_T      OrdinateType;
    typedef nemesis::Request    Request;
    typedef std::vector<char>   Buffer;
    //@}

  private:
    // Map node index.
    OrdinateType d_source_node;

    // Buffer for storing data.
    Buffer d_buffer;

    // Request handle.
    Request d_request;

  public:
    //! Constructor.
    explicit Receive_Buffer(OrdinateType source, OrdinateType buffer_size)
        : d_source_node(source)
        , d_buffer(buffer_size)
    {   }

    //! Get the source node.
    OrdinateType source() const { return d_source_node; }

    //! Get the requests.
    Request& request() { return d_request; }

    //! Get the buffer.
    Buffer& buffer() { return d_buffer; }

    //! Return whether given request is complete.
    static bool complete(Receive_Buffer<OrdinateType>& buf) 
    { 
        return buf.request().complete(); 
    }
};


//===========================================================================//
/*!
 * \class Messenger
 * \brief A Messenger class to send and receive field data in a map
 *
 * \sa Messenger.cc for detailed descriptions.
 */
/*! 
 * \example coupler/test/tstMessenger.cc
 *
 * Test of Messenger.
 */
//===========================================================================//

template<class DataType_T>
class Messenger 
{
  public:   
    //@{
    //! Useful Typedefs.
    typedef DataType_T                          DataType;
    typedef int                                 OrdinateType;
    typedef Receive_Buffer<OrdinateType>        Receive_Buffer_t;
    typedef typename Receive_Buffer_t::Buffer   Buffer;
    typedef Base_Map<OrdinateType, DataType>    Map_t;
    typedef typename Map_t::KeyType             KeyType;
    typedef typename Map_t::Vec_Ord             Vec_Ord;
    typedef typename Map_t::Vec_Node            Vec_Node;
    typedef denovo::SP<Map_t>                   SP_Map;
    typedef denovo::Packer                      Packer;
    typedef denovo::Unpacker                    Unpacker;
    typedef nemesis::Communicator_t             Communicator_t;
    //@}
    
  private:
    // The communicator the corresponds to the map PID's.
    const Communicator_t &d_communicator;

    // Base_Map object.
    SP_Map d_map;

    // Store the sizes of the request buffers
    Vec_Ord d_buffer_sizes;

 public:
    // Constructor
    Messenger(const Communicator_t &comm_world, SP_Map map);

    // Communicate
    void communicate(const KeyType &key);

  private:
    // Private typedefs
    typedef std::list<Receive_Buffer_t>    BufferList;

    // Create empty buffers for packing and unpacking data
    void calculate_buffer_sizes();

    // Post the receives
    void post_receives(BufferList &buffer_list);

    // Send the data
    void send(const KeyType& key);

    // Fill the map with the received data
    void fill_nodes(BufferList &buffer_list, const KeyType &key);
};

} // end namespace coupler

//---------------------------------------------------------------------------//
// TEMPLATE DEFINITIONS
//---------------------------------------------------------------------------//

#include "Messenger.i.hh"

#endif // coupler_Messenger_hh

//---------------------------------------------------------------------------//
//              end of coupler/Messenger.hh
//---------------------------------------------------------------------------//
