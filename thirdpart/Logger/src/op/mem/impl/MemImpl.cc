#include "MemImpl.h"
#include "../../../log/impl/LoggerImpl.h"

namespace Operation {

	MemImpl::MemImpl() {
		pos_ = 0;
		buffer_.clear();
	}

	MemImpl::~MemImpl() {
	}

	char const* MemImpl::Path() {
		return NULL;
	}

	bool MemImpl::Valid() {
		return buffer_.size() > 0;
	}

	bool MemImpl::IsFile() {
		return false;
	}

	bool MemImpl::Close() {
		return true;
	}

	int MemImpl::Eof() {
		return pos_ == buffer_.size() ? -1 : 0;
	}

	int MemImpl::Getc() {
		int nget = 0;
		if (0 == Read(&nget, sizeof(char), 1)) {
			return EOF;
		}
		return nget;
	}

	int MemImpl::GetPos(fpos_t* pos) {
#if WIN32
		* pos = pos_;
#else
		pos->__pos = pos_;
#endif
		return 0;
	}

	char* MemImpl::Gets(char* str, int num) {
		char* szReadBuffer = 0;
		do {
			if (!str || pos_ >= buffer_.size()) {
				break;
			}

			size_t unReaded = 0;
			if (pos_ + num <= buffer_.size()) {
				unReaded = num;
			}
			else {
				unReaded = buffer_.size() - pos_;
			}

			if (0 == unReaded) {
				break;
			}

			size_t index = 0;
			szReadBuffer = str;
			for (; index < unReaded; ++index) {
				memcpy(szReadBuffer + index, buffer_.data() + pos_ + index, 1);
				if ('\n' == *(buffer_.data() + pos_ + index)) {
					break;
				}
			}

			pos_ += index;
		} while (0);

		return szReadBuffer;
	}

	bool MemImpl::Open(Mode mode) {
		pos_ = 0;
		return true;
	}

	int MemImpl::Putc(int character) {
		size_t unCount = sizeof(character);
		Write(&character, sizeof(char), unCount);
		return 0;
	}

	int MemImpl::Puts(char const* str) {
		size_t unCount = strlen(str);
		Write(str, sizeof(char), unCount);
		return 0;
	}

	size_t MemImpl::Read(void* ptr, size_t size, size_t count) {
		unsigned int unReaded = 0;
		do {
			if (!ptr || pos_ >= buffer_.size()) {
				break;
			}

			unsigned int unNeedRead = size * count;
			if (pos_ + unNeedRead <= buffer_.size()) {
				unReaded = unNeedRead;
			}
			else {
				unReaded = buffer_.size() - pos_;
			}

			if (0 == unReaded) {
				break;
			}

			memcpy(ptr, buffer_.data() + pos_, unReaded);
			pos_ += unReaded;
		} while (0);

		return unReaded;
	}

	int MemImpl::Seek(long offset, int origin) {
		int nRet = EOF;
		if (offset >= 0) {
			if (SEEK_CUR == origin) {
				pos_ = pos_ + offset;
				nRet = 0;
			}
			else if (SEEK_SET == origin) {
				pos_ = offset;
				nRet = 0;
			}
			else if (SEEK_END == origin) {
				pos_ = buffer_.size() + offset;
				nRet = 0;
			}
		}
		else {
			if (SEEK_CUR == origin) {
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4018)
#endif
				if (pos_ > abs(offset)) {
					pos_ = pos_ - abs(offset);
					nRet = 0;
				}
#ifdef WIN32				
#pragma warning(pop) 
#endif
			}
			else if (SEEK_CUR == origin || SEEK_END == origin) {
#ifdef WIN32				
#pragma warning(push)
#pragma warning(disable:4018)
#endif
				if (buffer_.size() > abs(offset)) {
					pos_ = buffer_.size() - abs(offset);
					nRet = 0;
				}
				else {
					pos_ = 0;
				}
#ifdef WIN32				
#pragma warning(pop) 
#endif
			}
		}
		return nRet;

	}

	int MemImpl::Setpos(const fpos_t* pos) {
#ifdef WIN32			
#pragma warning(push)
#pragma warning(disable:4244)
#endif

#ifdef WIN32
		pos_ = *pos;
#else
		pos_ = pos->__pos;
#endif

#ifdef WIN32			
#pragma warning(pop) 
#endif
		return 0;
	}

	long MemImpl::Tell() {
		return pos_;
	}

	size_t MemImpl::Write(void const* ptr, size_t size, size_t count) {
		size_t unWritten = 0;
		do {
			size_t unNeedWrite = size * count;
			if (0 == unNeedWrite) {
				break;
			}

			size_t unRealPos = pos_;
			if (unRealPos > buffer_.size()) {
				unRealPos = buffer_.size();
			}

			size_t unOriCount = buffer_.size();
			if (unRealPos + unNeedWrite > unOriCount) {
				buffer_.resize(unRealPos + unNeedWrite);
			}

			void* pDest = (void*)memcpy(buffer_.data() + unRealPos, (char*)ptr, unNeedWrite);
			if ((void*)-1 == pDest) {
				buffer_.resize(unOriCount);
			}
			else {
				unWritten = unNeedWrite;
				unRealPos += unWritten;
				pos_ = unRealPos;
			}
		} while (0);
		return unWritten;
	}

	void MemImpl::Rewind() {
		pos_ = 0;
	}

	void MemImpl::Buffer(char* buffer, size_t size) {
		memset(buffer, 0, size);
		memcpy(buffer, buffer_.data(), Tell());
	}

	void MemImpl::Buffer(std::string& s) {
		s.clear();
		s.append((char*)buffer_.data(), (long)Tell());
	}

	void MemImpl::Buffer(std::vector<char>& buffer) {
	}

	MemImpl::MemImpl(void* buffer, unsigned long length) : pos_(0) {
		buffer_.resize(length);
		memcpy(buffer_.data(), buffer, length);
	}
