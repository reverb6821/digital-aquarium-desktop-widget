# Fallback build without CMake:  make -C sim   (needs SDL2-devel for the GUI target)
SRC := ../aquarium/src
CORE := $(wildcard $(SRC)/engine/*.c $(SRC)/scene/*.c $(SRC)/config/*.c $(SRC)/ui/*.c $(SRC)/app/*.c) hal_sim.c png_write.c
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -D_DEFAULT_SOURCE
OUT ?= ../build-sim

all: $(OUT)/aquarium_headless $(OUT)/aquarium_sim

$(OUT):
	mkdir -p $(OUT)

$(OUT)/aquarium_headless: headless.c $(CORE) | $(OUT)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(OUT)/aquarium_sim: main_sdl.c $(CORE) | $(OUT)
	$(CC) $(CFLAGS) $$(sdl2-config --cflags) -o $@ $^ $$(sdl2-config --libs) -lm

.PHONY: all
