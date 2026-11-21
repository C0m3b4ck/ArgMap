# ArgMap - build with the FLTK/FLUID toolchain already installed.
# Editing gui/MapWindow.fl and running `make` regenerates the UI code.

CXX ?= g++
FLUID ?= fluid
FLTK_CONFIG ?= fltk-config

BUILD := build
GEN := $(BUILD)/generated
OBJ := $(BUILD)/obj
BIN := argmap

FLTK_CXXFLAGS := $(shell $(FLTK_CONFIG) --cxxflags)
FLTK_LDFLAGS := $(shell $(FLTK_CONFIG) --ldflags)

CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -I. -I$(GEN) $(FLTK_CXXFLAGS)
LDLIBS += $(FLTK_LDFLAGS) -lz

UI_FL := gui/MapWindow.fl
UI_H := $(GEN)/MapWindow.H
UI_CPP := $(GEN)/MapWindow.cpp

SOURCES := main.cpp \
           $(wildcard domain/*.cpp) \
           $(wildcard data/*.cpp) \
           $(wildcard app/*.cpp) \
           $(wildcard gui/*.cpp) \
           $(UI_CPP)

OBJECTS := $(SOURCES:%.cpp=$(OBJ)/%.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean test regenerate

all: $(BIN)

$(BIN): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# The generated header must exist before any source that includes it compiles.
$(OBJECTS): | $(UI_H)

$(UI_H) $(UI_CPP) &: $(UI_FL)
	@mkdir -p $(GEN)
	cd $(GEN) && $(FLUID) -c -o MapWindow.cpp -h MapWindow.H $(abspath $<)

$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

regenerate:
	rm -f $(UI_H) $(UI_CPP)
	$(MAKE) $(UI_H) $(UI_CPP)

TEST_BIN := $(BUILD)/domain_tests
TEST_SOURCES := tests/domain_tests.cpp $(wildcard domain/*.cpp) data/JsonMapRepository.cpp data/Json.cpp data/PngExporter.cpp
TEST_OBJECTS := $(TEST_SOURCES:%.cpp=$(OBJ)/test/%.o)
TEST_DEPS := $(TEST_OBJECTS:.o=.d)

test: $(TEST_BIN)
	$(TEST_BIN)

$(TEST_BIN): $(TEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lz

$(OBJ)/test/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)
-include $(TEST_DEPS)

clean:
	rm -rf $(BUILD) $(BIN)

