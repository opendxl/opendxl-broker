#!/bin/bash

DXLBROKER_DIR=/dxlbroker/bin/
DXLBROKER_APP=/dxlbroker/bin/dxlbroker
DVOL=/dxlbroker-volume
DVOL_CONFIG_DIR=$DVOL/config
DVOL_CONFIG_FILE=$DVOL_CONFIG_DIR/dxlbroker.conf

runuser -g dxl dxl /dxlbroker/startup.sh

# Run the DXL broker
cd $DXLBROKER_DIR || { fail 'Unable to change to DXL broker directory.'; }
$DXLBROKER_APP --config $DVOL_CONFIG_FILE
