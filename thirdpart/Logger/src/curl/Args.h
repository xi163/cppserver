#pragma once

#include "../op/IOperation.h"

namespace Curl {
	enum REQState {
		eNetError,
		eContinue,
		eTimeout,
		eSuc,
		eFailed,
		eInterrupt,
	};
}

namespace Operation {
	struct Args {
		Operation::IOperation* value;
		std::string key;
		bool postAsFile;
		struct FileInfo {
			char filename[128];
		};
		struct MemInfo {
			bool multi;
		};
		union {
			FileInfo fi;
			MemInfo mi;
		};
	};
}