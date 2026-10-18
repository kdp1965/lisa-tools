#!/usr/bin/python3.11
import serial
import io
import time
import argparse

# =====================================================
# Python based Lisa debugger
# =====================================================
class LisaPyDb:
   def openPort(self, name, baud=460800):
      #self.ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=0.05)
      self.ser = serial.Serial(name, baud, timeout=0.05)
      self.sio = io.TextIOWrapper(io.BufferedRWPair(self.ser, self.ser))

   # =============================================
   # Get the Lisa HW version
   # =============================================
   def init(self):
      # Send \n and test response for >>> REPL
      repl = False
      for x in range(5):
         self.sio.write(f'\r')
         self.sio.flush()
         time.sleep(0.2)
         self.sio.readline()
         resp = self.sio.readline().strip()
         if '>>>' in resp:
            repl = True
            break
         if len(resp) > 0:
            print(resp)

      # Verify REPL found
      if repl:
         print('REPL detected')
         self.sio.write(f'print(tt.shuttle)\r')
         self.sio.flush()
         self.sio.readline()
         resp = self.sio.readline()

         # Test for tt06 shuttle
         if 'tt06' in resp:
            print('Shuttle tt06 detected')
            self.sio.write(f'tt.shuttle.tt_um_lisa.enable()\r')
         elif 'tt07' in resp:
            print('Shuttle tt07 detected')
            self.sio.write(f'tt.shuttle.tt_um_lisa.enable()\r')
         self.sio.flush()

         # Now read lines until REPL received
         for x in range(10):
            time.sleep(1)
            resp = self.sio.readline().strip()
            if len(resp) > 0:
               if '>>>' in resp:
                  break
               print(resp)

         # Now import uartPass to get access to the debugger
         self.sio.write(f'import uartPass\r')
         self.sio.flush()
         time.sleep(0.2)
         self.sio.readline()
         resp = self.sio.readline().strip()

         # Check if import already performed, then we must
         # manually run the passthrough function
         if '>>>' in resp:
            self.sio.write(f'uartPass.passthrough()\r')
            self.sio.flush()
            time.sleep(0.2)

   # =============================================
   # Get the Lisa HW version
   # =============================================
   def getVer(self):
      for x in range(5):
         self.sio.write('\n')
         self.sio.flush()
         self.sio.readline()
         self.sio.readline()
      self.sio.write('v')
      self.sio.flush()
      ver = self.sio.readline().strip()
      return ver

   # =============================================
   # Write a register
   # =============================================
   def writeReg(self, reg:int, val:int):
      self.sio.write(f'w{reg:2x}{val:4x}\n')
      self.sio.flush()

   # =============================================
   # Read a LISA reg
   # =============================================
   def readReg(self, reg:int):
      self.sio.write(f'r{reg:2x}\n')
      self.sio.flush()
      val = int(self.sio.readline().strip(), 16)
      return val

   # =============================================
   # Set the lisa core PC value
   # =============================================
   def writePC(self, val:int):
      self.writeReg(2, val)

   # =============================================
   # Reset the lisa core
   # =============================================
   def reset(self):
      self.sio.write(f't')
      self.sio.flush()

   # =============================================
   # Get the lisa core PC value
   # =============================================
   def readPC(self):
      return self.readReg(2)

   # =============================================
   # Set the lisa core SP value
   # =============================================
   def writeSP(self, val:int):
      self.writeReg(3, val)

   # =============================================
   # Get the lisa core SP value
   # =============================================
   def readSP(self):
      return self.readReg(3)

   # =============================================
   # Read all opcodes from the HEX file
   # =============================================
   def readHexFile(self, name):
      with open(name, "r") as f:
         lines = [line.strip() for line in f if line.strip()]
      if not lines or not lines[0].startswith(':'):
         # lisa_as output: one 16-bit opcode in hex per line
         self.opcodes = [line.lower() for line in lines]
         return
      # Intel HEX (sdcc -mlisa): byte addresses, two bytes per word, low
      # byte first (as lisa_sim's loader); gaps are zero
      data = {}
      for line in lines:
         n, addr, rtype = int(line[1:3], 16), int(line[3:7], 16), int(line[7:9], 16)
         if rtype == 1:
            break
         if rtype == 0:
            for i in range(n):
               data[addr + i] = int(line[9 + 2 * i:11 + 2 * i], 16)
      words = (max(data) + 2) // 2 if data else 0
      self.opcodes = [f'{data.get(2 * w + 1, 0):02x}{data.get(2 * w, 0):02x}' for w in range(words)]

   # =============================================
   # Halt the lisa core
   # =============================================
   def halt(self):
      self.writeReg(0, 1)
      self.sio.write('v')
      self.sio.flush()

      # Halt will cause the debugger to send an 's' character
      # so we must read it from the channel
      for x in range(6):
        line = self.sio.readline().rstrip()
        if 'lisav' in line:
            break;
      
   # =============================================
   # Resume the lisa core
   # =============================================
   def resume(self):
      self.writeReg(0, 2)

   # =============================================
   # Single-step the lisa core
   # =============================================
   def step(self):
      self.writeReg(0, 5)

   # =============================================
   # Verify the program was loaded successfully
   # =============================================
   def verify(self):
      # First halt the core
      self.halt()

      print("verifying load ... ", end='', flush=True)

      # First set the FLASH base address
      self.setFlashBase()

      # Set the debug address to zero
      addr = 0
      for i in self.opcodes:
         self.sio.write(f'r20\n')
         self.sio.flush()
         line = self.sio.readline().rstrip()
         if line != i:
            print(f'Opcode mismatch at address {addr:4x}:  Expected {i} got {line}') 
            break
      else:
         print("passed")

   # =============================================
   # Read data SRAM.  Format of data is addr,len
   # =============================================
   def readData(self, data):
      v = data.split(',')
      try:
         a = int(v[0],16)
         len = int(v[1])

         # Set ram select
         self.sio.write('s1\n')
         self.sio.flush()

         # Set address
         self.sio.write(f'a{a:04x}\n')
         self.sio.flush()

         # Issue 'm'emory read request
         self.sio.write(f'm{len:04x}\n')
         self.sio.flush()

         # Read all data
         bytesOnLine = 0
         for l in range(len):
            if bytesOnLine == 0:
               print(f'{a:04X}: ', end='')

            # Read next byte
            d = int(self.sio.readline().rstrip(), 16)
            print(f'{d:02X} ', end='')
            bytesOnLine += 1
            if bytesOnLine == 16:
               bytesOnLine = 0
               a += 16
               print(' ')

            # Read the \r
            line = self.sio.readline().rstrip()

         if bytesOnLine != 8:
            print(' ')
      except:
         print(f'Format of --data is:')
         print(f'   --data=addr,len')

   # =============================================
   # Read data STACK. 
   # =============================================
   def readStack(self):
      self.halt()

      # Read the SP value
      sp = self.readSP()
      sp = sp & 0xFFF0

      # Set the Data bus access address
      self.writeReg(7, sp)

      # Read back the Debug address
      d = self.readReg(7)
      if (d != sp):
         print(f'Invalid Debug Addr: 0x{d:04X}')

      # Read all data
      bytesOnLine = 0
      len = 16
      for l in range(len):
         if bytesOnLine == 0:
            print(f'{sp+l:04X}: ', end='')

         # Read next byte
         d = self.readReg(6)
         print(f'{d:02X} ', end='', flush=True)
         bytesOnLine += 1
         if bytesOnLine == 16:
            bytesOnLine = 0
            print(' ')

         # Read the \r
         line = self.sio.readline().rstrip()

      if bytesOnLine != 8:
         print(' ')

      if d == 0xa5:
         self.resume()

      # Set debug addres to zero so it doesn't hold d_periph high
      self.writeReg(7, 0)

   # =============================================
   # Set the FLASH base address
   # =============================================
   def setFlashBase(self):
      # Set the debug interface base address
      msb = self.args.flashbase >> 16
      self.writeReg(0x11, msb)

      # Set the lisa1 (instruction bus) interface base address also
      msb = self.args.flashbase >> 8
      self.writeReg(0x12, msb)

      # Set the debug interface LSBs
      lsb = self.args.flashbase & 0xFFFF
      self.writeReg(0x10, lsb)

      # Configure the UIO interface for QUAD SPI
      self.sio.write(f'w1c0000\n')
      self.sio.flush()

      # Configure the UO interface for Lisa PortA output
      self.writeReg(0x1b, 0x5455)

   # =============================================
   # Read the flash status register
   # =============================================
   def readFlashStatus(self):
      # Read from debug address 0x22
      return self.readReg(0x22) & 0xFF

   # =============================================
   # Wait for FLASH ready
   # =============================================
   def awaitFlashReady(self):
      s = self.readFlashStatus()
      timeout = 400
      while s != 0 and timeout != 0:
         s = self.readFlashStatus()
         timeout -= 1

      # Return False if FLASH is not ready within timeout
      if timeout == 0:
         return False

      # Indicate FLASH ready
      return True

   # =============================================
   # Program all opcodes to the lisa core
   # =============================================
   def loadFlash(self):
      if self.iwidth != 16:
         return

      # Send the +++ escape in case LISA has the UART
      self.sio.write(f'+++')
      self.sio.flush()
      time.sleep(1)

      # First halt the core
      self.halt()

      # Ensure the PC is zero
      self.writePC(0)

      # Program the debug SPI interface base address
      self.setFlashBase()

      # Set the debug interface to single SPI FLASH mode
      self.writeReg(0x17, 4)

      # Configure SPI mode 1 and set inter-CS delay to 0xd
      self.writeReg(0x1e, 0x08d3)

      # Set the WREN bit
      self.writeReg(0x21, 6)
      
      # Perform a sector erase
      print('Erasing FLASH sector...')
      self.writeReg(0x21, 0x01d8)
      
      if not self.awaitFlashReady():
         print('The FLASH erase did not report complete!!')
         return

      print('Programming FLASH...')

      # Now write all opcodes to the core
      for i in self.opcodes:
         self.sio.write(f'w20{i}\n')
         self.sio.flush()
         self.awaitFlashReady()

      # Ensure the PC is zero and SP is MAX
      self.writePC(0)

   # =============================================
   # Program all opcodes to the lisa core
   # =============================================
   def loadSram(self):
      if self.iwidth != 16:
         return

      # Send the +++ escape in case LISA has the UART
      self.sio.write(f'+++')
      self.sio.flush()
      time.sleep(1)

      # First halt the core
      self.halt()

      # Ensure the PC is zero
      self.writePC(0)

      # Program the debug SPI interface base address
      self.setFlashBase()

      # Set the debug interface to single SPI SRAM 16-bit mode
      self.writeReg(0x17, 0x10)

      # Configure SPI mode 1 and set inter-CS delay to 0xd
      self.writeReg(0x1e, 0x08d3)
      
      print('Programming SRAM...')

      # Now write all opcodes to the core
      for i in self.opcodes:
         self.sio.write(f'w20{i}\n')
         self.sio.flush()

      # Ensure the PC is zero and SP is MAX
      self.writePC(0)

   def noCache(self):
      # Send the +++ escape in case LISA has the UART
      print("Enabling no-cache SRAM mode")
      self.sio.write(f'+++')
      self.sio.flush()
      time.sleep(1)

      self.sio.write(f'\n')
      self.sio.flush()

      # Set the SRAM to non-cache mode
      self.writeReg(0x1d, 7)

