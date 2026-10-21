# Full Super Mario Bros. NumWorks port. Native game engine, no CPU emulator.
CC = arm-none-eabi-gcc
NWLINK ?= npx --yes -- nwlink@1.0.0
PYTHON ?= python3
HOST_CC ?= cc
ARM_LD ?= arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
SRC = src/main.c src/game.c src/render.c src/data.c
CORE = vendor/smbcore/common.c vendor/smbcore/common_sound.c vendor/smbcore/area.c vendor/smbcore/smb1only.c
OBJECTS = $(patsubst %.c,output/%.o,$(SRC) $(CORE))
HEADERS = $(wildcard src/*.h vendor/smbcore/*.h) ../common/epsilon_app.h ../common/epsilon_files.h
PACKER = tools/compact_nwa.py tools/standalone.ld
CFLAGS = -std=c11 $(shell $(NWLINK) eadk-cflags-device) -Os -Wall -Wextra -Werror
CFLAGS += -Wno-unused-parameter -Wno-unused-variable -DSMB1_MODE -DNDEBUG -fwrapv -fsigned-char -fno-strict-aliasing
CFLAGS += -ffunction-sections -fdata-sections -fstack-usage -fno-tree-loop-distribute-patterns -flto -fno-fat-lto-objects -fvisibility=internal
LINK = -nostartfiles --specs=nano.specs -Wl,--relocatable,--gc-sections,-e,main -flinker-output=nolto-rel
.PHONY: build module check test replay fidelity assets check-assets clean
build: output/supermariobros.nwa
output/%.o: %.c $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
output/icon.o: src/icon.png
	mkdir -p output
	$(NWLINK) png-icon-o $< $@
output/icon-compact.o: output/icon.o $(PACKER)
	$(PYTHON) tools/compact_nwa.py icon $< $@ --objcopy $(OBJCOPY)
output/supermariobros-debug.nwa: $(OBJECTS) output/icon-compact.o $(PACKER)
	$(CC) $(CFLAGS) $(LINK) -Wl,-u,eadk_app_name,-u,eadk_app_icon,-u,eadk_api_level $(OBJECTS) output/icon-compact.o -lm -o output/standalone.o
	$(PYTHON) tools/compact_nwa.py app output/standalone.o $@ --ld $(ARM_LD)
output/supermariobros.nwa: output/supermariobros-debug.nwa $(PACKER)
	$(PYTHON) tools/compact_nwa.py release $< $@ --objcopy $(OBJCOPY)
module: $(OBJECTS)
	$(CC) $(CFLAGS) $(LINK) $^ -lm -o output/module.o
check: build
	$(NWLINK) nwa-bin output/supermariobros.nwa output/release.bin --ram-length 107674
	$(NWLINK) nwa-bin output/supermariobros-debug.nwa output/debug.bin --ram-length 107674
	$(PYTHON) -c "from pathlib import Path; assert Path('output/release.bin').read_bytes() == Path('output/debug.bin').read_bytes()"
test:
	mkdir -p output
	$(PYTHON) tests/test_packing.py
	$(HOST_CC) -std=c11 -O2 -DSMB1_MODE -fwrapv -fsigned-char -fno-strict-aliasing tests/test.c src/game.c src/render.c src/data.c $(CORE) -o output/test
	cd output && ./test
	$(HOST_CC) -std=c11 -O2 -DSMB1_MODE -fwrapv -fsigned-char -fno-strict-aliasing tests/rom.c src/game.c src/data.c $(CORE) -o output/rom
	./output/rom assets/tables.bin tests/legacy-save.bin
fidelity:
	mkdir -p output
	$(HOST_CC) -std=c11 -O2 -DSMB1_MODE -fwrapv -fsigned-char -fno-strict-aliasing tests/fidelity.c src/game.c src/render.c src/data.c $(CORE) -o output/fidelity
	cd output && ./fidelity ../tests/reference.rec ../tests/fidelity.expected
replay:
	mkdir -p output
	$(HOST_CC) -std=c11 -O2 -DSMB1_MODE -fwrapv -fsigned-char -fno-strict-aliasing tests/replay.c src/game.c src/data.c $(CORE) -o output/replay
	./output/replay tests/reference.rec
assets:
	$(PYTHON) tools/assets.py
check-assets:
	$(PYTHON) tools/assets.py --check
clean:
	rm -rf output
