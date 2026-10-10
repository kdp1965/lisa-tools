/****************************************************************************
 * f1000/tui/tui.c
 *
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

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <system/termcurses.h>
#include <graphics/curses.h>
#include <fcntl.h>

#include "iqsh.h"
#include "graphics/curses.h"

/****************************************************************************
 * Local Global Variables
 ****************************************************************************/

static int tui_active;

/****************************************************************************
 * Local Prototypes
 ****************************************************************************/

int tuicmd_help(int argc, char *argv[]);
int tuicmd_exit(int argc, char *argv[]);
int tuicmd_test(int argc, char *argv[]);

/****************************************************************************
 * Array of tui command pointers.
 ****************************************************************************/

static const iqcmd_t  g_Cmds[] = 
{
   // Name      min, max, function,             usage text              help text
   { "help",      0,   0, tuicmd_help,         "", "Help"},
   { "q",         0,   0, tuicmd_exit,         "", "Exit pl"},
   { "quit",      0,   0, tuicmd_exit,         "", "Exit pl"},
   { "test",      0,   0, tuicmd_test,         "", "Do a test read"},

   // The terminating entry
   { NULL, 0, 0, NULL, NULL, NULL }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int readline(FAR char *buffer, int len, FILE *in, FILE *out);

/****************************************************************************
 * Help 
 ****************************************************************************/

int tuicmd_help(int argc, char *argv[])
{
   int      x;
   int      y, len;

   // Test if help on a specific command is requested
   if (argc == 1)
   {
      /* General help.  Show all commands */

      printf("\r\nSupported commands:\r\n    ");
      
      for (x = 0, y = 0; g_Cmds[x].name != NULL; x++)
      {
         printf(g_Cmds[x].name);
         len = strlen(g_Cmds[x].name);
         while (len != 12)
         {
            printf(" ");
            len++;
         }

         if (y++ == 3)
         {
            y = 0;
            printf("\r\n    ");
         }
      }

      printf("\r\n\r");
   }
   else
   {
      // Find the specific command
      for (x = 0, y = 0; g_Cmds[x].name != NULL; x++)
      {
         // Test if it is this command
         if (strcmp(g_Cmds[x].name, argv[1]) == 0)
         {
            // Print this commands help text
            printf("\r\n\n%s\r\n\nUSAGE:\r\n  %s   %s\r\n\n", 
                  g_Cmds[x].help, g_Cmds[x].name, g_Cmds[x].usage);

            return 0;
         }
      }

      printf("\r\rUnknown command %s\r\n", argv[1]);
   }

  return 0;
}

/****************************************************************************
 * tuicmd_exit
 ****************************************************************************/

int tuicmd_exit(int argc, char *argv[])
{
  tui_active = 0;
  return 0;
}

/****************************************************************************
 * tuicmd_test
 ****************************************************************************/

int tuicmd_test(int argc, char *argv[])
{
  int     ch;
  int     ret;
  struct  winsize ws;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  //ret = ioctl(0, TCURSIOCINIT, NULL);

  /* Initialize Curses */

  initscr();

  nl();
  noecho();
  timeout(0);
  keypad(stdscr, true);

  ret = ioctl(0, TIOCGWINSZ, (unsigned long) &ws);
  if (ret == OK)
    printf("Screen: %d x %d\n", ws.ws_col, ws.ws_row);

  ch = 0;
  while (ch != 'q')
    {
      ch = getch();
      if (ch != -1 && ch != 0)
        {
          if (ch == 0x0a)
            printf("\n");
          else
            {
              if (ch >= 33 && ch < '~')
                printf("%c", ch);
              else
                printf("<0x%02X>", ch);
            }
        }
    }

  endwin();

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * tui_main
 ****************************************************************************/

#if defined(BUILD_MODULE)
int main(int argc, FAR char *argv[])
#else
int tui_main(int argc, char *argv[])
#endif
{
  char     buffer[256];

  // Print prompt on first entry or re-entry from longjmp exception
  printf("\r\ntui> ");
  fflush(stdout);
  tui_active = 1;

  while (tui_active)
    {
      /* Get next RX byte */

      readline(buffer, sizeof(buffer), stdin, stdout);

      while (buffer[strlen(buffer)-1] == '\x0D' || 
             buffer[strlen(buffer)-1] == '\x0A')
        {
          buffer[strlen(buffer)-1] = 0;
        }

      iqcmd_cmdexec((uint8_t *) buffer, g_Cmds);

      if (tui_active)
        {
          printf("tui> ");
          fflush(stdout);
        }
    }

  return OK;
}

// vim: sw=2 ts=2 et
