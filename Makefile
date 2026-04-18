CC       = cc
CFLAGS   = -Wall -Wextra -std=c99 -D_GNU_SOURCE -O2
LDFLAGS  =

# Library discovery via pkg-config (warn but do not fail if missing)
PKG_LIBS = libimobiledevice-1.0 libirecovery-1.0 libusb-1.0 \
           libplist-2.0 openssl libcurl

CFLAGS  += $(shell pkg-config --cflags $(PKG_LIBS) 2>/dev/null)
LDFLAGS += $(shell pkg-config --libs   $(PKG_LIBS) 2>/dev/null)

$(foreach lib,$(PKG_LIBS),$(if $(shell pkg-config --exists $(lib) 2>/dev/null && echo ok),,$(warning Library $(lib) not found by pkg-config)))

# Auto-discover all C sources under src/
SRCS     = $(shell find src -name '*.c')
OBJS     = $(SRCS:.c=.o)
TARGET   = tr4mpass

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude -c -o $@ $<

clean:
	find src -name '*.o' -delete
	rm -f $(TARGET) $(TEST_TARGET)

# ------------------------------------------------------------------ #
# Test target: compile only hardware-independent units                #
# ------------------------------------------------------------------ #

TEST_UNITS  = src/device/chip_db.c src/bypass/bypass.c src/util/log.c
TEST_TARGET = tests/run_tests
TEST_CFLAGS = $(CFLAGS) -Iinclude -DUNIT_TEST

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/test_sections.c $(TEST_UNITS)
	$(CC) $(TEST_CFLAGS) -o $@ $^

.PHONY: all clean test
