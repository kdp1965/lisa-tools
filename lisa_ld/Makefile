# ------------------------------------------------------------------------------
# (c) Copyright 
#         All Rights Reserved
# ------------------------------------------------------------------------------
#
# Module:  Makefile for lisa-ld linker
#
# ------------------------------------------------------------------------------
#
#    Author:                   Ken Pettit
#    Created:                  04/04/2016
#
# Description:  
#    This is a makefile for the lisa linker project.
#
# Modifications:
#
#    Author            Date        Ver  Description
#    ================  ==========  ===  =======================================
#    Ken Pettit        04/04/2016  1.0  Initial version
#
# ------------------------------------------------------------------------------

TARGET   = lisa_ld

CFLAGS   = -g
LDFLAGS  = -g -Bstatic
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
all: init $(TARGET)

#==============================================================================
# Example test.asm build target.  To build, type "make test" from command line.
#==============================================================================
test: test.csv

test.csv: $(TARGET) test.asm
	@$(MAKE)
	./$(TARGET) -m -o $@ test.asm

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

clean:
	@rm -rf $(OBJDIR) $(DEPDIR)
	@rm -f $(TARGET)
	@rm -f test.csv

