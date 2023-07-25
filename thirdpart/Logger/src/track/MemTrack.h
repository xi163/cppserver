/*
Copyright (c) 2002, 2008 Curtis Bartley
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the
distribution.
- Neither the name of Curtis Bartley nor the names of any other
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef INCLUDE_MEMTRACK_H
#define INCLUDE_MEMTRACK_H

//#define _MEMORY_TRACK_
//#define _DUMPTOLOG
//#define NO_PLACEMENT_NEW

#include "Logger/src/Macro.h"
#include <typeinfo>

#ifdef _windows_
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

namespace utils {

	namespace MemTrack {

		class MemStamp {
		public:
			MemStamp(char const* file, int line, char const* func);
			~MemStamp();
		public:
			char const* const file;
			int const line;
			char const* const func;
		};

		void initialize();
		void* trackMalloc(size_t size, char const* func);
		void trackFree(void* ptr, char const* func);
		void trackStamp(void* ptr, const MemStamp& stamp, char const* typeName);
		void trackDumpBlocks();
		void trackMemoryUsage();
		void cleanup();

		template <class T> inline T* operator*(const MemStamp& stamp, T* ptr) {
			trackStamp(ptr, stamp, typeid(T).name());
			return ptr;
		}
	}
}

#ifdef _MEMORY_TRACK_

#ifdef NO_PLACEMENT_NEW
#define MEMTRACK_NEW utils::MemTrack::MemStamp(__FILE__, __LINE__, __FUNC__) * new(__FILE__, __LINE__, __FUNC__)
#else
#define MEMTRACK_NEW utils::MemTrack::MemStamp(__FILE__, __LINE__, __FUNC__) * new
#endif

#define new MEMTRACK_NEW

#endif

#endif