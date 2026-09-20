# GNUmakefile — top-level wrapper around CMake
#
# Usage:
#   make               -> build with CMake's default generator (release)
#   make ninja         -> build with Ninja (faster)
#   make debug         -> build debug
#   make release       -> build release
#   make test          -> build + run ctest
#   make lint          -> run clang-tidy
#   make format        -> run clang-format
#   make clean         -> delete build dir
#   make rebuild       -> clean + build
#   make run           -> build + run ElysiumApp
#
# Environment overrides:
#   BUILD_DIR=build/foo make debug
#   JOBS=8 make debug

SHELL := /bin/bash

# Suppress "Entering/Leaving directory" noise from recursive make
MAKEFLAGS += --no-print-directory

# --- Detect platform for build dir suffix ---
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM := linux
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM := macos
    endif
    PLATFORM ?= unknown
endif

# --- Configuration ---
BUILD_TYPE     ?= Release
JOBS           ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CONFIG         ?= $(shell echo $(BUILD_TYPE) | tr A-Z a-z)
BUILD_DIR      ?= build/$(PLATFORM)-$(CONFIG)
TEST_BUILD_DIR ?= build/$(PLATFORM)-$(CONFIG)-test

# --- Detect Ninja availability ---
NINJA := $(shell which ninja 2>/dev/null)
NINJA_AVAILABLE := $(if $(NINJA),1,0)

# --- Tooling detection ---
CLANG_FORMAT := $(shell which clang-format 2>/dev/null)
CLANG_TIDY   := $(shell which clang-tidy 2>/dev/null)

# --- Convenience ---
CMAKE := cmake

