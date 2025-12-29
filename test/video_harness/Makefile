# Video Test Harness Makefile

CC = gcc
CXX = g++

# Build mode (default: dev with hardening)
# Options: dev, production
BUILD_MODE ?= dev

ifeq ($(BUILD_MODE),production)
  # Production build (optimized, no sanitizers)
  CFLAGS = -Wall -Wextra -std=c11 -O2 -DNDEBUG
  CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -DNDEBUG
  LDFLAGS = -lm -lstdc++
  $(info Building in PRODUCTION mode (optimized, no hardening))
else
  # Development build (ALWAYS includes hardening)
  CFLAGS = -Wall -Wextra -std=c11 -O1 -g \
           -D_FORTIFY_SOURCE=2 \
           -fstack-protector-strong \
           -fstack-clash-protection \
           -fsanitize=address \
           -fsanitize=undefined \
           -fsanitize=leak \
           -fno-omit-frame-pointer
  CXXFLAGS = -Wall -Wextra -std=c++17 -O1 -g \
             -D_FORTIFY_SOURCE=2 \
             -fstack-protector-strong \
             -fstack-clash-protection \
             -fsanitize=address \
             -fsanitize=undefined \
             -fsanitize=leak \
             -fno-omit-frame-pointer
  LDFLAGS = -lm -lstdc++ -fsanitize=address -fsanitize=undefined -fsanitize=leak
  $(info Building in DEVELOPMENT mode (debugging + hardening))
  $(info   + Buffer overflow detection: _FORTIFY_SOURCE=2)
  $(info   + Stack canaries: stack-protector-strong)
  $(info   + Stack clash protection)
  $(info   + AddressSanitizer: heap/stack/global overflow detection)
  $(info   + UndefinedBehaviorSanitizer)
  $(info   + LeakSanitizer)
  $(info   ⚠️  This build will ABORT on memory errors!)
endif

# Wasmtime
WASMTIME_CFLAGS = -I/usr/include
WASMTIME_LDFLAGS = -lwasmtime

# GStreamer
GST_CFLAGS = $(shell pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0)
GST_LDFLAGS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-app-1.0)

# Nanopb (for protobuf encoding)
NANOPB_DIR = ../../src/proto
NANOPB_CFLAGS = -I$(NANOPB_DIR)

# Combined flags
ALL_CFLAGS = $(CFLAGS) $(WASMTIME_CFLAGS) $(GST_CFLAGS) $(NANOPB_CFLAGS)
ALL_CXXFLAGS = $(CXXFLAGS) $(WASMTIME_CFLAGS) $(GST_CFLAGS) $(NANOPB_CFLAGS) -I/usr/local/include -I/usr/local/include/nlohmann
ALL_LDFLAGS = $(LDFLAGS) $(WASMTIME_LDFLAGS) $(GST_LDFLAGS)

# Source files
C_SRCS = main.c wasm_loader.c gst_pipeline.c synthetic_state.c
CXX_SRCS = config_validator.cpp
C_OBJS = $(C_SRCS:.c=.o)
CXX_OBJS = $(CXX_SRCS:.cpp=.o)
OBJS = $(C_OBJS) $(CXX_OBJS)

# JSON Schema Validator sources (from /usr/local/src in Docker)
VALIDATOR_SRCS = /usr/local/src/json-uri.cpp \
                 /usr/local/src/json-validator.cpp \
                 /usr/local/src/json-schema-draft7.json.cpp \
                 /usr/local/src/string-format-check.cpp \
                 /usr/local/src/smtp-address-validator.cpp \
                 /usr/local/src/json-patch.cpp

# Object files for validator (in current directory to avoid permission issues)
VALIDATOR_OBJS = json-uri.o \
                 json-validator.o \
                 json-schema-draft7.json.o \
                 string-format-check.o \
                 smtp-address-validator.o \
                 json-patch.o

