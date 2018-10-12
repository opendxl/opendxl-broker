include config.mk

DIRS=src

.PHONY : all mqtt-core clean cleanobjs install

all : mqtt-core

mqtt-core :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d}; done

clean :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} clean; done

cleanobjs :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} cleanobjs; done

install : mqtt-core
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} install; done
