#include "Logger/src/utils/utils.h"

class IContext {
public:
	virtual void resetAll() = 0;
	/*virtual*/ ~IContext() {
		Debugf("");
	}
};

class Context :public IContext {
public:
	explicit Context(int i) {

	}
	~Context() {
		Debugf("");
	}
	void resetAll() {
	
	}
};


int main() {
	
	
	int a = 0,b = 0;
	switch (a) {
	case 1:
		break;
	default:
		break;
	}
	ASSERT_V_IF(a == 0, b>0, "b=%d", b);

	IContext* p = new Context(1);
	delete p;
	//std::string src = "zXkB8PsKv1oTDfWuXoL4P-EtHaNz5qei5uurv1qKecP6ATtZ7TRIB2KOLVdxpITOS1oifFd-4-FrIwrgjz5U8uAOhiTEl-GWtPctzDcczTTZlSLuARLVI_pwP3WE03TxPT4HfD8V3tlDNxMIMNRGD3Gh3wqBoLCmniTFUe5U2jmF-FTqBBvO9summnxsnM-qdv3qCZqOw1FZp7m5yoaOXmSDzVd6kFRmGS5mH6OBQhkdmN4aPlQVhJutLGxlBrll7xGvT1eqzQpXMXb6Jhxp6KpmA5dU_USVyFOo5EkUiQXf-64OtMMEhrJb7d7Rp0qv";
	std::string src = "jjQilTkG_BE4m72aANwhMGoehQ4f9EOJ2C9JNlW5BxiziXXH_7fPLEYRjRCJCgRpNLYTKIuLDtHswWuLJeW8EqGV8dNE4wfIveq1Zvo-5aqcYE1MyLCeKYWXvrhIp9XyGK_exBdG7xA6ZdqGdsqNc_9I8LS2RESp_-QQYOu2-lyy3-UCdxBMbyYO6RdPWQhjXCnuYabqPODjDttbvFgHBxPg_zkGZe6WyIpgETyAShmFA0KENrZckU7kuUjhVVYUh74MZq-XRYKlatfNebtTy6vpz6yUADY_SaMyG9lO0eY-9OR3vGlqOmbMrtoi9IWT";
	std::string key = "111362EE140F157D";
	std::string data = Crypto::AES_ECBDecrypt(src, key);
	//Debugf("%s", data.c_str());

	return 0;
}