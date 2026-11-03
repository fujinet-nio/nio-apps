TARGETS := msdos atari bbc bbc-clib linux amiga
DEFAULT_TARGET := $(if $(TARGET),$(TARGET),all-targets)
AMIGA_PROFILES := wb13 wb31 wb32
AMIGA_PROFILE ?=

AMIGA_CRT_wb13 := nix13
AMIGA_CRT_wb31 := clib2
AMIGA_CRT_wb32 := clib2

.PHONY: all all-targets clean disk disk-all test-httphead $(TARGETS)

all: $(DEFAULT_TARGET)

test-httphead:
	bash tests/run_httphead_test.sh

msdos atari bbc bbc-clib linux:
	$(MAKE) -f makefiles/build.mk TARGET=$@

# Amiga executables are always profile-qualified.  Supplying AMIGA_PROFILE
# builds one profile; plain `make amiga` produces all supported variants.
amiga:
ifeq ($(AMIGA_PROFILE),)
	@for profile in $(AMIGA_PROFILES); do \
		$(MAKE) -C ../fujinet-nio-driver amiga AMIGA_PROFILE=$$profile || exit $$?; \
		$(MAKE) -f makefiles/build.mk TARGET=amiga \
			TARGET_BUILD_DIR=build/amiga/$$profile \
			AMIGA_CRT=$$(if [ "$$profile" = wb13 ]; then echo nix13; else echo clib2; fi) \
			AMIGA_WB13=$$(if [ "$$profile" = wb13 ]; then echo 1; else echo 0; fi) \
			FUJINET_NIO_DRIVER_BUILD=../fujinet-nio-driver/build/amiga/$$profile || exit $$?; \
	done
else
	$(MAKE) -C ../fujinet-nio-driver amiga AMIGA_PROFILE=$(AMIGA_PROFILE)
	$(MAKE) -f makefiles/build.mk TARGET=amiga \
		TARGET_BUILD_DIR=build/amiga/$(AMIGA_PROFILE) \
		AMIGA_CRT=$(AMIGA_CRT_$(AMIGA_PROFILE)) \
		AMIGA_WB13=$(if $(filter wb13,$(AMIGA_PROFILE)),1,0) \
		FUJINET_NIO_DRIVER_BUILD=../fujinet-nio-driver/build/amiga/$(AMIGA_PROFILE)
endif

all-targets: $(TARGETS)

disk:
	$(MAKE) -f makefiles/build.mk TARGET=$(if $(TARGET),$(TARGET),msdos) disk

disk-all:
	@for target in $(TARGETS); do \
		$(MAKE) -f makefiles/build.mk TARGET=$$target disk; \
	done

clean:
	rm -rf build

.PHONY: atari-stream-disk
atari-stream-disk:
	$(MAKE) -f makefiles/build.mk TARGET=atari streamtest
	mkdir -p build/atari/stream-disk
	cp build/atari/bin/streamtest.xex build/atari/stream-disk/STREAM.XEX
	dir2atr -b PicoBoot406 build/atari/streamtest.atr build/atari/stream-disk
