/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERHELPERS_H_
#define BROKERHELPERS_H_
#include <string>

/** The broker configuration file name */
#define CONFIG_LOC "dxlbroker.conf"
/** The broker general policy name */
#define POLICYGENERALSTATE_LOC "general.policy"
/** The broker state policy name */
#define POLICYBROKERSTATE_LOC "brokerstate.policy"
/** The topic authorization policy name */
#define TOPICAUTHSTATE_LOC "topicauth.policy"

namespace dxl {
namespace broker {

/**
 * Structure containing common string constants used throughout broker library.
 */
struct BrokerHelpers {
    /** The product version */
    static const char* PROPERTY_VALUE_PRODUCT_VERSION;  // ("1.0.0");
};

} /* namespace broker */
} /* namespace dxl */

#endif  // BROKERHELPERS_H_