// 
// 
// 	bool MemImpl::Open() {
// 		return true;
// 	}
// 
// 	bool MemImpl::Read( void* lpBuffer, unsigned long ulNumberOfBytesToRead, unsigned long* lpNumberOfBytesRead ) {
// 		bool bRet = false;
// 		unsigned long dwReaded;
// 
// 		do {
// 			if ( !lpBuffer || !ulNumberOfBytesToRead ) {
// 				break;
// 			}
// 
// 			if ( pos_ + ulNumberOfBytesToRead <= buffer_.GetCount() ) {
// 				dwReaded = ulNumberOfBytesToRead;
// 			}
// 			else {
// 				dwReaded = (unsigned long)buffer_.GetCount() - pos_;
// 			}
// 
// 			if ( 0 == dwReaded ) {
// 				break;
// 			}
// 
// 			memcpy(lpBuffer, buffer_.GetData() + pos_, dwReaded);
// 			pos_ += dwReaded;
// 
// 			if ( lpNumberOfBytesRead ) {
// 				*lpNumberOfBytesRead = dwReaded;
// 			}
// 
// 			bRet = true;
// 		} while (0);
// 		
// 		return bRet;
// 	}
// 
// 	bool MemImpl::Write( void const* lpBuffer, unsigned long ulNumberOfBytesToWrite, unsigned long* lpNumberOfBytesWritten )
// 	{
// 		bool bRet = false;
// 		unsigned long dwWritten = ulNumberOfBytesToWrite;
// 
// 		do {
// 			if ( !lpBuffer || !ulNumberOfBytesToWrite ) {
// 				break;
// 			}
// 
// 			if ( pos_ + ulNumberOfBytesToWrite > buffer_.GetCount() ) {
// 				buffer_.SetCount(pos_ + ulNumberOfBytesToWrite);
// 			}
// 
// 			memcpy(buffer_.GetData() + pos_, lpBuffer, dwWritten);
// 			pos_ += dwWritten;
// 
// 			if ( lpNumberOfBytesWritten ) {
// 				*lpNumberOfBytesWritten = dwWritten;
// 			}
// 
// 			bRet = true;
// 		} while (0);
// 		
// 		return bRet;
// 	}
// 
// 	unsigned long MemImpl::Seek( long offset, int origin )
// 	{
// 		unsigned long retval = INVALID_SET_FILE_POINTER;
// 
// 		do {
// 			if ( FILE_CURRENT == origin ) {
// 				if ( offset > 0 ) {
// 					if ( pos_ + offset < buffer_.GetCount() ) {
// 						pos_ += offset;
// 					}
// 					else {
// 						break;
// 					}
// 				}
// 				else {
// 					if (pos_ >= offset) {
// 						pos_ += offset;
// 					}
// 					else {
// 						break;
// 					}
// 				}
// 			}
// 			else if (FILE_END == origin) {
// 				if ( offset > 0 ) {
// 					break;
// 				}
// 
// 				if ( buffer_.GetCount() + offset < 0 ) {
// 					break;
// 				}
// 				pos_ = (unsigned long)buffer_.GetCount() + offset;
// 			}
// 			else if (FILE_BEGIN == origin) {
// 				if ( offset < 0 ) {
// 					break;
// 				}
// 
// 				if ( offset >= buffer_.GetCount() ) {
// 					break;
// 				}
// 
// 				pos_ = offset;
// 			}
// 		} while (0);
// 
// 		return pos_;
// 	}
// 
// 	bool MemImpl::Close() {
// 		return true;
// 	}
// 
// 	bool MemImpl::IsEmpty() {
// 		return buffer_.GetCount() == 0 ? true : false;
// 	}
}