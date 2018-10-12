DIRS=brokerlib mqtt-core/src

.PHONY : all dxlbroker clean

all : dxlbroker

dxlbroker :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d}; done

clean :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} clean; done

