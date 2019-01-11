###############################################################################
# Compile Broker
###############################################################################

FROM debian:stretch-slim as builder
ARG build_docs=false

# Packages (OpenSSL, Boost)
RUN apt-get update -y \
    && apt-get install -y libssl1.0-dev libboost-dev cmake uuid-dev wget build-essential

# Message Pack (0.5.8)
RUN cd /tmp \
    && wget https://github.com/msgpack/msgpack-c/releases/download/cpp-0.5.8/msgpack-0.5.8.tar.gz \
    && tar xvfz ./msgpack-0.5.8.tar.gz \
    && cd msgpack-0.5.8 \
    && ./configure \
    && make \
    && make install

# JsonCPP (1.6.0)
RUN cd /tmp \
    && wget https://github.com/open-source-parsers/jsoncpp/archive/1.6.0.tar.gz \
    && tar xvfz 1.6.0.tar.gz \
    && cd jsoncpp-1.6.0 \
    && cmake -DCMAKE_BUILD_TYPE=release -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF \
        -DARCHIVE_INSTALL_DIR=/usr/local/lib -G "Unix Makefiles" \
    && make \
    && make install

# Build broker
COPY src /tmp/src
RUN cd /tmp/src && make

# Generate documentation
COPY docs /tmp/docs
RUN mkdir /tmp/docs-output
RUN if [ "$build_docs" = "true" ]; then apt-get -y install flex bison python3 doxygen \
    && cd /tmp/docs \
    && . /tmp/src/version \
    && sed -i "s,@PROJECT_NUMBER@,$SOMAJVER.$SOMINVER.$SOSUBMINVER.$SOBLDNUM,g" doxygen.config \
    && doxygen doxygen.config > /tmp/docs-output/build.log 2>&1 ; fi

###############################################################################
# Build Broker Image
###############################################################################

FROM debian:stretch-slim

ARG DXL_CONSOLE_VERSION=0.3.0

# Install packages
RUN apt-get update -y \
    && apt-get install -y libssl1.0 wget uuid-runtime python iproute2 procps \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Python PIP
RUN wget -O get-pip.py 'https://bootstrap.pypa.io/get-pip.py' \
	&& python get-pip.py --disable-pip-version-check --no-cache-dir \
    && rm -f get-pip.py \
    && cp -f /usr/local/bin/pip2 /usr/local/bin/pip \
    && pip install dxlconsole==${DXL_CONSOLE_VERSION}

COPY dxlbroker /dxlbroker
COPY LICENSE* /dxlbroker/
COPY --from=builder /tmp/src/mqtt-core/src/dxlbroker /dxlbroker/bin
COPY --from=builder /usr/local/lib/libmsgpackc.so.2.0.0 /dxlbroker/lib

# Documentation
COPY --from=builder /tmp/docs-output /dxlbroker/docs

# Create volume directory
RUN mkdir /dxlbroker-volume

# Add user
RUN adduser --home /dxlbroker --disabled-password --gecos "" dxl \
    && chown -R dxl:dxl /dxlbroker-volume \
    && chown -R dxl:dxl /dxlbroker

# Ensure script is executable
RUN chmod +x /dxlbroker/startup.sh

# Expose the volume
VOLUME ["/dxlbroker-volume"]

# Set user
USER dxl

# Expose ports
EXPOSE 8883
EXPOSE 8443

ENTRYPOINT ["/dxlbroker/startup.sh"]
