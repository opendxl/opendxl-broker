FROM centos:7

VOLUME ["/dxlbroker-volume"]

# Install OS packages
RUN yum update -y \
    && yum install -y util-linux openssl \
    && yum clean all

# Install Python PIP and required packages
RUN curl "https://bootstrap.pypa.io/get-pip.py" -o "get-pip.py" \
    && python get-pip.py \
    && pip install tornado dxlbootstrap dxlclient

COPY dxlbroker /dxlbroker

EXPOSE 8883
EXPOSE 8443

ENTRYPOINT ["/dxlbroker/startup.sh"]
