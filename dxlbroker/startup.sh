#!/bin/bash

DXLBROKER_DIR=/dxlbroker
DXLBROKER_APP=/dxlbroker/sbin/dxlbroker
DXLBROKER_CONFIG_DIR=$DXLBROKER_DIR/config
DXLBROKER_CONFIG_FILE=$DXLBROKER_CONFIG_DIR/dxlbroker.conf.tmpl
DXLBROKER_LIB_DIR=$DXLBROKER_DIR/lib
DXLBROKER_CONSOLE_DIR=$DXLBROKER_DIR/console
DXLBROKER_CONSOLE_CONFIG_DIR=$DXLBROKER_CONSOLE_DIR/config
DXLBROKER_CONSOLE_CONFIG_FILE=$DXLBROKER_CONSOLE_CONFIG_DIR/dxlconsole.config
DXLBROKER_CONSOLE_LOGGING_FILE=$DXLBROKER_CONSOLE_CONFIG_DIR/logging.config
DXLBROKER_CONSOLE_CLIENT_CONFIG_FILE=$DXLBROKER_CONSOLE_CONFIG_DIR/dxlclient.config
DXLBROKER_CONSOLE_CLIENT_CONFIG_TMPL_FILE=$DXLBROKER_CONSOLE_CONFIG_DIR/dxlclient.config.tmpl

MSGPACK_LIB=$DXLBROKER_LIB_DIR/libmsgpackc.so.2.0.0
MSGPACK_LIB_SYMLINK1=$DXLBROKER_LIB_DIR/libmsgpackc.so.2
MSGPACK_LIB_SYMLINK2=$DXLBROKER_LIB_DIR/libmsgpackc.so

DVOL=/dxlbroker-volume
DVOL_CONFIG_DIR=$DVOL/config
DVOL_CONFIG_FILE=$DVOL_CONFIG_DIR/dxlbroker.conf
DVOL_CONSOLE_CONFIG_DIR=$DVOL/config/console
DVOL_CONSOLE_CONFIG_FILE=$DVOL_CONSOLE_CONFIG_DIR/dxlconsole.config
DVOL_CONSOLE_LOGGING_FILE=$DVOL_CONSOLE_CONFIG_DIR/logging.config
DVOL_CONSOLE_CLIENT_CONFIG_FILE=$DVOL_CONSOLE_CONFIG_DIR/dxlclient.config
DVOL_CONSOLE_CLIENT_CONFIG_TMPL_FILE=$DVOL_CONSOLE_CONFIG_DIR/dxlclient.config.tmpl
DVOL_POLICY_DIR=$DVOL/policy
DVOL_KEYSTORE_DIR=$DVOL/keystore
DVOL_LOGS_DIR=$DVOL/logs
DVOL_CLIENT_CA_CERT_FILE=$DVOL_KEYSTORE_DIR/ca-client.crt
DVOL_CLIENT_CA_KEY_FILE=$DVOL_KEYSTORE_DIR/ca-client.key
DVOL_BROKER_CA_CERT_FILE=$DVOL_KEYSTORE_DIR/ca-broker.crt
DVOL_BROKER_CA_KEY_FILE=$DVOL_KEYSTORE_DIR/ca-broker.key
DVOL_BROKER_CA_CSR_FILE=$DVOL_KEYSTORE_DIR/ca-broker.csr
DVOL_BROKER_CERT_FILE=$DVOL_KEYSTORE_DIR/broker.crt
DVOL_BROKER_KEY_FILE=$DVOL_KEYSTORE_DIR/broker.key
DVOL_BROKER_CSR_FILE=$DVOL_KEYSTORE_DIR/broker.csr
DVOL_BROKER_V3_EXT_FILE=$DVOL_KEYSTORE_DIR/v3.ext
REQUIRED_CA_FILES=($DVOL_CLIENT_CA_CERT_FILE $DVOL_BROKER_CA_CERT_FILE $DVOL_BROKER_CERT_FILE $DVOL_BROKER_KEY_FILE)

CERT_PASS=OpenDxlBroker
CERT_DAYS=3650

#
# Function that is invoked when the script fails.
#
# $1 - The message to display prior to exiting.
#
function fail() {
    echo $1
    echo "Exiting."
    exit 1
}

#
# Link libraries
#

if [ ! -f $MSGPACK_LIB_SYMLINK1 ]; then
    ln -s $MSGPACK_LIB $MSGPACK_LIB_SYMLINK1 || { fail 'Error creating message pack symbolic link (1).'; }
fi
if [ ! -f $MSGPACK_LIB_SYMLINK2 ]; then
    ln -s $MSGPACK_LIB $MSGPACK_LIB_SYMLINK2 || { fail 'Error creating message pack symbolic link (2).'; }
