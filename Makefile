BUILD_DIR ?= build
PREFIX ?= $(HOME)/.local

.PHONY: all build install clean run

all: build

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) --parallel

install: build
	cmake --install $(BUILD_DIR) --prefix $(PREFIX)

clean:
	cmake --build $(BUILD_DIR) --target clean

run: build
	$(BUILD_DIR)/kbextract

