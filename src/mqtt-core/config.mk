-include ../../version

BROKERCOMMON_DIR=../../common
BROKERLIB_DIR=../../brokerlib

VERSION=1.3.5
TIMESTAMP:=$(shell date "+%F %T%z")

BROKER_LIBS:=$(ADD_LIB)
BROKER_CFLAGS:= \
	-Wall -Wno-missing-field-initializers -ggdb -O2 -Wextra -std=gnu++0x \
	-I. -I.. -I../lib -I./dxl -I${BROKERLIB_DIR} -I${BROKERCOMMON_DIR}/include $(ADD_INCLUDE) \
	-DVERSION="\"${VERSION}\"" -DTIMESTAMP="\"${TIMESTAMP}\"" \
	-DDXL -D__STDC_FORMAT_MACROS -DPACKET_COUNT \
	-DSOMAJVER="\"${SOMAJVER}\"" -DSOMINVER="\"${SOMINVER}\"" -DSOSUBMINVER="\"${SOSUBMINVER}\"" \
	-DSOBLDNUM="\"${SOBLDNUM}\""

UNAME:=$(shell uname -s)

ifeq ($(UNAME),FreeBSD)
	BROKER_LIBS:=$(BROKER_LIBS) -lm
else
	BROKER_LIBS:=$(BROKER_LIBS) -ldl -lm
endif

ifeq ($(UNAME),Linux)
	BROKER_LIBS:=$(BROKER_LIBS) -lrt
endif

BROKER_LIBS:=$(BROKER_LIBS) -Wl,-rpath,'$$ORIGIN/../lib' -L${BROKERLIB_DIR} \
	-lmsgpackc -ldxlbroker -lssl -ljsoncpp -luuid -lpthread -lssl -lcrypto -lwebsockets
