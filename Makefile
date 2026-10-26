# SPDX-License-Identifier: GPL-3.0-only
# record-streaming: build with the AZ-MODS SDK container.
#
#   <sdk>/tools/azmod-pack build .
#   make test        host test of the hook decision logic (no deck needed)

MOD_ID          := record-streaming
SRCS            := record-streaming.c
AZ_SDK_INCLUDE  ?= ../include
OUT             ?= out
TEST_CC         ?= cc

ifeq ($(origin CC),default)
CC := gcc
endif
CFLAGS  := -O2 -Wall -Wextra -Werror=implicit-function-declaration \
           -fPIC -fvisibility=hidden -fno-omit-frame-pointer \
           -fexceptions -fasynchronous-unwind-tables \
           -I$(AZ_SDK_INCLUDE)
LDFLAGS := -shared -static-libgcc -Wl,--no-undefined -Wl,--hash-style=gnu
NO_OUTLINE_ATOMICS := $(shell printf 'int main(void){return 0;}\n' | \
    $(CC) -mno-outline-atomics -x c -o /dev/null - 2>/dev/null && echo -mno-outline-atomics)
CFLAGS += $(NO_OUTLINE_ATOMICS)

.PHONY: all test clean
all: $(OUT)/$(MOD_ID).so

$(OUT)/$(MOD_ID).so: $(SRCS) $(AZ_SDK_INCLUDE)/azmod-mod.h | $(OUT)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS)

$(OUT):
	mkdir -p $(OUT)

test: | $(OUT)
	$(TEST_CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(AZ_SDK_INCLUDE) \
	    test/test-logic.c -o $(OUT)/test-logic
	$(OUT)/test-logic

clean:
	rm -rf $(OUT)
