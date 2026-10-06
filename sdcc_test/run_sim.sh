#!/bin/sh
# run_sim.sh <prog.ihx> [cycles] - run a program on lisa_sim in batch mode and
# print only what it wrote to UART1
SIM=${SIM:-$(dirname "$0")/../lisa_sim/lisa_sim}
"$SIM" -b -c "${2:-2000000}" "$1" 2>&1 | awk '
  NR <= 3 { next }                      # banner + "Loaded N words"
  /^Execution complete:/ { done = 1 }
  done { next }
  { print }' | sed -e :a -e '/^\n*$/{$d;N;ba' -e '}'
