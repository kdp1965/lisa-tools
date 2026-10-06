
# Define our tools
CC = ../lisa_cc/lisa_cc
AS = ../lisa_as/lisa_as
LD = ../lisa_ld/lisa_ld
DBG = ../lisa_pydb/lisa_pydb.py
ASFLAGS = -w16 -7
#ASFLAGS = -w16
CFLAGS = -O2 -S
FILES = ../lisa_as/lib/out/__uctoint.rel ../lisa_as/lib/out/puts_c.rel ../lisa_as/lib/out/printf.rel ../lisa_as/lib/out/console_putc.rel 

PROG = bringup

ifeq ($(V),1)
   Q =
else
   Q = @
endif

# The main target is out output HEX file
all:  $(PROG)

# Clean the project
clean:
	@rm -rf $(PROG)
	@rm -rf $(PROG).rel
	@rm -rf $(PROG).S

# Final link step to create the HEX output file
$(PROG): $(PROG).rel isr_test.rel
	@$(LD) -T lisa.ld -M -o $@ -t isr_test.rel $<

# Assemble the C file to a relocatable object (.rel) file
$(PROG).rel: $(PROG).S
	$(Q)$(AS) $(ASFLAGS) -o $@ $<

# Assemble the C file to a relocatable object (.rel) file
print_hello.rel: print_hello.S
	$(Q)$(AS) $(ASFLAGS) -o $@ $<

# Assemble the C file to a relocatable object (.rel) file
isr_test.rel: isr_test.S
	$(Q)$(AS) $(ASFLAGS) -o $@ $<

# Assemble the C file to a relocatable object (.rel) file
fops_test.rel: fops_test.S
	$(Q)$(AS) $(ASFLAGS) -o $@ $<

# Compile the C file to an assembly (.S) file
$(PROG).S: $(PROG).c
	$(Q)$(CC) $(CFLAGS) -o $@ $<

# Program the lisa SoC and tell it to run
prog: $(PROG)
	@$(DBG) --load -V --run $<

ast: 
	$(Q)$(CC) $(CFLAGS) -fdump-ast $(PROG).c > $(PROG).ast

# Load code to the lisa SoC but don't run
load: $(PROG)
	@$(DBG) --load -V $<

# Verify the program load
verify: $(PROG)
	@$(DBG) -V --verify $<

# Report the lisa core Program Counter
pc: $(PROG)
	@$(DBG)  --pc $<

# Report the lisa core Stack Pointer
sp: $(PROG)
	@$(DBG)  --sp $<

# Tell the list core to run
run: $(PROG)
	@$(DBG)  --run $<

# Tell the list core to halt
halt: $(PROG)
	@$(DBG)  --halt $<

# Verify the program load
ver: $(PROG)
	@$(DBG) -V  $<

stack: $(PROG)
	@$(DBG) --stack $<

reset: $(PROG)
	@$(DBG) --reset $<

step: $(PROG)
	@$(DBG) --step $<

flash: $(PROG)
	@$(DBG) --flash $<

