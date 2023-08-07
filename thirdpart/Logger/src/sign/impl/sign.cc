#include "Logger/src/sign/sign.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../rand/impl/StdRandomImpl.h"
#include "../../excp/impl/excpImpl.h"
#include "../../crypt/aes.h"
#include "../../log/impl/LoggerImpl.h"

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
				time_t now = (int64_t)time(NULL);
			token.timestamp = (int64_t)now;
			token.expired = now + expired;
			BOOST::Json obj;
			obj.put("rand", token.rand);
			obj.put("timestamp", token.timestamp);
			obj.put("expired", token.expired);
			obj.put("data", data);
			std::string json = obj.to_json();
			__LOG_ERROR("\n%s", json.c_str());
			std::string strBase64 = Crypto::AES_ECBEncrypt(json, secret);
			//std::string strBase64 = Crypto::Des_Encrypt(json, (char *)secret.c_str());
			//return utils::URL::Encode(strBase64);
			return strBase64;
		}
		std::string Encode(BOOST::Any const& data, int64_t expired, std::string const& secret) {
			sign_t token;
			__MY_TRY()
			token.rand = utils::random::_charStr(_RANDOM().betweenInt(10, 20).randInt_mt());
			__MY_CATCH()
			time_t now = (int64_t)time(NULL);
			token.timestamp = (int64_t)now;
			token.expired = now + expired;
			BOOST::Json obj;
			obj.put("rand", token.rand);
			obj.put("timestamp", token.timestamp);
			obj.put("expired", token.expired);
			obj.put("data", data);
			std::string json = obj.to_json();
			__LOG_ERROR("\n%s", json.c_str());
			std::string strBase64 = Crypto::AES_ECBEncrypt(json, secret);
			//std::string strBase64 = Crypto::Des_Encrypt(json, (char *)secret.c_str());
			//return utils::URL::Encode(strBase64);
			return strBase64;
		}
	}
}