# ANSI colors
GREEN := \033[32m
RED   := \033[31m
CYAN  := \033[36m
RESET := \033[0m

# ============================================================
#  Phony targets
# ============================================================
.PHONY: all help debug release ninja ninja-debug _build _build-ninja \
        run run-debug test test-debug _test \
        lint lint-fix format \
        clean distclean rebuild info

# --- Default target ---
all: release                                 ## Build release (default, CMake generator)

# ============================================================
#  Help
# ============================================================
help:                                        ## Show this help message
	@printf "\n"
	@printf "ElysiumEngine build system\n"
	@printf "==========================\n\n"
	@printf "Usage: make [target] [VAR=value]\n\n"
	@printf "Targets:\n"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) \
		| sort \
		| awk 'BEGIN {FS = ":.*?## "}; {printf "  $(CYAN)%-16s$(RESET) %s\n", $$1, $$2}'
	@printf "\n"
	@printf "Variables (override on command line):\n"
	@printf "  $(CYAN)BUILD_TYPE$(RESET)      Release (default) | Debug | RelWithDebInfo | MinSizeRel\n"
	@printf "  $(CYAN)BUILD_DIR$(RESET)       build/<platform>-<config> (default)\n"
	@printf "  $(CYAN)JOBS$(RESET)            $(JOBS) (default: detected CPUs)\n"
	@printf "\n"
	@printf "Generators:\n"
	@printf "  $(CYAN)make$(RESET)              Uses CMake's default generator\n"
	@printf "  $(CYAN)make ninja$(RESET)        Uses Ninja (faster, recommended)\n"
	@printf "                      $(if $(filter 1,$(NINJA_AVAILABLE)),Ninja is installed, Ninja NOT found)\n"
	@printf "\n"

# ============================================================
#  Configuration stamps
# ============================================================
$(BUILD_DIR)/CMakeCache.txt:
	@echo "==> Configuring $(BUILD_TYPE) (default generator) in $(BUILD_DIR)"
	@$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

$(BUILD_DIR)-ninja/CMakeCache.txt:
	@echo "==> Configuring $(BUILD_TYPE) with Ninja in $(BUILD_DIR)-ninja"
	@$(CMAKE) -S . -B $(BUILD_DIR)-ninja -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

$(TEST_BUILD_DIR)-ninja/CMakeCache.txt:
	@echo "==> Configuring $(BUILD_TYPE) tests with Ninja in $(TEST_BUILD_DIR)-ninja"
	@$(CMAKE) -S . -B $(TEST_BUILD_DIR)-ninja -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=ON

# ============================================================
#  Default-generator build path
# ============================================================
debug:                                       ## Build debug (default generator)
	@$(MAKE) BUILD_TYPE=Debug   CONFIG=debug   _build
release:                                     ## Build release (default generator)
	@$(MAKE) BUILD_TYPE=Release CONFIG=release _build

_build: $(BUILD_DIR)/CMakeCache.txt
	@echo "==> Building $(BUILD_TYPE) (default generator, $(JOBS) jobs)"
	@MAKEFLAGS="--no-print-directory" $(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS)
	@printf "\n"
	@printf "  $(GREEN)✓ Build succeeded$(RESET) — $(BUILD_TYPE)\n"
	@printf "  Binary: $(BUILD_DIR)/bin/ElysiumApp\n"
	@printf "\n"

# ============================================================
#  Ninja build path
# ============================================================
ninja:                                       ## Build with Ninja (release)
	@$(MAKE) BUILD_TYPE=Release CONFIG=release _build-ninja

ninja-debug:                                 ## Build with Ninja (debug)
	@$(MAKE) BUILD_TYPE=Debug CONFIG=debug _build-ninja

_build-ninja:
ifeq ($(NINJA_AVAILABLE),0)
	@printf "  $(RED)✗ Ninja not found$(RESET) — install it (apt install ninja-build / brew install ninja)\n"
	@exit 1
else
	@$(MAKE) _build-ninja-inner
endif

_build-ninja-inner: $(BUILD_DIR)-ninja/CMakeCache.txt
	@echo "==> Building $(BUILD_TYPE) with Ninja, $(JOBS) jobs"
	@ninja -C $(BUILD_DIR)-ninja -j $(JOBS)
	@printf "\n"
	@printf "  $(GREEN)✓ Build succeeded$(RESET) — $(BUILD_TYPE) (Ninja)\n"
	@printf "  Binary: $(BUILD_DIR)-ninja/bin/ElysiumApp\n"
	@printf "\n"

# ============================================================
#  Tests
# ============================================================
test:                                        ## Build + run all tests
ifeq ($(NINJA_AVAILABLE),0)
	@printf "  $(RED)✗ Ninja not found$(RESET) — install it to run tests\n"
	@exit 1
else
	@$(MAKE) BUILD_TYPE=Release CONFIG=release _test
endif

test-debug:                                  ## Build + run tests in debug mode
ifeq ($(NINJA_AVAILABLE),0)
	@printf "  $(RED)✗ Ninja not found$(RESET) — install it to run tests\n"
	@exit 1
else
	@$(MAKE) BUILD_TYPE=Debug CONFIG=debug _test
endif

_test: $(TEST_BUILD_DIR)-ninja/CMakeCache.txt
	@echo "==> Building tests ($(BUILD_TYPE)) with Ninja, $(JOBS) jobs"
	@ninja -C $(TEST_BUILD_DIR)-ninja -j $(JOBS)
	@printf "\n"
	@echo "==> Running tests"
	@cd $(TEST_BUILD_DIR)-ninja && ctest --output-on-failure
	@printf "\n"
	@printf "  $(GREEN)✓ Tests passed$(RESET) — $(BUILD_TYPE)\n"
	@printf "\n"

# ============================================================
#  Lint (clang-tidy)
# ============================================================
lint:                                        ## Run clang-tidy on sources
ifeq ($(CLANG_TIDY),)
	@printf "  $(RED)✗ clang-tidy not found$(RESET) — install it (apt install clang-tidy / brew install llvm)\n"
	@exit 1
else
ifeq ($(NINJA_AVAILABLE),0)
	@printf "  $(RED)✗ Ninja not found$(RESET) — clang-tidy needs compile_commands.json from a Ninja build\n"
	@exit 1
else
	@$(MAKE) BUILD_TYPE=Release CONFIG=release _lint
endif
endif

_lint: $(BUILD_DIR)-ninja/CMakeCache.txt
	@echo "==> Ensuring compile_commands.json is up to date"
	@$(CMAKE) -S . -B $(BUILD_DIR)-ninja -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
	@echo "==> Running clang-tidy on app/, engine/, test/"
	@find app engine test \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print > /tmp/elysium_lint_files.txt
	@$(CLANG_TIDY) -p $(BUILD_DIR)-ninja --quiet $$(cat /tmp/elysium_lint_files.txt)
	@printf "\n"
	@printf "  $(GREEN)✓ clang-tidy passed$(RESET)\n"
	@printf "\n"

lint-fix:                                    ## Run clang-tidy with --fix (auto-apply suggestions)
ifeq ($(CLANG_TIDY),)
	@printf "  $(RED)✗ clang-tidy not found$(RESET) — install it to use 'make lint-fix'\n"
	@exit 1
else
ifeq ($(NINJA_AVAILABLE),0)
	@printf "  $(RED)✗ Ninja not found$(RESET) — clang-tidy needs compile_commands.json\n"
	@exit 1
else
	@echo "==> Ensuring compile_commands.json is up to date"
	@$(CMAKE) -S . -B $(BUILD_DIR)-ninja -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
	@echo "==> Running clang-tidy --fix on app/, engine/, test/"
	@find app engine test \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print > /tmp/elysium_lint_files.txt
	@$(CLANG_TIDY) -p $(BUILD_DIR)-ninja --fix --quiet $$(cat /tmp/elysium_lint_files.txt)
	@printf "\n"
	@printf "  $(GREEN)✓ clang-tidy --fix done$(RESET)\n"
	@printf "  Review changes with: git diff\n"
	@printf "\n"
endif
endif

# ============================================================
#  Run
# ============================================================
run:                                         ## Build + run ElysiumApp
	@$(MAKE) ninja
	@echo "==> Running ElysiumApp"
	@$(BUILD_DIR)-ninja/bin/ElysiumApp

run-debug:                                   ## Build + run in debug mode
	@$(MAKE) ninja-debug
	@echo "==> Running ElysiumApp (debug)"
	@$(BUILD_DIR)-ninja/bin/ElysiumApp

# ============================================================
#  Formatting
# ============================================================
format:                                      ## Run clang-format on sources
ifeq ($(CLANG_FORMAT),)
	@printf "  $(RED)✗ clang-format not found$(RESET) — install it to use 'make format'\n"
	@exit 1
else
	@echo "==> Formatting C++ sources"
	@find app engine test \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print | xargs $(CLANG_FORMAT) -i
	@printf "\n"
	@printf "  $(GREEN)✓ Formatted$(RESET)\n"
	@printf "\n"
endif

# ============================================================
#  Cleanup
# ============================================================
clean:                                       ## Remove current build dirs
	@echo "==> Removing build dirs for $(BUILD_TYPE)"
	@rm -rf $(BUILD_DIR) $(BUILD_DIR)-ninja $(TEST_BUILD_DIR)-ninja

distclean:                                   ## Remove the entire build/ tree
	@echo "==> Removing entire build/ tree"
	@rm -rf build

rebuild: clean                               ## Clean + rebuild (Ninja)
	@$(MAKE) ninja

# ============================================================
#  Info
# ============================================================
info:                                        ## Print current build configuration
	@printf "\n"
	@printf "Platform        : $(PLATFORM)\n"
	@printf "Build type      : $(BUILD_TYPE)\n"
	@printf "Default build   : $(BUILD_DIR)\n"
	@printf "Ninja build     : $(BUILD_DIR)-ninja\n"
	@printf "Test build      : $(TEST_BUILD_DIR)-ninja\n"
	@printf "Jobs            : $(JOBS)\n"
	@printf "Ninja           : $(if $(filter 1,$(NINJA_AVAILABLE)),$(NINJA),<not found>)\n"
	@printf "clang-format    : $(if $(CLANG_FORMAT),$(CLANG_FORMAT),<not found>)\n"
	@printf "clang-tidy      : $(if $(CLANG_TIDY),$(CLANG_TIDY),<not found>)\n"
	@printf "\n"