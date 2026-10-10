/*
========================================================================
thread.cpp	Class implementation for OS independant threads.

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

#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "thread.h"

/*
================================================================================
thread_entry:	This is the entry point for the thread.
================================================================================
*/
static void *thread_entry(void* pParams)
{
	CThread* pThread = (CThread *) pParams;

	// Run the simulation
	pThread->RunThread();

	return 0;
}

/*
================================================================================
Class constructor
================================================================================
*/
CThread::CThread()
{
}

/*
================================================================================
Class destructor
================================================================================
*/
CThread::~CThread()
{
}

/*
================================================================================
Start:	This routine starts the thread
================================================================================
*/
void CThread::Start(void)
{
	// Create the simulation thread
	pthread_create(&m_thread, NULL, thread_entry, this);
}

/*
================================================================================
StopThread:	This routine stops the thread
================================================================================
*/
void CThread::StopThread(void)
{
}