# =========================================================
# Parse the input arguments
# =========================================================
def parseArgs():
  parser = argparse.ArgumentParser(
      description = 'LISA core debugger')
  parser.add_argument("input_file",
      type=str,
      default=None,
      help="Input HEX file")
  parser.add_argument("--init",
      dest = 'init',
      default = False,
      action = 'store_true',
      help='Test for Python REPL prompt and do project initialization.')
  parser.add_argument("--flash",
      dest = 'flash',
      default = False,
      action = 'store_true',
      help='Load the firmware to the SPI Flash.')
  parser.add_argument("--sram",
      dest = 'sram',
      default = False,
      action = 'store_true',
      help='Load the firmware to the SPI SRAM.')
  parser.add_argument("--verify",
      dest = 'verify',
      default = False,
      action = 'store_true',
      help='Verify the loaded firmware.')
  parser.add_argument("--run",
      dest = 'run',
      default = False,
      action = 'store_true',
      help='Run the core.')
  parser.add_argument("--resume",
      dest = 'resume',
      default = False,
      action = 'store_true',
      help='Resume the core.')
  parser.add_argument("--pc",
      dest = 'pc',
      default = False,
      action = 'store_true',
      help='Display the PC value.')
  parser.add_argument("--sp",
      dest = 'sp',
      default = False,
      action = 'store_true',
      help='Display the SP value.')
  parser.add_argument("--halt",
      dest = 'halt',
      default = False,
      action = 'store_true',
      help='Halt the processor.')
  parser.add_argument("--nocache",
      dest = 'nocache',
      default = False,
      action = 'store_true',
      help='Set SRAM for non-cache 128 byte scratchpad access.')
  parser.add_argument("-V",
      dest = 'version',
      default = False,
      action = 'store_true',
      help='Display the debugger version.')
  parser.add_argument("--baud",
      dest = 'baud',
      default = 460800,
      type = int,
      help='Serial baud rate: 460800 for the demo board (its RP2040 re-times to the chip), 115200 straight to the chip\'s debug UART at 50 MHz, 125000 on the 40 MHz Arty build (the autobaud measures in 32-clock steps).')
  parser.add_argument("--port",
      dest = 'port',
      type = str,
      default = "/dev/ttyUSB0",
      help='Serial port of the device.')
  parser.add_argument("--data",
      dest = 'data',
      type = str,
      default = "",
      help='Read data at given address,length.')
  parser.add_argument("--iwidth",
      dest = 'iwidth',
      type = int,
      default = 16,
      help='Instruction width.')
  parser.add_argument("--stack",
      dest = 'stack',
      default = False,
      action = 'store_true',
      help='Read data at STACK.')
  parser.add_argument("--reset",
      dest = 'reset',
      default = False,
      action = 'store_true',
      help='Reset the processor.')
  parser.add_argument("--step",
      dest = 'step',
      default = False,
      action = 'store_true',
      help='Single step the processor.')
  parser.add_argument("--flashbase",
      dest = 'flashbase',
      default = 0x000000,
      type = int,
      help='Base address of ASM code in SPI Flash.')

  args = parser.parse_args()
  return args

