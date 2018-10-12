/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ServiceRegistryQueryResponseServicePayload.h"
#include "topicauthorization/include/topicauthorizationservice.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void ServiceRegistryQueryResponseServicePayload::write( Json::Value& out ) const
{
    // Super
    ServiceRegistryRegisterEventPayload::write( out, true );

    // Whether the service is local to the broker.
    out[ DxlMessageConstants::PROP_LOCAL ] = m_serviceRegistration.isLocal();

    // Get the authorization service
    const shared_ptr<TopicAuthorizationService> authService = 
        TopicAuthorizationService::Instance();

    // Determine unauthorized channels for service
    Value unauthChannels( arrayValue );
    unordered_set<string> requestChannels = getRequestChannels();
    for( auto it = requestChannels.begin(); it != requestChannels.end(); ++it )
    {
        if( !( m_serviceRegistration.isManagedClient() ?            
                authService->isAuthorizedToSubscribe( 
                    m_serviceRegistration.getClientGuid(), *it ) :
                authService->isAuthorizedToSubscribe( 
                    m_serviceRegistration.getCertificatesUtHash(), *it ) ) )
        {
            unauthChannels.append( *it );
        }
    }
    out[ DxlMessageConstants::PROP_UNAUTHORIZED_CHANNELS ] = unauthChannels;     
}
