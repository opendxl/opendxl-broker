/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <cstring>
#include <stdexcept>
#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "core/include/CoreMessageContext.h"
#include "dxlcommon.h"

using namespace std;
using namespace dxl::broker::message;
using namespace dxl::broker::core;

/** {@inheritDoc} */
CoreMessageContext::CoreMessageContext(
    const char* sourceId, const char* canonicalSourceId, bool isSourceBridge, 
    uint8_t contextFlags, const char* topic,
    uint32_t payloadLen, const void* payload ) :
    m_sourceId( sourceId ), 
    m_canonicalSourceId( canonicalSourceId ),
    m_isSourceBridge( isSourceBridge ), 
    m_topic( topic ),
    m_originalPayloadLen( payloadLen ), m_originalPayload( payload ),
    m_dxlMessageParsed( false ),
    m_messageInsertEnabled( true ),
    m_serviceNotFoundEnabled( true ),
    m_destCount( 0 ), m_contextFlags( contextFlags ),
    m_clientSpecificMessageGenerated( false )
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "CoreMessageContext::CoreMessageContext()" << SL_DEBUG_END;
}

/** {@inheritDoc} */
CoreMessageContext::~CoreMessageContext()
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "CoreMessageContext::~CoreMessageContext()" << SL_DEBUG_END;
}

/** {@inheritDoc} */
bool CoreMessageContext::isLocalBrokerSource() const
{
    return !strcmp( getSourceId(), BrokerSettings::getGuid() );
}

/** {@inheritDoc} */
bool CoreMessageContext::isDxlBrokerServiceRequest() const
{
    return strncmp( 
        DXL_BROKER_REQUEST_PREFIX, 
        getTopic(), 
        DXL_BROKER_REQUEST_PREFIX_LEN ) == 0;
}

/** {@inheritDoc} */
void CoreMessageContext::parseDxlMessage()
{
    if( !m_dxlMessageParsed )
    {
        m_dxlMessageParsed = true;

        try
        {        
            m_dxlMessage =
                DxlMessageService::getInstance().fromBytes(
                    static_cast<const unsigned char*>(m_originalPayload), m_originalPayloadLen );
        }
        catch( const exception& ex )
        {
            if( SL_LOG.isDebugEnabled() )
                SL_START << "Non DXL-message: " << m_topic << ", reason=" << ex.what() << SL_DEBUG_END;
        }
        catch( ... )
        {
            if( SL_LOG.isDebugEnabled() )
                SL_START << "Non DXL-message: " << m_topic << SL_DEBUG_END;
        }
    }
}

/** {@inheritDoc} */
bool CoreMessageContext::isDxlMessage()
{
    parseDxlMessage();
    return m_dxlMessage.get() != NULL;
}

/** {@inheritDoc} */
DxlMessage* CoreMessageContext::getDxlMessage()
{
    parseDxlMessage();
    DxlMessage* message = m_dxlMessage.get();
    if( message == NULL )
    {
        throw runtime_error( "Message payload is not a DXL message");
    }
    return message;
}

/** {@inheritDoc} */
DxlRequest* CoreMessageContext::getDxlRequest()
{
    DxlRequest* request = dynamic_cast<DxlRequest*>( getDxlMessage() );
    if( request == NULL )
    {
        throw runtime_error( "Message payload is not a DXL request");
    }
    return request;
}

/** {@inheritDoc} */
DxlEvent* CoreMessageContext::getDxlEvent()
{
    DxlEvent* evt = dynamic_cast<DxlEvent*>( getDxlMessage() );
    if( evt == NULL )
    {
        throw runtime_error( "Message payload is not a DXL event");
    }
    return evt;
}

/** {@inheritDoc} */
DxlErrorResponse* CoreMessageContext::getDxlErrorResponse()
{
    DxlErrorResponse* errResponse = dynamic_cast<DxlErrorResponse*>( getDxlMessage() );
    if( errResponse == NULL )
    {
        throw runtime_error( "Message payload is not a DXL error response");
    }
    return errResponse;

}
