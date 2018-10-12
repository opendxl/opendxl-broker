/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERBASE_H_
#define BROKERBASE_H_

#include <string>
#include <cstdint>

namespace dxl {
namespace broker {

/** Default port number */
static const uint32_t DEFAULTPORT = 1883;
/** Default connection limit */
static const uint32_t DEFAULTCONNLIMIT = 0;

/**
 * Base class used to contain information about a broker
 */
class BrokerBase
{
public:
    /**
     * Constructor
     *
     * @param   id The identifier of the broker
     * @param   hostname The host of the broker
     * @param   port The port of the broker
     */
    BrokerBase(
        const std::string& id, const std::string& hostname,
        uint32_t port = DEFAULTPORT);

    /** Destructor */
    virtual ~BrokerBase();

    /**
     * Returns the broker's identifier
     *
     * @return  The broker's identifier
     */
    const std::string& getId() const { return m_id; }

    /**
     * Returns the broker's host name
     *
     * @return  The broker's host name
     */
    const std::string& getHostname() const { return m_hostname; }

    /**
     * Returns the broker's port
     *
     * @return  The broker's port
     */
    uint32_t getPort() const { return m_port; }

    /** Equality */
    virtual bool operator==(const BrokerBase rhs) const;
    /** Inequality */
    virtual bool operator!=(const BrokerBase rhs) const { return *this != rhs; }

protected:
    /**
     * Sets the broker's identifier
     *
     * @param   id The broker's identifier
     */
    void setId( const std::string& id );

    /**
     * Sets the broker's host name
     *
     * @param   hostname The broker's host name
     */
    void setHostname( const std::string& hostname );

    /**
     * Sets the broker's port
     *
     * @param   port The broker's port
     */
    void setPort( uint32_t port );

private:
    /** The broker's identifier */
    std::string m_id;
    /** The broker's host name */
    std::string m_hostname;
    /** The broker's port */
    uint32_t m_port;
};

}  // namespace broker
}  // namespace dxl

#endif  // BROKERBASE_H_
