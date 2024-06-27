#
# Module:  Makefile for lisa_asm
#
# Copyright 2024 by Ken Pettit <pettitkd@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# ------------------------------------------------------------------------------

TARGET   = lisa_as

CFLAGS   = -g
LDFLAGS  = -g
CC       = $(CROSS_COMPILE)g++
LIBS     = -lstdc++

DEPDIR   = .dep
OBJDIR   = obj

# Compile all CPP files in the project
SRC      = $(wildcard *.cpp)
OTMP     = $(SRC:.cpp=.o)
OBJFILES = $(patsubst %,$(OBJDIR)/%,$(OTMP))
DEPS     = $(patsubst %.o,$(DEPDIR)/%.d,$(OTMP))

#==============================================================================
# Main target is $(TARGET)
#==============================================================================
all: init $(TARGET) libs

#Include our built dependencies
-include $(DEPS)
 
# Make our dependencies from the source
%.d: %.cpp
	$(SHELL) -ec '$(CC) -M $(SRC) $< | sed '\"s/$*.o/& $@/g'\" > .dep/$@'

# The rule to make our target
$(TARGET):  $(OBJFILES)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJFILES) -o $(TARGET)

# The rule to compile our sources and build dependencies
$(OBJDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM -MT $(OBJDIR)/$*.o $(CFLAGS) $*.cpp > $(DEPDIR)/$*.d

init:	
	@mkdir -p $(DEPDIR)
	@mkdir -p $(OBJDIR)

libs:
	@echo =======================================================
	@echo Building LISA libraries
	@echo =======================================================
	@$(MAKE) -C lib

clean:
	@rm -rf $(OBJDIR) $(DEPDIR)
	@rm -f $(TARGET)
	@$(MAKE) -C lib clean

