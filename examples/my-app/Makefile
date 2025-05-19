# examples/my-app/Makefile
CONTIKI_PROJECT = child-node
all: $(CONTIKI_PROJECT)

CONTIKI = ../..

# Include the Simple UDP module
MODULES += os/net/app-layer/simple-udp

# Include logging
MODULES += os/sys/log

# Include random number generator
MODULES += os/lib/random

include $(CONTIKI)/Makefile.include