# =========================================================
# The main program entry
# =========================================================
def main():
   # Parse the input arguments
   args = parseArgs()

   # Create the debug object
   db = LisaPyDb()
   db.iwidth = args.iwidth
   db.openPort(args.port, args.baud)
   db.args = args

   # Send \n and test for Python REPL.  If found, then
   # use the tt SDK to initialize the lisa project
   if args.init:
      db.init()

   # Get the version.  This is a way to ensure the FSM
   # is in a good state.  We get the Ver even if there was
   # no request to display it.
   if args.version:
      db.ver = db.getVer()
      print(f'Version: {db.ver}')

   # Read the HEX input file
   if args.verify or args.flash or args.sram:
      db.readHexFile(args.input_file)

   # Test for nocache SRAM mode
   if args.nocache:
      db.noCache()

   # Load the HEX input file to SPI flash
   if args.flash:
      print(f'Loading {len(db.opcodes)} opcodes')
      db.loadFlash()

   # Load the HEX input file to SPI SRAM
   if args.sram:
      print(f'Loading {len(db.opcodes)} opcodes')
      db.loadSram()

   # Test if verify requested
   if args.verify:
      db.verify()

   # Test if run requested
   if args.run:
      db.writePC(0)
      db.resume()

   # Test if resume requested
   if args.resume:
      db.resume()

   # Test if resuesting print of PC
   if args.pc:
      pc = db.readPC()
      print(f'PC: {pc:4X}')

   # Test if resuesting print of SP
   if args.sp:
      sp = db.readSP()
      print(f'SP: {sp:4X}')

   # Test if resuesting halt
   if args.halt:
      db.halt()

   # Test for request for data SRAM dump
   if args.data != "":
      db.readData(args.data)

   # Test for request for data STACK dump
   if args.stack:
      db.readStack()

   # Test for request for reset
   if args.reset:
      db.reset()

   # Test for step request
   if args.step:
      db.step()

if __name__ == '__main__':
  main()

