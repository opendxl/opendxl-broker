/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "json/include/JsonService.h"
#include "message/include/DxlMessage.h"
#include "message/include/dx_message.h"
#include <boost/format.hpp>
#include "dxlcommon.h"
#include <cstring>
#include <stdexcept>

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::json;
using namespace dxl::broker::message;

/** {@inheritDoc} */
DxlMessage::DxlMessage( dxl_message_t* msg ) : 
    m_msg( msg ), 
    m_isDirty( false ), 
    m_destBrokerGuids( NULL ), 
    m_destClientGuids( NULL ),
    m_nextBrokerGuids( NULL ),
    m_otherFields( NULL ),
    m_otherFieldsPending( false ),
    m_destTenantGuids( NULL )
{  
}

/** {@inheritDoc} */
DxlMessage::~DxlMessage()
{
    freeMessage( m_msg );    
    if( m_destBrokerGuids )
    {
        delete m_destBrokerGuids;
    }
    if( m_destClientGuids )
    {
        delete m_destClientGuids;
    }
    if( m_nextBrokerGuids )
    {
        delete m_nextBrokerGuids;
    }
    if( m_otherFields )
    {
        delete m_otherFields;
    }
    if( m_destTenantGuids )
    {
        delete m_destTenantGuids;
    }
}

/** {@inheritDoc} */
dxl_message_t* DxlMessage::getMessage() const
{
    return m_msg;
}

/** {@inheritDoc} */
const char* DxlMessage::getMessageId() const
{
    return m_msg->messageId;
}

/** {@inheritDoc} */
const char* DxlMessage::getSourceBrokerGuid() const
{
    return m_msg->sourceBrokerGuid;
}

