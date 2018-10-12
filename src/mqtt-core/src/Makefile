include ../config.mk

.PHONY: all clean cleanobjs

all : clean cleanobjs dxlbroker

OBJS += \
	bridge.o \
	conf.o \
	context.o \
	database.o \
	logging.o \
	loop.o \
	mosquitto.o \
	net.o \
	read_handle.o \
	read_handle_client.o \
	read_handle_server.o \
	search_optimization.o \
	send_server.o \
	subs.o \
	../lib/memory_mosq.o \
	../lib/net_mosq.o \
	../lib/read_handle_shared.o \
	../lib/send_client_mosq.o \
	../lib/send_mosq.o \
	../lib/time_mosq.o \
	../lib/tls_mosq.o \
	../lib/util_mosq.o \
	../lib/will_mosq.o \
	dxl/BridgeConfigurationRunner.o \
	dxl/CheckConnectionRunner.o \
	dxl/dxl.o \
	dxl/MqttCoreInterface.o \
	dxl/MqttWorkQueue.o \
	dxl/RestartMqttListenersRunner.o \
	dxl/RevokeCertsRunner.o \
	dxl/SendMessageRunner.o \
	dxl/SetConnectionLimitRunner.o \
	dxl/SetDefaultBridgeKeepaliveRunner.o

dxlbroker : $(OBJS)
	${CXX} $^ -o $@ $(BROKER_LIBS)

%.o : %.c
	${CXX} $(BROKER_CFLAGS) -c $< -o $@

%.o : %.cpp
	${CXX} $(BROKER_CFLAGS) -c $< -o $@

cleanobjs :
	-rm -f $(OBJS)

clean : cleanobjs
	-rm -f dxlbroker
