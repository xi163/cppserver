#pragma once

#include "../Macro.h"

namespace Operation {

	enum class Mode { M_READ, M_WRITE, M_APPEND };

	class IOperation {
	public:
		IOperation() {}
		virtual ~IOperation() {}
		virtual char const* Path() = 0;
		virtual bool Valid() = 0;
		virtual bool IsFile() = 0;
		virtual void ClearErr() {}
		//int fclose ( FILE * stream );
		virtual bool Close() = 0;
		//int feof ( FILE * stream );
		virtual int Eof() = 0;
		// int ferror ( FILE * stream );
		virtual int Error() { return 0; }
		//int fflush ( FILE * stream );
		virtual int Flush() { return 0; }
		//int fgetc ( FILE * stream );
		virtual int Getc() = 0;
		//int fgetpos ( FILE * stream, fpos_t * pos );
		virtual int GetPos(fpos_t* pos) = 0;
		//char * fgets ( char * str, int num, FILE * stream );
		virtual char* Gets(char* str, int num) = 0;
		//FILE * fopen ( const char * filename, const char * mode );
		virtual bool Open(Mode mode = Mode::M_READ) = 0;
		// int fputc ( int character, FILE * stream );
		virtual int Putc(int character) = 0;
		// int fputs ( const char * str, FILE * stream );
		virtual int Puts(char const* str) = 0;
		//size_t fread ( void * ptr, size_t size, size_t count, FILE * stream );
		virtual size_t Read(void* ptr, size_t size, size_t count) = 0;
		//int fseek ( FILE * stream, long int offset, int origin );
		virtual int Seek(long offset, int origin) = 0;
		//int fsetpos ( FILE * stream, const fpos_t * pos );
		virtual int Setpos(const fpos_t* pos) = 0;
		//long int ftell ( FILE * stream );	
		virtual long Tell() = 0;
		//size_t fwrite ( const void * ptr, size_t size, size_t count, FILE * stream );
		virtual size_t Write(void const* ptr, size_t size, size_t count) = 0;
		//void rewind ( FILE * stream );
		virtual void Rewind() = 0;
		virtual void Buffer(char* buffer, size_t size) = 0;
		virtual void Buffer(std::string& s) = 0;
		virtual void Buffer(std::vector<char>& buffer) = 0;
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
	};
}