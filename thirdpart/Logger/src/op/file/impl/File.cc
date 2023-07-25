#include "../File.h"
#include "FileImpl.h"
#include "../../../auth/auth.h"

namespace Operation {

	CFile::CFile(char const* path)
		: impl_(New<FileImpl>(path)) {
		//placement new
	}

	CFile::~CFile() {
		Delete<FileImpl>(impl_);
	}
	
	char const* CFile::Path() {
		AUTHORIZATION_CHECK_P;
		return impl_->Path();
	}

	bool CFile::Valid() {
		AUTHORIZATION_CHECK_B;
		return impl_->Valid();
	}

	bool CFile::IsFile() {
		AUTHORIZATION_CHECK_B;
		return impl_->IsFile();
	}

	void CFile::ClearErr() {
		AUTHORIZATION_CHECK;
		impl_->ClearErr();
	}

	bool CFile::Close() {
		AUTHORIZATION_CHECK_B;
		return impl_->Close();
	}

	int CFile::Eof() {
		AUTHORIZATION_CHECK_R;
		return impl_->Eof();
	}

	int CFile::Error() {
		AUTHORIZATION_CHECK_I;
		return impl_->Error();
	}

	int CFile::Flush() {
		AUTHORIZATION_CHECK_I;
		return impl_->Flush();
	}

	int CFile::Getc() {
		AUTHORIZATION_CHECK_I;
		return impl_->Getc();
	}

	int CFile::GetPos(fpos_t* pos) {
		AUTHORIZATION_CHECK_I;
		return impl_->GetPos(pos);
	}

	char* CFile::Gets(char* str, int num) {
		AUTHORIZATION_CHECK_P;
		return impl_->Gets(str, num);
	}

	bool CFile::Open(Mode mode) {
		AUTHORIZATION_CHECK_B;
		return impl_->Open(mode);
	}

	int CFile::Putc(int character) {
		AUTHORIZATION_CHECK_I;
		return impl_->Putc(character);
	}

	int CFile::Puts(char const* str) {
		AUTHORIZATION_CHECK_I;
		return impl_->Puts(str);
	}

	size_t CFile::Read(void* ptr, size_t size, size_t count) {
		AUTHORIZATION_CHECK_I;
		return impl_->Read(ptr, size, count);
	}

	int CFile::Seek(long offset, int origin) {
		AUTHORIZATION_CHECK_R;
		return impl_->Seek(offset, origin);
	}

	int CFile::Setpos(const fpos_t* pos) {
		AUTHORIZATION_CHECK_R;
		return impl_->Setpos(pos);
	}

	long CFile::Tell() {
		AUTHORIZATION_CHECK_I;
		return impl_->Tell();
	}

	size_t CFile::Write(void const* ptr, size_t size, size_t count) {
		AUTHORIZATION_CHECK_I;
		return impl_->Write(ptr, size, count);
	}

	void CFile::Rewind() {
		AUTHORIZATION_CHECK;
		impl_->Rewind();
	}

	void CFile::Buffer(char* buffer, size_t size) {
		AUTHORIZATION_CHECK;
		impl_->Buffer(buffer, size);
	}

	void CFile::Buffer(std::string& s) {
		AUTHORIZATION_CHECK;
		impl_->Buffer(s);
	}

	void CFile::Buffer(std::vector<char>& buffer) {
		AUTHORIZATION_CHECK;
		impl_->Buffer(buffer);
	}
}