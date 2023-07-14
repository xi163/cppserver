#pragma once

#include "IOperation.h"

namespace Operation {

	class COperation {
	public:
		COperation();
		virtual ~COperation();
		bool Open(Mode mode = Mode::M_READ);
		void Flush();
		void Close();
		size_t Write(void const* ptr, size_t size, size_t count);
		size_t Read(void* ptr, size_t size, size_t count);
		void Buffer(char* buffer, size_t size);
		void GetBuffer(std::string& s);
		void GetBuffer(std::vector<char>& buffer);
		void SetOperation(IOperation* op);
		IOperation* GetOperation();
	private:
		IOperation* op_;
	};
}