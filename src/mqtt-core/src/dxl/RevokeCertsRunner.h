/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef REVOKECERTSRUNNER_H_
#define REVOKECERTSRUNNER_H_

#include "MqttWorkQueue.h"
#include "cert_hashes.h"

namespace dxl {
namespace broker {
namespace core {

/**
 * Runner that revokes the specified set of certificates
 */
class RevokeCertsRunner : public MqttWorkQueue::Runnable
{    
public:

    /**
     * Constructs the runner
     *
     * @param   revokedCerts The certificates that have been revoked
     */
    RevokeCertsRunner( struct cert_hashes* revokedCerts ) : 
        m_revokedCerts( revokedCerts ) {};

    /** Destructor */
    virtual ~RevokeCertsRunner() {};
    
    /** {@inheritDoc} */
    void run();

protected:
    /** The revoked certificates */
    struct cert_hashes* m_revokedCerts;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* REVOKECERTSRUNNER_H_ */