/** {@inheritDoc} */
void DxlMessage::setSourceBrokerGuid( const char* sourceBrokerGuid )
{
    dxl_message_error_t result =
        setDxlMessageSourceBrokerGuid( NULL, m_msg, sourceBrokerGuid );
    if( result != DXLMP_OK )
    {
        throw runtime_error(
            ( boost::format("Error attempting to set source broker guid: %1%" ) %
                result ).str() );
    }

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
const char* DxlMessage::getSourceClientId() const
{
    return m_msg->sourceClientId;
}

/** {@inheritDoc} */
void DxlMessage::setSourceClientId( const char* sourceClientId )
{
    dxl_message_error_t result =
        setDxlMessageSourceClientId( NULL, m_msg, sourceClientId );
    if( result != DXLMP_OK )
    {
        throw runtime_error(
            ( boost::format( "Error attempting to set source client id: %1%" ) %
                result ).str() );
    }

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
const char* DxlMessage::getSourceClientInstanceId() const
{
    return 
        ( m_msg->sourceClientInstanceId != NULL && strlen(m_msg->sourceClientInstanceId) > 0 ) ?
            m_msg->sourceClientInstanceId : getSourceClientId();
}

/** {@inheritDoc} */
void DxlMessage::setSourceClientInstanceId( const char* sourceClientInstanceId )
{
    dxl_message_error_t result =
        setDxlMessageSourceClientInstanceId( NULL, m_msg, sourceClientInstanceId );
    if( result != DXLMP_OK )
    {
        throw runtime_error(
            ( boost::format( "Error attempting to set source client instance id: %1%" ) %
                result ).str() );
    }

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
void DxlMessage::setPayload( const unsigned char* bytes, size_t size )
{
    dxl_message_error_t result = setDxlMessagePayload( 0, getMessage(), bytes, size );
    if( result != DXLMP_OK )
    {
        throw runtime_error(
            ( boost::format( "Error setting message payload: %1%" ) %
                result ).str() );
    }

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
void DxlMessage::setPayload( const std::string& payload )
{
    setPayload( reinterpret_cast<const unsigned char*>(payload.c_str()), payload.length() );
}

/** {@inheritDoc} */
void DxlMessage::setPayload( const JsonWriter& writer, bool log )
{
    setPayload( JsonService::getInstance().toJson( writer, log ) );
}

/** {@inheritDoc} */
void DxlMessage::getPayload( const unsigned char** bytes, size_t* size ) const
{
    *bytes = m_msg->payload;
    *size = m_msg->payloadSize;
}

/** {@inheritDoc} */
string DxlMessage::getPayloadStr() const
{
    const unsigned char* bytes;
    size_t size;
    getPayload( &bytes, &size );
    return bytes ? string( reinterpret_cast<char const*>( bytes ), size ) : "";
}

/**
 * Sets the destination broker guid
 *
 * @param brokerGuid The destination broker guid 
 */
void DxlMessage::setDestinationBrokerGuid( const char* brokerGuid )
{
    if( m_destBrokerGuids )
    {
        delete m_destBrokerGuids;
        m_destBrokerGuids = NULL;
    }

    if( m_nextBrokerGuids )
    {
        delete m_nextBrokerGuids;
        m_nextBrokerGuids = NULL;
    }

    const char* brokerGuids[] = { brokerGuid };
    setDxlMessageBrokerGuids( NULL, m_msg, brokerGuids, 1 );

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
const unordered_set<string>* DxlMessage::getDestinationBrokerGuids()
{
    if( !m_destBrokerGuids && m_msg->brokerGuidCount > 0 )
    {
        m_destBrokerGuids = new unordered_set<string>();
        for( size_t i = 0; i < m_msg->brokerGuidCount; i++ )
        {
            m_destBrokerGuids->insert( m_msg->brokerGuids[i] );
        }
    }

    return m_destBrokerGuids;
}

/** {@inheritDoc} */
void DxlMessage::setDestinationClientGuid( const char* clientGuid )
{
    if( m_destClientGuids )
    {
        delete m_destClientGuids;
        m_destClientGuids = NULL;
    }

    const char* clientGuids[] = { clientGuid };
    setDxlMessageClientGuids( NULL, m_msg, clientGuids, 1 );    

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
const unordered_set<string>* DxlMessage::getDestinationClientGuids()
{
    if( !m_destClientGuids && m_msg->clientGuidCount > 0 )
    {
        m_destClientGuids = new unordered_set<string>();
        for( size_t i = 0; i < m_msg->clientGuidCount; i++ )
        {
            m_destClientGuids->insert( m_msg->clientGuids[i] );
        }
    }

    return m_destClientGuids;
}

/** {@inheritDoc} */
const unordered_set<string>* DxlMessage::getNextBrokerGuids()
{
    if( m_nextBrokerGuids )
    {
        return m_nextBrokerGuids;
    }

    const unordered_set<string>* destGuids = getDestinationBrokerGuids();    
    if( destGuids )
    {        
        m_nextBrokerGuids = new unordered_set<string>();
        BrokerRegistry& registry = BrokerRegistry::getInstance();
        const char* brokerGuid = BrokerSettings::getGuid();

        for( auto iter = destGuids->begin(); iter != destGuids->end(); ++iter )
        {
            // Determine the next broker. 
            // If this broker is the destination, add it.
            string nextBroker = registry.getNextBroker( brokerGuid, *iter );
            if( !nextBroker.empty() )
            {
                m_nextBrokerGuids->insert( nextBroker );
            }
        }        
    }

    return m_nextBrokerGuids;
}

/** {@inheritDoc} */
void DxlMessage::setOtherField( const string& name, const string& value )
{
    // Make sure the other fields have been loaded
    getOtherFields();

    // Create the map if necessary (no other fields exist)
    if( !m_otherFields )
    {
        m_otherFields = new unordered_map<string,string>();
    }

    // Insert the value
    m_otherFields->insert( pair<string,string>( name, value ) );

    // Set that the other field writes are pending
    m_otherFieldsPending = true;

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
bool DxlMessage::getOtherField( const string& name, string& value )
{
    auto otherFields = getOtherFields();
    if( otherFields )
    {
        auto find = otherFields->find( name );
        if( find != otherFields->end() )
        {
            value = find->second;
            return true;
        }
    }

    return false;
}


/** {@inheritDoc} */
const unordered_map<string, string>* DxlMessage::getOtherFields()
{
    if( !m_otherFields && m_msg->otherFieldsCount > 0 )
    {
        // Ensure it is divisible by 2 (name-value pairs)
        if( ( m_msg->otherFieldsCount % 2 ) == 0 )
        {
            m_otherFields = new unordered_map<string,string>();
            for( size_t i = 0; i < m_msg->otherFieldsCount; i+=2 )
            {
                m_otherFields->insert( 
                    pair<string,string>( 
                        m_msg->otherFields[i], m_msg->otherFields[i+1] ) );
            }
        }
        else 
        {
            // TODO: Log error?
        }
    }

    return m_otherFields;
}

/** {@inheritDoc} */
void DxlMessage::onPreToBytes()
{
    // Output other fields (if applicable)
    if( m_otherFieldsPending )
    {
        m_otherFieldsPending = false;
        if( m_otherFields && m_otherFields->size() > 0 )
        {
            size_t otherFieldsCount = ( m_otherFields->size() << 1 );
            const char** otherFields = (const char**)calloc( otherFieldsCount, sizeof(char*) );
            if( otherFields )
            {
                int pos = 0;
                for( auto iter = m_otherFields->begin(); iter != m_otherFields->end(); iter++ )
                {
                    otherFields[pos++] = iter->first.c_str();
                    otherFields[pos++] = iter->second.c_str();
                }
                setDxlMessageOtherFields( NULL, m_msg, otherFields, otherFieldsCount );            
                free( otherFields );
            }
            else
            {
                throw runtime_error( "Error allocating memory for 'other' fields." );
            }
            
        }
    }
}

/** {@inheritDoc} */
const char* DxlMessage::getSourceTenantGuid() const
{
     return m_msg->sourceTenantGuid;
}

/** {@inheritDoc} */
void DxlMessage::setSourceTenantGuid( const char* sourceTenantGuid )
{
    dxl_message_error_t result = setDxlMessageSourceTenantGuid( NULL, m_msg, sourceTenantGuid );
    if( result != DXLMP_OK )
    {
        throw runtime_error(
            (boost::format( "Error attempting to set source tenant guid: %1%" ) 
                %result).str());
    }

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
const unordered_set<std::string>* DxlMessage::getDestinationTenantGuids()
{
    if( !m_destTenantGuids && m_msg->tenantGuidCount > 0 )
    {
        m_destTenantGuids = new unordered_set<string>();
        for( size_t i = 0; i < m_msg->tenantGuidCount; ++i )
        {
            m_destTenantGuids->insert( m_msg->tenantGuids[i] );
        }
    }

    return m_destTenantGuids;
}

/** {@inheritDoc} */
void DxlMessage::setDestinationTenantGuids( const char* tenantGuids[], size_t tenantGuidCount )
{
    if( m_destTenantGuids )
    {
        delete m_destTenantGuids;
        m_destTenantGuids = NULL;
    }

    setDxlMessageTenantGuids( NULL, m_msg, tenantGuids, tenantGuidCount );

    // Mark dirty
    markDirty();
}
