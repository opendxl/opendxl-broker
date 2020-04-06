#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
DOCKERFILE_TEMPLATE="$DIR/Dockerfile.template"
DEB_STRETCH_SLIM_DOCKERFILE="$DIR/../Dockerfile"
RH_UBI_DOCKERFILE="$DIR/../redhat-ubi/Dockerfile"

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
# Debian stretch-slim
#
yes | cp -f $DOCKERFILE_TEMPLATE $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error copying template (deb stretch slim).'; }
sed -i "s,@BUILDER_IMAGE@,debian:stretch-slim,g" $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error setting builder image (deb stretch slim).'; }
INSTALL_BUILDER_PACKAGES='apt-get update -y \\\n'\
'    \&\& apt-get install -y libssl1.0-dev libboost-dev cmake uuid-dev wget build-essential'
sed -i "s,@INSTALL_BUILDER_PACKAGES@,$INSTALL_BUILDER_PACKAGES,g" $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error setting builder packages (deb stretch slim).'; }
sed -i "s,@BROKER_IMAGE@,debian:stretch-slim,g" $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error setting broker image (deb stretch slim).'; }
INSTALL_BROKER_PACKAGES='apt-get update -y \\\n'\
'    \&\& apt-get install -y libssl1.0 wget uuid-runtime python iproute2 procps \\\n'\
'    \&\& apt-get clean \\\n'\
'    \&\& rm -rf /var/lib/apt/lists/*'
sed -i "s,@INSTALL_BROKER_PACKAGES@,$INSTALL_BROKER_PACKAGES,g" $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error setting broker packages (deb stretch slim).'; }
INSTALL_PIP='\n# Install Python PIP\n'\
'RUN wget -O get-pip.py '"'https://bootstrap.pypa.io/get-pip.py'"' \\\n'\
'    \&\& python get-pip.py --disable-pip-version-check --no-cache-dir \\\n'\
'    \&\& rm -f get-pip.py \\\n'\
'    \&\& cp -f /usr/local/bin/pip2 /usr/local/bin/pip \\\n'\
'    \&\& pip install dxlconsole==${DXL_CONSOLE_VERSION}\n'
sed -i "s,@INSTALL_PIP@,$INSTALL_PIP,g" $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error setting install pip (deb stretch slim).'; }
sed -i "s,@ADD_USER@,adduser --home /dxlbroker --disabled-password --gecos \"\" dxl,g" $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error setting add user (deb stretch slim).'; }
sed -i "s,@INSTALL_DOC_PACKAGES@,apt-get -y install flex bison python3 doxygen,g" $DEB_STRETCH_SLIM_DOCKERFILE \
    || { fail 'Error setting doc packages (deb stretch slim).'; }

#
# RedHat UBI
#
yes | cp -f $DOCKERFILE_TEMPLATE $RH_UBI_DOCKERFILE \
    || { fail 'Error copying template (RedHat UBI).'; }
sed -i "s,@BUILDER_IMAGE@,centos:8,g" $RH_UBI_DOCKERFILE \
    || { fail 'Error setting builder image (RedHat UBI).'; }
INSTALL_BUILDER_PACKAGES='yum group install -y "Development Tools" \\\n'\
'    \&\& yum install -y openssl-devel boost-devel cmake libuuid-devel wget'
sed -i "s,@INSTALL_BUILDER_PACKAGES@,$INSTALL_BUILDER_PACKAGES,g" $RH_UBI_DOCKERFILE \
    || { fail 'Error setting builder packages (RedHat UBI).'; }
sed -i "s,@BROKER_IMAGE@,registry.redhat.io/ubi8/ubi-minimal:latest,g" $RH_UBI_DOCKERFILE \
    || { fail 'Error setting broker image (RedHat UBI).'; }
INSTALL_BROKER_PACKAGES='microdnf install -y shadow-utils util-linux wget python2-pip openssl procps-ng uuid libuuid iproute \\\n'\
'    \&\& alternatives --set python /usr/bin/python2 \\\n'\
'    \&\& pip2 install dxlconsole==${DXL_CONSOLE_VERSION}'
sed -i "s,@INSTALL_BROKER_PACKAGES@,$INSTALL_BROKER_PACKAGES,g" $RH_UBI_DOCKERFILE \
    || { fail 'Error setting broker packages (RedHat UBI).'; }
INSTALL_PIP=''
sed -i "s,@INSTALL_PIP@,$INSTALL_PIP,g" $RH_UBI_DOCKERFILE \
    || { fail 'Error setting install pip (RedHat UBI).'; }
sed -i "s,@ADD_USER@,useradd -d /dxlbroker -c \"\" dxl,g" $RH_UBI_DOCKERFILE \
    || { fail 'Error setting add user (RedHat UBI).'; }
INSTALL_DOC_PACKAGES='dnf install -y '"'dnf-command(config-manager)'"' \\\n'\
'    \&\& yum config-manager --set-enabled PowerTools \\\n'\
'    \&\& yum -y install flex bison python3 doxygen'
sed -i "s,@INSTALL_DOC_PACKAGES@,$INSTALL_DOC_PACKAGES,g" $RH_UBI_DOCKERFILE \
    || { fail 'Error setting doc packages (RedHat UBI).'; }
