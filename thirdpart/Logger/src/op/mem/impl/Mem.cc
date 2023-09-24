#include "../Mem.h"
#include "MemImpl.h"
#include "../../../auth/auth.h"

namespace Operation {

	CMemory::CMemory() : impl_(New<MemImpl>()) {
		//placement new
	}

	CMemory::~CMemory() {
		Delete<MemImpl>(impl_);
	}
	
	char const* CMemory::Path() {
		AUTHORIZATION_CHECK_P;
		return impl_->Path();
	}

	bool CMemory::Valid() {
		AUTHORIZATION_CHECK_B;
		return impl_->Valid();
	}

	bool CMemory::IsFile() {
		AUTHORIZATION_CHECK_B;
		return impl_->IsFile();
	}

	bool CMemory::Close() {
		AUTHORIZATION_CHECK_B;
		return impl_->Close();
	}

	int CMemory::Eof() {
		AUTHORIZATION_CHECK_R;
		return impl_->Eof();
	}

	int CMemory::Getc() {
		AUTHORIZATION_CHECK_I;
		return impl_->Getc();
	}

	int CMemory::GetPos(fpos_t* pos) {
		AUTHORIZATION_CHECK_I;
		return impl_->GetPos(pos);
	}
	
	char* CMemory::Gets(char* str, int num) {
		AUTHORIZATION_CHECK_P;
		return impl_->Gets(str, num);
	}
	
	bool CMemory::Open(Mode mode) {
		AUTHORIZATION_CHECK_B;
		return impl_->Open(mode);
	}

	int CMemory::Putc(int character) {
		AUTHORIZATION_CHECK_I;
		return impl_->Putc(character);
	}

	int CMemory::Puts(char const* str) {
		AUTHORIZATION_CHECK_I;
		return impl_->Puts(str);
	}

	size_t CMemory::Read(void* ptr, size_t size, size_t count) {
		AUTHORIZATION_CHECK_I;
		return impl_->Read(ptr, size, count);
	}

	int CMemory::Seek(long offset, int origin) {
		AUTHORIZATION_CHECK_R;
		return impl_->Seek(offset, origin);
	}

	int CMemory::Setpos(const fpos_t* pos) {
		AUTHORIZATION_CHECK_R;
		return impl_->Setpos(pos);
	}

	long CMemory::Tell() {
		AUTHORIZATION_CHECK_I;
		return impl_->Tell();
	}

	size_t CMemory::Write(void const* ptr, size_t size, size_t count) {
		AUTHORIZATION_CHECK_I;
		return impl_->Write(ptr, size, count);
	}

	void CMemory::Rewind() {
		AUTHORIZATION_CHECK;
		impl_->Rewind();
	}

	void CMemory::Buffer(char* buffer, size_t size) {
		AUTHORIZATION_CHECK;
		impl_->Buffer(buffer, size);
	}

	void CMemory::Buffer(std::string& s) {
		AUTHORIZATION_CHECK;
		impl_->Buffer(s);
	}

	void CMemory::Buffer(std::vector<char>& buffer) {
		AUTHORIZATION_CHECK;
		impl_->Buffer(buffer);
	}

	CMemory::CMemory(void* buffer, unsigned long length) : impl_(New<MemImpl>(buffer, length)) {
		//placement new
	}
// 
// 
// 	bool CMemory::Open() {
// 		return true;
// 	}
// 
// 	bool CMemory::Read( void* lpBuffer, unsigned long ulNumberOfBytesToRead, unsigned long* lpNumberOfBytesRead ) {
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
// 	bool CMemory::Write( void const* lpBuffer, unsigned long ulNumberOfBytesToWrite, unsigned long* lpNumberOfBytesWritten )
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
// 	unsigned long CMemory::Seek( long offset, int origin )
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
// 	bool CMemory::Close() {
// 		return true;
// 	}
// 
// 	bool CMemory::IsEmpty() {
// 		return buffer_.GetCount() == 0 ? true : false;
// 	}
}