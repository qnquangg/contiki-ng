# CONTIKI_PROJECT = test-light-and-led
# CONTIKI_PROJECT = test-border-router-tunslip-mqtt-client
# CONTIKI_PROJECT = test-multicast-nrf52840-sender-root test-multicast-nrf52840-receiver-sink
CONTIKI_PROJECT = test-nrf52840-led-button

all: $(CONTIKI_PROJECT)

CONTIKI = ../..

PLATFORMS_ONLY = nrf52840

# ------------------------------------
# Test Light Sensor and LED on Nrf52840
# ------------------------------------

# ifeq ($(CONTIKI_NG_DRIVERS_SENSOR_DIR),)
#   CONTIKI_NG_DRIVERS_SENSOR_DIR = $(CONTIKI)/arch/dev/sensor
# endif
# MODULES += $(CONTIKI_NG_DRIVERS_SENSOR_DIR)/bh1750

# MODULES_REL += $(TARGET)

# ------------------------------------
# MQTT Border Router Example
# ------------------------------------

# include $(CONTIKI)/Makefile.dir-variables
# MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt

# include $(CONTIKI)/Makefile.identify-target

# # MODULES_REL += arch/platform/$(TARGET)

# # Add the Border Router module
# MODULES += os/services/rpl-border-router

# Define the C file for our project
# CONTIKI_SOURCEFILES += mqtt-border-router.c

# ------------------------------------
# Test Multicast Nrf52840
# ------------------------------------

include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_NET_DIR)/ipv6/multicast

# MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

# ------------------------------------

include $(CONTIKI)/Makefile.include