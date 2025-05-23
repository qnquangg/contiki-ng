CONTIKI_PROJECT = test-bh1750

all: $(CONTIKI_PROJECT)

CONTIKI = ../../..

# TARGET = nrf52840
# BOARD = nrf52840/dk
# MODULES_REL += $(TARGET)
PLATFORMS_ONLY = nrf52840

# PROJECT_SOURCEFILES += $(CONTIKI)/arch/cpu/nrf52840/dev/bh1750-arch.c

# include $(CONTIKI)/Makefile.dir-variables

ifeq ($(CONTIKI_NG_DRIVERS_SENSOR_DIR),)
  CONTIKI_NG_DRIVERS_SENSOR_DIR = $(CONTIKI)/arch/dev/sensor
endif
MODULES += $(CONTIKI_NG_DRIVERS_SENSOR_DIR)/bh1750

# CFLAGS += -I$(CONTIKI)/arch/dev/sensor/bh1750

include $(CONTIKI)/Makefile.include
