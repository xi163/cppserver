#include "FileImpl.h"
#include "../../../log/impl/LoggerImpl.h"

namespace Operation {

	FileImpl::FileImpl(char const* path)
		: stream_(NULL)
		, path_(path) {
		Open();
	}

	FileImpl::~FileImpl() {
		Close();
	}
	
	char const* FileImpl::Path() {
		return path_.c_str();
	}

	bool FileImpl::Valid() {
		return stream_ != NULL;
	}

	bool FileImpl::IsFile() {
		return true;
	}

	void FileImpl::ClearErr() {
	}

	bool FileImpl::Close() {
		if (stream_) {
			fclose(stream_);
			stream_ = NULL;
		}
		return true;
	}

	int FileImpl::Eof() {
		if (stream_) {
			//http://c.biancheng.net/cpp/html/2514.html
			return feof(stream_);
		}
		return 0x0010;
	}

	int FileImpl::Error() {
		if (stream_) {
			return ferror(stream_);
		}
		return 0x0020;
	}

	int FileImpl::Flush() {
		if (stream_) {
			return fflush(stream_);
		}
		return -1;
	}

	int FileImpl::Getc() {
		if (stream_) {
			return fgetc(stream_);
		}
		return -1;
	}

	int FileImpl::GetPos(fpos_t* pos) {
		if (stream_ && pos) {
			return fgetpos(stream_, pos);
		}
		return -1;
	}

	char* FileImpl::Gets(char* str, int num) {
		if (stream_ && str) {
			return fgets(str, num, stream_);
		}
		return 0;
	}

	bool FileImpl::Open(Mode mode) {
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif
		Close();
		switch (mode) {
		case Mode::M_WRITE: {
			stream_ = fopen(path_.c_str(), "wb+");
			//if (!stream_) {
			//	_Errorf_pure("文件可能被占用，写方式打开失败");
			//}
			return stream_ != NULL;
		}
		case Mode::M_APPEND: {
			stream_ = fopen(path_.c_str(), "ab+");
			//if (!stream_) {
			//	_Errorf_pure("文件可能被占用，写方式打开失败");
			//}
			return stream_ != NULL;
		}
		default: {
			stream_ = fopen(path_.c_str(), "rb");
		}
		}
#ifdef WIN32
#pragma warning(pop)
#endif
		if (0 == stream_) {
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif
			stream_ = fopen(path_.c_str(), "wb+");
#ifdef WIN32
#pragma warning(pop)
#endif
		}
		return stream_ != NULL;
	}

	int FileImpl::Putc(int character) {
		if (stream_) {
			return fputc(character, stream_);
		}
		return EOF;
	}

	int FileImpl::Puts(char const* str) {
		if (stream_) {
			return fputs(str, stream_);
		}
		return EOF;
	}

	size_t FileImpl::Read(void* ptr, size_t size, size_t count) {
		if (stream_) {
			//fread(ptr, 1, length, stream_);
			return fread(ptr, size, count, stream_);
		}
		return 0;
	}

	int FileImpl::Seek(long offset, int origin) {
		if (stream_) {
			//SEEK_SET/SEEK_CUR/SEEK_END
			return fseek(stream_, offset, origin);
		}
		return EOF;
	}

	int FileImpl::Setpos(const fpos_t* pos) {
		if (stream_ && pos) {
			return fsetpos(stream_, pos);
		}
		return EOF;
	}

	long FileImpl::Tell() {
		if (stream_) {
			//fseek(stream_, 0L, SEEK_END);
			return ftell(stream_);
		}
		return EOF;
	}

	size_t FileImpl::Write(void const* ptr, size_t size, size_t count) {
		if (stream_) {
			size_t n = fwrite(ptr, size, count, stream_);
			if (n != 0) {
				return n;
			}
			//写失败，可能读方式打开
#ifdef WIN32
#else
			int nerror = ferror(stream_);
			if (0x20 != nerror) {
				return n;
			}
#endif
			long lPos = ftell(stream_);
			if (0 == lPos) {
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif
				//写方式重新打开
				stream_ = freopen(path_.c_str(), "wb+", stream_);
#ifdef WIN32
#pragma warning(pop)
#endif
				//if (!stream_) {
				//	_Errorf_pure("文件可能被占用，写方式打开失败");
				//}
				return stream_ ? fwrite(ptr, size, count, stream_) : 0;
			}
		}
		return 0;
	}

	void FileImpl::Rewind() {
		if (stream_) {
			rewind(stream_);
		}
	}

	void FileImpl::Buffer(char* buffer, size_t size) {
	}

	void FileImpl::Buffer(std::string& s) {
	}

	void FileImpl::Buffer(std::vector<char>& buffer) {
		buffer.clear();
		if (stream_) {
			fseek(stream_, 0L, SEEK_END);
			size_t length = ftell(stream_);
			if (length > 0) {
				buffer.resize(length);
				fseek(stream_, 0L, SEEK_SET);
				size_t n = fread(&buffer.front(), 1, length, stream_);
				(void)n;
			}
		}
	}
}