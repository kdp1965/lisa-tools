############################################################################
#   Copyright (C) 2018 Ken Pettit. All rights reserved.
#   Author: Ken Pettit <kpettit@iqanalog.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

.SILENT:

APPNAME = lisa

SRCDIR   = src
OBJDRI   = obj
DEPDIR   = .dep
DEPS     = $(wildcard $(DEPDIR)/*.d)

CFLAGS   = -Wall -g -DSTANDALONE -I tui -I include -I ../lisa_cdb
CXXFLAGS = $(CFLAGS) -std=c++17
CXXSRCS  = $(SRCDIR)/Lisa.cxx $(SRCDIR)/LisaCdb.cxx $(SRCDIR)/Main.cxx
CXXSRCS += $(SRCDIR)/serial.cxx
CXXSRCS += tui/Tui.cxx tui/simevent.cxx tui/thread.cxx tui/Tabs.cxx tui/History.cxx
# the .cdb reader and the image loader, shared with lisa_sim
CXXSRCS += ../lisa_cdb/cdb.cxx ../lisa_cdb/image.cxx
LIBS     = -lreadline -lm -lpthread

# macOS: Homebrew ncurses/readline when installed (/opt/homebrew only: /usr/local
# holds an x86_64 Homebrew whose libraries do not link for arm64); otherwise the
# system ncurses, which is wide already
BREW_NCURSES = $(wildcard /opt/homebrew/opt/ncurses)
ifneq ($(BREW_NCURSES),)
  CFLAGS += -I$(BREW_NCURSES)/include
  LIBS   += -L$(BREW_NCURSES)/lib -lncursesw -lpanelw
  BREW_READLINE = $(wildcard /opt/homebrew/opt/readline)
  ifneq ($(BREW_READLINE),)
    CFLAGS += -I$(BREW_READLINE)/include
    LIBS   += -L$(BREW_READLINE)/lib
  endif
else
  LIBS   += -lcurses -lpanel
endif

COBJS    = $(CSRCS:.c=.o)
CXXOBJS  = $(CXXSRCS:.cxx=.o)
OBJS     = $(CXXOBJS) $(COBJS)

%.o: %.cxx
	@echo Compiling $<
	g++ $(CXXFLAGS) -g -c $< -o $@
	$(QD) $(CC) -MM -MT $*.o $(CFLAGS) $*.cxx > $(DEPDIR)/$(shell basename $*.d)

%.o: %.c
	@echo Compiling $<
	gcc $(CFLAGS) -g -c $< -o $@
	$(QD) $(CC) -MM -MT $*.o $(CFLAGS) $*.c > $(DEPDIR)/$(shell basename $*.d)

$(APPNAME): init $(OBJS)
	@echo Linking $(APPNAME)
	g++ $(CFLAGS) -g -o $(APPNAME) $(OBJS) $(LIBS)

-include $(DEPS)

init: 
	@mkdir -p $(DEPDIR)

clean:
	@rm -f $(OBJS)
	@rm -f $(SRCDIR)/*.o
	@rm -f ../tui/*.o
	@rm -f $(APPNAME)
	@rm -rf $(DEPDIR)
