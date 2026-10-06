CXX      ?= g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -Wpedantic -pthread
LDFLAGS   =

# Optional readline support
HAVE_READLINE ?= $(shell echo '\#include <readline/readline.h>' | $(CXX) -x c++ -E - >/dev/null 2>&1 && echo 1 || echo 0)
ifeq ($(HAVE_READLINE),1)
  CXXFLAGS += -DHAVE_READLINE
  LDFLAGS  += -lreadline
endif

# Build mode
ifeq ($(MODE),release)
  CXXFLAGS += -O2 -DNDEBUG
else
  CXXFLAGS += -g -O0
endif

SRCS = main.cxx core.cxx memory.cxx periph.cxx uart.cxx bf16.cxx div.cxx \
       disasm.cxx simulator.cxx cli.cxx serial.cxx
OBJS = $(SRCS:.cxx=.o)
TARGET = lisa_sim

.PHONY: all clean debug release

all: $(TARGET)

debug:
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release all

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

# Dependencies (auto-generated would be better, but keep it simple)
main.o:      main.cxx simulator.h cli.h
core.o:      core.cxx core.h memory.h disasm.h bf16.h div.h
memory.o:    memory.cxx memory.h
periph.o:    periph.cxx periph.h uart.h
uart.o:      uart.cxx uart.h serial.h
bf16.o:      bf16.cxx bf16.h
div.o:       div.cxx div.h
disasm.o:    disasm.cxx disasm.h
simulator.o: simulator.cxx simulator.h core.h memory.h periph.h uart.h
cli.o:       cli.cxx cli.h simulator.h disasm.h
serial.o:    serial.cxx serial.h