fi

#
# Create directories
#

if [ ! -d $DVOL_CONFIG_DIR ]; then
    echo "Creating config directory..."
    mkdir -p $DVOL_CONFIG_DIR || { fail 'Error creating config directory.'; }
fi
if [ ! -d $DVOL_CONSOLE_CONFIG_DIR ]; then
    echo "Creating console config directory..."
    mkdir -p $DVOL_CONSOLE_CONFIG_DIR || { fail 'Error creating console config directory.'; }
fi
if [ ! -d $DVOL_POLICY_DIR ]; then
    echo "Creating policy directory..."
    mkdir -p $DVOL_POLICY_DIR || { fail 'Error creating policy directory.'; }
fi
if [ ! -d DVOL_KEYSTORE_DIR ]; then
    echo "Creating keystore directory..."
    mkdir -p $DVOL_KEYSTORE_DIR || { fail 'Error creating keystore directory.'; }
fi
if [ ! -d DVOL_LOGS_DIR ]; then
    echo "Creating logs directory..."
    mkdir -p $DVOL_LOGS_DIR || { fail 'Error creating logs directory.'; }
fi

#
# Console configuration files
#
if [ ! -f $DVOL_CONSOLE_CONFIG_FILE ]; then
    cp $DXLBROKER_CONSOLE_CONFIG_FILE $DVOL_CONSOLE_CONFIG_FILE \
        || { fail 'Error copying console configuration file.'; }
    sed -i "s,@CLIENT_CA_CERT_FILE@,$DVOL_CLIENT_CA_CERT_FILE,g" $DVOL_CONSOLE_CONFIG_FILE \
        || { fail 'Error setting CA certificate file in console configuration file.'; }
    sed -i "s,@CLIENT_CA_KEY_FILE@,$DVOL_CLIENT_CA_KEY_FILE,g" $DVOL_CONSOLE_CONFIG_FILE \
        || { fail 'Error setting CA key file in console configuration file.'; }
    sed -i "s,@BROKER_CA_BUNDLE_FILE@,$DVOL_BROKER_CA_CERT_FILE,g" $DVOL_CONSOLE_CONFIG_FILE \
        || { fail 'Error setting broker CA bundle file in console configuration file.'; }
    sed -i "s,@CLIENT_CA_PASSWORD@,$CERT_PASS,g" $DVOL_CONSOLE_CONFIG_FILE \
        || { fail 'Error setting client CA password in console configuration file.'; }
    sed -i "s,@CLIENT_CONFIG_TMPL_FILE@,$DVOL_CONSOLE_CLIENT_CONFIG_TMPL_FILE,g" $DVOL_CONSOLE_CONFIG_FILE \
        || { fail 'Error setting client configuration template file in console configuration file.'; }
fi
if [ ! -f $DVOL_CONSOLE_CLIENT_CONFIG_FILE ]; then
    cp $DXLBROKER_CONSOLE_CLIENT_CONFIG_FILE $DVOL_CONSOLE_CLIENT_CONFIG_FILE \
        || { fail 'Error copying console client configuration file.'; }
fi
if [ ! -f DVOL_CONSOLE_LOGGING_FILE ]; then
    cp $DXLBROKER_CONSOLE_LOGGING_FILE $DVOL_CONSOLE_LOGGING_FILE \
        || { fail 'Error copying console logging configuration file.'; }
fi
if [ ! -f $DVOL_CONSOLE_CLIENT_CONFIG_TMPL_FILE ]; then
    cp $DXLBROKER_CONSOLE_CLIENT_CONFIG_TMPL_FILE $DVOL_CONSOLE_CLIENT_CONFIG_TMPL_FILE \
        || { fail 'Error copying console client configuration template file.'; }
fi

#
# Create broker configuration file
#

if [ ! -f $DVOL_CONFIG_FILE ]; then
    echo "No broker configuration file found, creating one..."
    cp $DXLBROKER_CONFIG_FILE $DVOL_CONFIG_FILE || { fail 'Copy failed.'; }

    echo "  Setting broker identifier..."
    BROKERID=$(uuidgen) || { fail 'Broker identifier generation failed.'; }
    sed -i "s/@DXLBROKER_ID@/$BROKERID/g" $DVOL_CONFIG_FILE \
        || { fail 'Error setting broker identifier in config file.'; }

    echo "  Updating configuration paths..."
    sed -i "s,@DXLBROKER_POLICYDIR@,$DVOL_POLICY_DIR,g" $DVOL_CONFIG_FILE \
        || { fail 'Error setting policy directory in config file.'; }
    sed -i "s,@DXLBROKER_KEYSTOREDIR@,$DVOL_KEYSTORE_DIR,g" $DVOL_CONFIG_FILE \
        || { fail 'Error setting keystore directory in config file.'; }
    sed -i "s,@DXLBROKER_LOGDIR@,$DVOL_LOGS_DIR,g" $DVOL_CONFIG_FILE \
        || { fail 'Error setting logs directory in config file.'; }
