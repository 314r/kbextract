BUILD_DIR ?= build
PREFIX ?= $(HOME)/.local
VERSION ?= 0.1.0

.PHONY: all build install clean run test package-linux

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

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

package-linux: build
	packaging/linux/build-packages.sh $(BUILD_DIR) $(BUILD_DIR)/packages $(VERSION)
