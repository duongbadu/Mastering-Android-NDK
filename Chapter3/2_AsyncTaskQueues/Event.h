/*
 * Copyright (C) 2013-2015 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013-2015 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <vector>
#include <array>

#include "tinythread.h"
#include "IntrusivePtr.h"

class iAsyncCapsule: public iIntrusiveCounter
{
public:
	virtual void Invoke() = 0;
};

class clAsyncQueue: public iIntrusiveCounter
{
public:
	clAsyncQueue()
		: FDemultiplexerMutex()
		, FCurrentQueue( 0 )
		, FAsyncQueues()
		, FAsyncQueue( &FAsyncQueues[0] )
	{ }

	/// Put the event into the events queue
	virtual void EnqueueCapsule( const clPtr<iAsyncCapsule>& Capsule )
	{
		tthread::lock_guard<tthread::mutex> Lock( FDemultiplexerMutex );
		FAsyncQueue->push_back( Capsule );
	}

	/// Events demultiplexer as described in the Reactor pattern
	virtual void DemultiplexEvents()
	{
		CallQueue& LocalQueue = FAsyncQueues[ FCurrentQueue ];

		// switch current queue
		{
			tthread::lock_guard<tthread::mutex> Lock( FDemultiplexerMutex );

			FCurrentQueue = ( FCurrentQueue + 1 ) % 2;
			FAsyncQueue = &FAsyncQueues[ FCurrentQueue ];
		}

		// invoke callbacks
		for ( auto& i : LocalQueue ) { i->Invoke(); }

		LocalQueue.clear();
	}

private:
	using CallQueue = std::vector< clPtr<iAsyncCapsule> >;

	size_t                   FCurrentQueue;
	std::array<CallQueue, 2> FAsyncQueues;

	/// switched for shared non-locked access
	CallQueue*     FAsyncQueue;
	tthread::mutex FDemultiplexerMutex;
};