fi

#
# Read broker identifier from configuration file
#

BROKER_ID=$(awk -F"brokerId *= *" '{printf $2}' $DVOL_CONFIG_FILE)
if [ -z $BROKER_ID ]; then
    fail 'Unable to find broker identifier in configuration file.'
fi

#
# Check and possibly generate certificate information
#

# Check to see if any of the required CA files exist
found_ca_file=false
for f in "${REQUIRED_CA_FILES[@]}"
do
    if [ -f $f ]; then
        found_ca_file=true
        break
	fi
done

if [ $found_ca_file = true ]
then
    # At least one file exists, make sure they all exist
    found_all_files=true
    for f in "${REQUIRED_CA_FILES[@]}"
    do
        if [ ! -f $f ]; then
            found_all_files=false
            echo "Required CA file not found: $f"
        fi
    done
    if [ $found_all_files = false ]; then
        fail 'Required CA files were not found.'
    fi
else
    # No CA files exist, generate them.
    echo "Generating certificate files..."

    # Create Client CA
    openssl req -new -passout pass:"$CERT_PASS" -subj "/CN=OpenDxlClientCA-$BROKER_ID" -x509 -days $CERT_DAYS \
        -extensions v3_ca -keyout $DVOL_CLIENT_CA_KEY_FILE -out $DVOL_CLIENT_CA_CERT_FILE \
        || { fail 'Error creating client CA.'; }

    # Generate Broker CA CSR
    openssl req -out $DVOL_BROKER_CA_CSR_FILE -subj "/CN=OpenDxlBrokerCA-$BROKER_ID" -new -newkey rsa:2048 -nodes \
        -keyout $DVOL_BROKER_CA_KEY_FILE \
        || { fail 'Error generating broker CA certificate signing request.'; }

    # Create V3 extension file (CA is true)
    echo "basicConstraints=CA:TRUE" > $DVOL_BROKER_V3_EXT_FILE \
        || { fail 'Error creating broker CA V3 extension file.'; }

    # Sign Broker CA CSR
    openssl x509 -req -passin pass:"$CERT_PASS" -in $DVOL_BROKER_CA_CSR_FILE -CA $DVOL_CLIENT_CA_CERT_FILE \
        -CAkey $DVOL_CLIENT_CA_KEY_FILE -CAcreateserial -out $DVOL_BROKER_CA_CERT_FILE -days $CERT_DAYS \
         -extfile $DVOL_BROKER_V3_EXT_FILE \
        || { fail 'Error signing broker CA.'; }

    # Append Client CA to Broker CA
    cat $DVOL_CLIENT_CA_CERT_FILE >> $DVOL_BROKER_CA_CERT_FILE \
        || { fail 'Unable to append Client CA to Broker CA.'; }

    # Create broker CSR
    openssl req -out $DVOL_BROKER_CSR_FILE -subj "/CN=OpenDxlBroker-$BROKER_ID" -new -newkey rsa:2048 -nodes -keyout \
        $DVOL_BROKER_KEY_FILE \
        || { fail 'Error generating broker CSR.'; }

    # Create V3 extension file (CA is false)
    echo "basicConstraints=CA:FALSE" > $DVOL_BROKER_V3_EXT_FILE \
        || { fail 'Error creating broker V3 extension file.'; }

    # Sign the Broker CSR
    openssl x509 -req -passin pass:"$CERT_PASS" -in $DVOL_BROKER_CSR_FILE -CA $DVOL_BROKER_CA_CERT_FILE \
        -CAkey $DVOL_BROKER_CA_KEY_FILE -CAcreateserial -out $DVOL_BROKER_CERT_FILE -days $CERT_DAYS \
        -extfile $DVOL_BROKER_V3_EXT_FILE \
        || { fail 'Error signing broker CSR.'; }

    # Remove temporary files
    rm -f $DVOL_KEYSTORE_DIR/*.csr
    rm -f $DVOL_KEYSTORE_DIR/*.srl
    rm -f $DVOL_BROKER_V3_EXT_FILE
fi

# Run the broker console
cd $DXLBROKER_CONSOLE_DIR || { fail 'Unable to change to broker console directory.'; }
python -m dxlconsole $DVOL_CONSOLE_CONFIG_DIR &
cd $DXLBROKER_DIR || { fail 'Unable to change to DXL broker directory.'; }

# Run the DXL broker
$DXLBROKER_APP --config $DVOL_CONFIG_FILE
