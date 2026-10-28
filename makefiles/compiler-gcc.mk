CC := /usr/bin/gcc

CFLAGS += -Wall -Wextra -O2 -std=c99
CFLAGS += -I$(SUPPORT_INCLUDE_DIR)
CFLAGS += -I$(NIO_INCLUDE_DIR)

LDFLAGS +=

define compile_c
	$(CC) $(CFLAGS) -MMD -MF $(@:.o=.d) -c -o $@ $<
endef

define link_program
	$(CC) $(LDFLAGS) $(EXTRA_PROGRAM_LDFLAGS) -o $@ $^
endef
