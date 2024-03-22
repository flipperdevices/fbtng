HALSRC = $(wildcard $(HALDIR)/Src/*.c)

# Required include directories
HALINC = -I$(HALDIR)/Inc
HALINC += -I$(HALDIR)

DEFINES += -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER

# Shared variables
SRCS += $(HALSRC)
INCLUDES  += $(HALINC)
