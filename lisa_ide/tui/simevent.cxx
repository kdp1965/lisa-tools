/*
========================================================================
simevent.cpp:	Class implementation for OS independant notification event.

 Copyright 2011-2015 Ken Pettit
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

========================================================================
*/

#include	"simevent.h"

CSimEvent::CSimEvent()
{
}

CSimEvent::~CSimEvent()
{
}

/*
================================================================================
Open:	This routine opens and initializes the event.
================================================================================
*/
int CSimEvent::Open(void)
{
	sem_init(&m_Event, 0, 0);
	return 0;
}

/*
================================================================================
Closes:	This routine closes / de-initializes the event.
================================================================================
*/
void CSimEvent::Close(void)
{
}

/*
================================================================================
Trigger:	This routine triggers the event and wakes up any waiters.
================================================================================
*/
void CSimEvent::Trigger(void)
{
	sem_post(&m_Event);
}

/*
================================================================================
Wait:	This routine waits forever on the event.
================================================================================
*/
void CSimEvent::Wait(void)
{
	sem_wait(&m_Event);
}

/*
================================================================================
WaitTimeout:	This routine waits forever on the event.
================================================================================
*/
int CSimEvent::TryWait(void)
{
	return sem_trywait(&m_Event);
}

/*
================================================================================
Release:	This routine releases the semaphore
================================================================================
*/
void CSimEvent::Release(void)
{
	sem_post(&m_Event);
}

/*
================================================================================
Acquire:	This routine waits forever on the semaphore.
================================================================================
*/
void CSimEvent::Acquire(void)
{
	sem_wait(&m_Event);
}


