#include "../Operation.h"

namespace Operation {

	COperation::COperation() :op_(NULL) {}

	COperation::~COperation() {}

	bool COperation::Open(Mode mode/* = Mode::M_READ*/) {
		if (op_) {
			return op_->Open(mode);
		}
		return false;
	}

	void COperation::Flush() {
		if (op_) {
			op_->Flush();
		}
	}

	void COperation::Close() {
		if (op_) {
			op_->Close();
		}
	}

	size_t COperation::Write(void const* ptr, size_t size, size_t count) {
		if (op_) {
			return op_->Write(ptr, size, count);
		}
		return 0;
	}

	size_t COperation::Read(void* ptr, size_t size, size_t count) {
		if (op_) {
			return op_->Read(ptr, size, count);
		}
		return 0;
	}

	void COperation::Buffer(char* buffer, size_t size) {
		if (op_) {
			op_->Buffer(buffer, size);
		}
	}

	void COperation::GetBuffer(std::string& s) {
		if (op_) {
			op_->Buffer(s);
		}
	}

	void COperation::GetBuffer(std::vector<char>& buffer) {
		if (op_) {
			op_->Buffer(buffer);
		}
	}

	void COperation::SetOperation(IOperation* op) { op_ = op; }

	IOperation* COperation::GetOperation() { return op_; }
}