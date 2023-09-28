#include "Logger/src/sign/sign.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../rand/impl/StdRandomImpl.h"
#include "../../excp/impl/excpImpl.h"
#include "../../crypt/aes.h"
#include "../../log/impl/LoggerImpl.h"
#include "../time/time.h"

namespace utils {
	namespace sign {

		struct sign_t {
			std::string rand;
			int64_t timestamp;
			int64_t expired;
			BOOST::Any* data;
		};
		std::string Encode(BOOST::Json const& data, int64_t expired, std::string const& secret) {
			sign_t token;
			__MY_TRY()
			token.rand = utils::random::_charStr(_RANDOM().betweenInt(10, 20).randInt_mt());
			__MY_CATCH()
			STD::time_point now(NOW());
			token.timestamp = now.to_millisec();
			token.expired = now.add(expired).to_millisec();
			BOOST::Json json;
			json.put("rand", token.rand);
			json.put("timestamp", token.timestamp);
			json.put("expired", token.expired);
			json.put("data", data);
			std::string s = json.to_string();
			_Errorf("\n%s", s.c_str());
			std::string strBase64 = Crypto::AES_ECBEncrypt(s, secret);
			//std::string strBase64 = Crypto::Des_Encrypt(s, (char *)secret.c_str());
			//return utils::URL::Encode(strBase64);
			return strBase64;
		}
		std::string Encode(BOOST::Any const& data, int64_t expired, std::string const& secret) {
			sign_t token;
			__MY_TRY()
			token.rand = utils::random::_charStr(_RANDOM().betweenInt(10, 20).randInt_mt());
			__MY_CATCH()
			STD::time_point now(NOW());
			token.timestamp = now.to_millisec();
			token.expired = now.add(expired).to_millisec();
			BOOST::Json json;
			json.put("rand", token.rand);
			json.put("timestamp", token.timestamp);
			json.put("expired", token.expired);
			json.put("data", data);
			std::string s = json.to_string();
			_Errorf("\n%s", s.c_str());
			std::string strBase64 = Crypto::AES_ECBEncrypt(s, secret);
			//std::string strBase64 = Crypto::Des_Encrypt(s, (char *)secret.c_str());
			//return utils::URL::Encode(strBase64);
			return strBase64;
		}
	}
}