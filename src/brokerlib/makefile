-include ../version

###############################################################################
# Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
###############################################################################

BASE_DIR=.

CFLAGS=-std=gnu++0x -O2 -g -Wall -Wextra -c -fmessage-length=0 -fPIC \
	-DDXL -DBROKERLIB_VERSION=\"$(SOMAJVER).$(SOMINVER).$(SOSUBMINVER).$(SOBLDNUM)\"

GLOBAL_INCLUDE=-I$(BASE_DIR)/ -I../common/include $(ADD_INCLUDE)

# All of the sources participating in the build are defined here
-include src/subdir.mk
-include cert/src/subdir.mk
-include message/src/subdir.mk
-include metrics/src/subdir.mk
-include json/src/subdir.mk
-include brokerregistry/src/subdir.mk
-include brokerconfiguration/src/subdir.mk
-include serviceregistry/src/subdir.mk
-include topicauthorization/src/subdir.mk
-include core/src/subdir.mk
-include util/src/subdir.mk

all: libdxlbroker.a

###############################################################################
# Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
###############################################################################

%.o: %.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CFLAGS) $(GLOBAL_INCLUDE) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

libdxlbroker.a: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	ar rcs libdxlbroker.a $(OBJS) $(USER_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '
