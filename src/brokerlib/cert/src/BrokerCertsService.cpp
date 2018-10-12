/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/brokerlib.h"
#include "include/BrokerSettings.h"
#include "include/GeneralPolicySettings.h"
#include "include/SimpleLog.h"
#include "cert/include/BrokerCertsService.h"
#include "util/include/FileUtil.h"
#include "util/include/TimeUtil.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/objects.h>

#include "dxlcommon.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::cert;
using namespace dxl::broker::util;


/** {@inheritDoc} */
BrokerCertsService& BrokerCertsService::getInstance()
{
    static BrokerCertsService instance;
    return instance;
}

/* {@inheritDoc} */
BrokerCertsService::BrokerCertsService()
{
    m_clientGuidNid = OBJ_create(
        "1.3.6.1.4.1.1230.540.1", "DxlClientGuidOID", "DXL Client GUID OID" );
    m_tenantGuidNid = OBJ_create(
        "1.3.6.1.4.1.1230.540.2", "DxlTenantOID", "DXL Tenant GUID OID" );
}


/** {@inheritDoc} */
bool BrokerCertsService::getFilesExist()
{
    return checkFilesExist();
}


/**
 * Looks up and returns the specified asn1 nid certificate extension value
 *
 * @return    The specified asn1 nid certificate extension value 
 *            (or empty string if it could not be determined)
 */
static string lookupCertExtension( int asn1nid )
{
    string retVal;
    BIO *certbio = NULL;
    X509 *cert = NULL;
    int ret;

    // Create the Input/Output BIO's.                             
    certbio = BIO_new( BIO_s_file() );

    // Read the certificate
    ret = BIO_read_filename( certbio, BrokerSettings::getBrokerCertFile().c_str() );
    if ( ret != 1 || !( cert = PEM_read_bio_X509(certbio, NULL, 0, NULL ) ) ) 
    {
        SL_START << "Error loading cert into memory" << SL_ERROR_END;
    }
    else
    {
        // Find the Tenant GUID extension
        X509_CINF *cert_inf = cert->cert_info;
        STACK_OF(X509_EXTENSION) *ext_list = NULL;
        if( cert_inf )
        {
            ext_list = cert_inf->extensions;
        }

        if( ext_list )
        {
            for( int i = 0; i < sk_X509_EXTENSION_num( ext_list ); i++ )
            {
                ASN1_OBJECT *obj;
                X509_EXTENSION *ext;
                ext = sk_X509_EXTENSION_value( ext_list, i );
                if( !ext ) continue;
                obj = X509_EXTENSION_get_object( ext );
                if( !obj ) continue;
                int nid = OBJ_obj2nid( obj );
                if( nid != 0 && nid == asn1nid )
                {
                    ASN1_OCTET_STRING* octet_str = X509_EXTENSION_get_data( ext );
                    if( octet_str )
                    {
                        const unsigned char* octet_str_data = octet_str->data;
                        if( octet_str_data )
                        {
                            long xlen;
                            int tag, xclass;
                            /*int ret =*/ ASN1_get_object( &octet_str_data, &xlen, &tag, &xclass, octet_str->length );
                            retVal = (char*)octet_str_data;
                        }
                    }
                }
            }
        }
    }

    X509_free( cert );
    BIO_free_all( certbio );

    return retVal;
}

/** {@inheritDoc} */
string BrokerCertsService::determineBrokerTenantGuid() const
{
    if( !checkFilesExist() )
    {
        SL_START << "Unable to determine broker Tenant GUID, cert files don't exist" << SL_ERROR_END;
        return "";
    }

    return lookupCertExtension( m_tenantGuidNid );
}

/** {@inheritDoc} */
string BrokerCertsService::determineBrokerGuid() const
{
    if( !checkFilesExist() )
    {
        SL_START << "Unable to determine broker GUID, cert files don't exist" << SL_ERROR_END;
        return "";
    }

    return lookupCertExtension( m_clientGuidNid );
}

/** {@inheritDoc} */
bool BrokerCertsService::checkFilesExist() const
{
    return (
        FileUtil::fileExists( BrokerSettings::getBrokerPrivateKeyFile() ) && 
        FileUtil::fileExists( BrokerSettings::getBrokerCertFile() ) && 
        FileUtil::fileExists( BrokerSettings::getBrokerCertChainFile() ) && 
        FileUtil::fileExists( BrokerSettings::getClientCertChainFile() )
    );
}
