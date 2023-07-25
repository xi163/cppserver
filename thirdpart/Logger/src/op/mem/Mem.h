#ifndef INCLUDE_MEM_H
#define INCLUDE_MEM_H

#include "Logger/src/Macro.h"
#include "Logger/src/op/IOperation.h"

namespace Operation {
	class MemImpl;
	class CMemory : public IOperation {
	public:
		CMemory();
		~CMemory();
	public:
		virtual char const* Path();
		virtual bool Valid();
		virtual bool IsFile();
		virtual bool Close();
		virtual int Eof();
		virtual int Getc();
		virtual int GetPos(fpos_t* pos);
		virtual char* Gets(char* str, int num);
		virtual bool Open(Mode mode = Mode::M_READ);
		virtual int Putc(int character);
		virtual int Puts(char const* str);
		virtual size_t Read(void* ptr, size_t size, size_t count);
		virtual int Seek(long offset, int origin);
		virtual int Setpos(const fpos_t* pos);
		virtual long Tell();
		virtual size_t Write(void const* ptr, size_t size, size_t count);
		virtual void Rewind();
		virtual void Buffer(char* buffer, size_t size);
		virtual void Buffer(std::string& s);
		virtual void Buffer(std::vector<char>& buffer);
		CMemory(void* buffer, unsigned long length);
		//bool Open();
		//bool Read(void* lpBuffer, unsigned long ulNumberOfBytesToRead, unsigned long* lpNumberOfBytesRead);
		//bool Write(void const* lpBuffer, unsigned long ulNumberOfBytesToWrite, unsigned long* lpNumberOfBytesWritten);
		//unsigned long Seek( long offset, int origin );
		//bool Close();
		//bool IsEmpty();
	private:
		template <class T> static inline T* New() {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T();
		}
		template <class T> static inline T* New(void* buffer, unsigned long length) {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T(buffer, length);
		}
		template <class T> static inline void Delete(T* ptr) {
			ptr->~T();
			free(ptr);
		}
	private:
		MemImpl* impl_;
	};
}

#endif