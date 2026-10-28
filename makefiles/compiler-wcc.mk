CC := wcc
LD := wcl

CFLAGS += -0 -bt=dos -os -ms -s -q
CFLAGS += -i=$(SUPPORT_INCLUDE_DIR)
CFLAGS += -i=$(NIO_INCLUDE_DIR)
CFLAGS += -i=diagnostics/msdos/include
CFLAGS += -D__MSDOS__

LDFLAGS += -q -0 -bt=dos -ms

define compile_c
	$(CC) $(CFLAGS) -fo=$@ $<
endef

define link_program
	$(LD) $(LDFLAGS) $(EXTRA_PROGRAM_LDFLAGS) -fe=$@ $^
endef
