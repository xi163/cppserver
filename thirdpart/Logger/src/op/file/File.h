#ifndef INCLUDE_FILE_H
#define INCLUDE_FILE_H

#include "../../Macro.h"
#include "../IOperation.h"

namespace Operation {
	class FileImpl;
	class CFile : public IOperation {
	public:
		explicit CFile(char const* path);
		~CFile();
		virtual char const* Path();
		virtual bool Valid();
		virtual bool IsFile();
		virtual void ClearErr();
		//int fclose ( FILE * stream );
		virtual bool Close();
		//int feof ( FILE * stream );
		virtual int Eof();
		// int ferror ( FILE * stream );
		virtual int Error();
		//int fflush ( FILE * stream );
		virtual int Flush();
		//int fgetc ( FILE * stream );
		virtual int Getc();
		//int fgetpos ( FILE * stream, fpos_t * pos );
		virtual int GetPos(fpos_t* pos);
		//char * fgets ( char * str, int num, FILE * stream );
		virtual char* Gets(char* str, int num);
		//FILE * fopen ( const char * filename, const char * mode );
		virtual bool Open(Mode mode = Mode::M_READ);
		// int fputc ( int character, FILE * stream );
		virtual int Putc(int character);
		// int fputs ( const char * str, FILE * stream );
		virtual int Puts(char const* str);
		//size_t fread ( void * ptr, size_t size, size_t count, FILE * stream );
		virtual size_t Read(void* ptr, size_t size, size_t count);
		//int fseek ( FILE * stream, long int offset, int origin );
		virtual int Seek(long offset, int origin);
		//int fsetpos ( FILE * stream, const fpos_t * pos );
		virtual int Setpos(const fpos_t* pos);
		//long int ftell ( FILE * stream );	
		virtual long Tell();
		//size_t fwrite ( const void * ptr, size_t size, size_t count, FILE * stream );
		virtual size_t Write(void const* ptr, size_t size, size_t count);
		//void rewind ( FILE * stream );
		virtual void Rewind();
		virtual void Buffer(char* buffer, size_t size);
		virtual void Buffer(std::string& s);
		virtual void Buffer(std::vector<char>& buffer);
		/* UnSuport
		int putc ( int character, FILE * stream ); use Putc
		int getc ( FILE * stream ); use Getc
		FILE * freopen ( const char * filename, const char * mode, FILE * stream );
		void setbuf ( FILE * stream, char * buffer );
		int setvbuf ( FILE * stream, char * buffer, int mode, size_t size );
		int fprintf ( FILE * stream, const char * format, ... );
		int fscanf ( FILE * stream, const char * format, ... );
		int ungetc ( int character, FILE * stream );
		int vfprintf ( FILE * stream, const char * format, va_list arg );
		int vfscanf ( FILE * stream, const char * format, va_list arg );
		*/
	private:
		template <class T> static inline T* New() {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T();
		}
		template <class T> static inline T* New(char const* path) {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T(path);
		}
		template <class T> static inline void Delete(T* ptr) {
			ptr->~T();
			free(ptr);
		}
	private:
		FileImpl* impl_;
	};
}

#endif