# Nanopb source files
NANOPB_SRCS = $(NANOPB_DIR)/pb_common.c \
              $(NANOPB_DIR)/pb_encode.c \
              $(NANOPB_DIR)/jon_shared_data.pb.c \
              $(NANOPB_DIR)/jon_shared_data_compass.pb.c \
              $(NANOPB_DIR)/jon_shared_data_rotary.pb.c \
              $(NANOPB_DIR)/jon_shared_data_time.pb.c \
              $(NANOPB_DIR)/jon_shared_data_system.pb.c \
              $(NANOPB_DIR)/jon_shared_data_gps.pb.c \
              $(NANOPB_DIR)/jon_shared_data_camera_day.pb.c \
              $(NANOPB_DIR)/jon_shared_data_camera_heat.pb.c \
              $(NANOPB_DIR)/jon_shared_data_compass_calibration.pb.c \
              $(NANOPB_DIR)/jon_shared_data_rec_osd.pb.c \
              $(NANOPB_DIR)/jon_shared_data_day_cam_glass_heater.pb.c \
              $(NANOPB_DIR)/jon_shared_data_actual_space_time.pb.c \
              $(NANOPB_DIR)/jon_shared_data_lrf.pb.c \
              $(NANOPB_DIR)/jon_shared_data_types.pb.c
NANOPB_OBJS = $(NANOPB_SRCS:.c=.o)

# Output binary
TARGET = video_harness

.PHONY: all clean check-deps run

all: check-deps $(TARGET)

$(TARGET): $(OBJS) $(NANOPB_OBJS) $(VALIDATOR_OBJS)
	@echo "Linking $@..."
	$(CC) -o $@ $^ $(ALL_LDFLAGS)
	@echo "✅ Build complete: $(TARGET)"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(ALL_CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

$(NANOPB_DIR)/%.o: $(NANOPB_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Validator compilation rules (output .o files to current dir)
json-uri.o: /usr/local/src/json-uri.cpp
	@echo "Compiling $<..."
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

json-validator.o: /usr/local/src/json-validator.cpp
	@echo "Compiling $<..."
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

json-schema-draft7.json.o: /usr/local/src/json-schema-draft7.json.cpp
	@echo "Compiling $<..."
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

string-format-check.o: /usr/local/src/string-format-check.cpp
	@echo "Compiling $<..."
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

smtp-address-validator.o: /usr/local/src/smtp-address-validator.cpp
	@echo "Compiling $<..."
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

json-patch.o: /usr/local/src/json-patch.cpp
	@echo "Compiling $<..."
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

check-deps:
	@echo "Checking dependencies..."
	@command -v pkg-config >/dev/null 2>&1 || { echo "ERROR: pkg-config not found"; exit 1; }
	@pkg-config --exists gstreamer-1.0 || { echo "ERROR: GStreamer 1.0 not found"; exit 1; }
	@pkg-config --exists gstreamer-app-1.0 || { echo "ERROR: GStreamer app not found"; exit 1; }
	@ldconfig -p | grep -q libwasmtime || { echo "ERROR: Wasmtime library not found"; exit 1; }
	@echo "✅ All dependencies found"

run: $(TARGET)
	@echo "Running video harness..."
	@mkdir -p ../output
	./$(TARGET)

run-single: $(TARGET)
	@echo "Running video harness for single variant..."
	@mkdir -p ../output
	@if [ -z "$(VARIANT)" ]; then \
		echo "ERROR: Please specify VARIANT=<name>"; \
		echo "Available variants: live_day, live_thermal, recording_day, recording_thermal"; \
		exit 1; \
	fi
	./$(TARGET) $(VARIANT)

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJS) $(NANOPB_OBJS) $(VALIDATOR_OBJS) $(TARGET)
	@echo "✅ Clean complete"

clean-all: clean
	@echo "Cleaning output videos..."
	rm -f ../output/*.mp4
	@echo "✅ Clean all complete"

help:
	@echo "Video Test Harness Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build video harness (default)"
	@echo "  run          - Build and run (generate all 4 videos)"
	@echo "  run-single   - Build and run single variant (VARIANT=live_day)"
	@echo "  check-deps   - Check if dependencies are installed"
	@echo "  clean        - Remove build artifacts"
	@echo "  clean-all    - Remove build artifacts and output videos"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build"
	@echo "  make run                # Generate all videos"
	@echo "  make run-single VARIANT=live_day  # Generate single video"
	@echo ""
	@echo "Dependencies:"
	@echo "  - GStreamer 1.0 (gstreamer1.0-dev, gstreamer1.0-plugins-base)"
	@echo "  - GStreamer plugins (gstreamer1.0-plugins-good, gstreamer1.0-plugins-ugly)"
	@echo "  - Wasmtime C library (libwasmtime)"
	@echo "  - pkg-config"
