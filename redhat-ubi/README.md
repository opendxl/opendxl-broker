# OpenDXL Broker (RedHat Universal Base Image 8)

## Overview

A Red Hat Universal Base Image 8 compatible version of the OpenDXL broker.

## Documentation

A valid Red Hat subscription is required to access the UBI 8 image. 

Once validated with a Docker login, the broker can be built with the RedHat UBI 8 base image.

To build the Docker image, run a command similar to the following from the root directory:

`docker build . -f redhat-ubi/Dockerfile`