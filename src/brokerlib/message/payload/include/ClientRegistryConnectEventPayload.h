/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CLIENTREGISTRYCONNECTEVENTPAYLOAD_H_
#define CLIENTREGISTRYCONNECTEVENTPAYLOAD_H_

#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"
#include <string>

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "Connect" (or disconnect) message
 */
class ClientRegistryConnectEventPayload : 
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /**
     * Constructor
     *
     * @param   clientId The client associated with the message
     */
    ClientRegistryConnectEventPayload( const std::string& clientId = "" ) : m_clientId( clientId ) {}

    /** Destructor */
    virtual ~ClientRegistryConnectEventPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );    

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

    /**
     * Returns the client identifier
     *
     * @return  The client identifier
     */
    std::string getClientId() const { return m_clientId; }

private:
    std::string m_clientId;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* CLIENTREGISTRYCONNECTEVENTPAYLOAD_H_ */
