/****************************************************************************
 *   Copyright (C) 2018 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <kpettit@iqanalog.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../tui/Tui.h"
#include "../tui/simevent.h"
#include "Lisa.h"

int   gExitApp = 0;
int   gTelnetPort = 1616;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * merlin_main
 ****************************************************************************/

extern "C"
{
  int main(int argc, char *argv[])
  {
//    CRemoteServer telnet;
    int           c;

    /* Process the arguments */
    while ((c = getopt(argc, argv, "p:")) != -1)
    {
      switch (c)
      {
        // Set the Debugger telnet port
        case 'p':
          gTelnetPort = atoi(optarg);
          break;
      }
    }

    // If C++ initialization for static constructors is supported, then do
    // that first

    /* Now create the Tui */
  
    CTui *pLisaTui = new CTui;
    if (pLisaTui == NULL)
    {
      printf("Out of memory\n");
      return -1;
    }

    CLisa *pLisa = new CLisa(pLisaTui);
    if (pLisa == NULL)
    {
      printf("Out of memory\n");
      return -1;
    }

    pLisaTui->m_pPrompt = "\\c3lisa> \\c0";
    pLisaTui->AttachTuiSource(pLisa);
  
    /* Initialize the telnet server on port 1616 */
//    telnet.Initialize(pLisa, gTelnetPort, 1);

    /* Initialize the TUI screen and run the main loop */
    pLisaTui->RunThread();

    delete pLisaTui;
    delete pLisa;
    return OK;
  }
}

// vim: sw=2 ts=2 et
