CC      = GCC
CFLAGS  = -g -O0 -Wall -DHAVE_CONFIG_H -I. -Iinclude
LDFLAGS =

# ---- Homebrew (Apple Silicon) ----
BREW_PREFIX := /opt/homebrew

SDL2_CFLAGS     := $(shell $(BREW_PREFIX)/bin/sdl2-config --cflags) -I$(BREW_PREFIX)/include
SDL2_LIBS       := $(shell $(BREW_PREFIX)/bin/sdl2-config --libs)
SDL2_TTF_CFLAGS := -I$(BREW_PREFIX)/opt/sdl2_ttf/include/SDL2
SDL2_TTF_LIBS   := -L$(BREW_PREFIX)/opt/sdl2_ttf/lib -lSDL2_ttf

CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LIBS   := $(shell pkg-config --libs check)

# ---- Directories ----
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj

# ---- Sources ----
LIB_SRCS   := $(wildcard lib/*.c)
LIB_OBJS   := $(patsubst lib/%.c,$(OBJ_DIR)/lib/%.o,$(LIB_SRCS))

GBEMU_SRCS := gbemu/main.c
GBEMU_OBJS := $(patsubst gbemu/%.c,$(OBJ_DIR)/gbemu/%.o,$(GBEMU_SRCS))

TEST_SRCS  := tests/check_gbe.c
TEST_OBJS  := $(patsubst tests/%.c,$(OBJ_DIR)/tests/%.o,$(TEST_SRCS))

FONT       := NotoSansMono-Medium.ttf

# ---- Targets ----
all: $(BUILD_DIR)/gbemu

$(BUILD_DIR)/libemu.a: $(LIB_OBJS)
	ar rcs $@ $(LIB_OBJS)

$(BUILD_DIR)/gbemu: $(GBEMU_OBJS) $(BUILD_DIR)/libemu.a
	$(CC) $(GBEMU_OBJS) -L$(BUILD_DIR) -lemu $(SDL2_LIBS) $(SDL2_TTF_LIBS) -o $@
	cp $(FONT) $(BUILD_DIR)/

$(BUILD_DIR)/check_gbe: $(TEST_OBJS) $(BUILD_DIR)/libemu.a
	$(CC) $(TEST_OBJS) -L$(BUILD_DIR) -lemu $(SDL2_LIBS) $(SDL2_TTF_LIBS) $(CHECK_LIBS) -o $@

test: $(BUILD_DIR)/check_gbe
	cd $(BUILD_DIR) && ./check_gbe

# ---- Compile rules (create output subdirs as needed) ----
$(OBJ_DIR)/lib/%.o: lib/%.c | $(OBJ_DIR)/lib
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_TTF_CFLAGS) -c $< -o $@

$(OBJ_DIR)/gbemu/%.o: gbemu/%.c | $(OBJ_DIR)/gbemu
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_TTF_CFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c | $(OBJ_DIR)/tests
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) -c $< -o $@

$(OBJ_DIR)/lib $(OBJ_DIR)/gbemu $(OBJ_DIR)/tests:
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test