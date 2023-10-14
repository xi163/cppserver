
/* websocket 基础协议头(RFC6455规范) 2bytes/uint16_t/16bit */
/*
  高            低高             低
  H             L H             L
 +---------------------------------------------------------------+
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+

FIN                 1bit 帧结束标志位(0/1)，如果设为1，标识消息的最后一帧(尾帧/结束帧)，第一帧(头帧/首帧/起始帧)也可能是最后一帧
RSV1/RSV2/RSV3      3bit 必须设置为0，备用预留，默认都为0
opcode              4bit 操作码，标识消息帧类型，详见如下说明
MASK                1bit 掩码标志位(0/1)，是否加密数据，如果设为1，那么Masking-Key字段存在，C2S-1 S2C-0 DISCONN-1
Payload len         7bit,7+16bit,7+64bit 负载数据长度(扩展数据长度 + 应用数据长度)，取值范围0~127(2^7-1)，
						 如果值为0~125，那么表示负载数据长度，
						 如果值为  126，那么后续2字节(16bit)表示负载数据长度，
						 如果值为  127，那么后续8字节(64bit)表示负载数据长度，最高位为0
Masking-key         0 or 4 bytes 如果MASK标志位设为1，那么该字段存在，否则如果MASK标志位设为0，那么该字段缺失
Payload data        (x + y) bytes 负载数据 = 扩展数据 + 应用数据
						 当MASK标志位为1时，frame-payload-data = frame-masked-extension-data + frame-masked-application-data
						 当MASK标志位为0时，frame-payload-data = frame-unmasked-extension-data + frame-unmasked-application-data
						 frame-masked-extension-data 0x00-0xFF N*8(N>0) frame-masked-application-data 0x00-0xFF N*8(N>0)
						 frame-unmasked-extension-data 0x00-0xFF N*8(N>0) frame-unmasked-application-data 0x00-0xFF N*8(N>0)
Extension data      x bytes  扩展数据，除非协商过扩展，否则扩展数据长度为0bytes
Application data    y bytes  应用数据

-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |Opcode  | Meaning                             | Reference |                                               |
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x0   | Continuation Frame                  | RFC 6455  |  标识一个SEGMENT分片连续帧                       |
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x1   | Text Frame                          | RFC 6455  |  标识一个TEXT类型文本帧
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x2   | Binary Frame                        | RFC 6455  |  标识一个BINARY类型二进制帧
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x8   | Connection Close Frame              | RFC 6455  |  标识一个DISCONN连接关闭帧
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x9   | Ping Frame                          | RFC 6455  |  标识一个PING网络状态探测帧keep-alive/heart-beats
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0xA   | Pong Frame                          | RFC 6455  |  标识一个PONG网络状态探测帧keep-alive/heart-beats
-+--------+-------------------------------------+-----------|-----------------------------------------------+

一个未分片的消息
	FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
一个分片的消息
	FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
	FIN = FrameContinue opcode = SegmentMessage
	...
	...
	FIN = FrameFinished opcode = SegmentMessage (n >= 0)

大端模式(BigEndian) 低地址存高位 buf[4] = 0x12345678
---------------------------------------------------
buf[3] = 0x78  - 高地址 = 低位
buf[2] = 0x56
buf[1] = 0x34
buf[0] = 0x12  - 低地址 = 高位

小端模式(LittleEndian) 低地址存低位 buf[4] = 0x12345678
---------------------------------------------------
buf[3] = 0x12  - 高地址 = 高位
buf[2] = 0x34
buf[1] = 0x56
buf[0] = 0x78  - 低地址 = 低位

*
*/

//#define _NETTOHOST_BIGENDIAN_
//#define LIBWEBSOCKET_DEBUG

#include <libwebsocket/IHttpContext.h>
#include <libwebsocket/websocket.h>
#include <libwebsocket/private/Endian.h>
#include <libwebsocket/private/CurrentThread.h>

#include <openssl/sha.h>

#define CRLFCRLFSZ 4

#define STATIC_FUNCTON_TO_STRING_IMPLEMENT(MAP_, DETAIL_X_, DETAIL_Y_, NAME_, varname) \
			static std::string varname##_to_string(int varname) { \
				TABLE_DECLARE(table_##varname##s_, MAP_, DETAIL_X_, DETAIL_Y_); \
				RETURN_##NAME_(table_##varname##s_, varname); \
				return ""; \
			}

//websocket连接状态
#define STATE_MAP(XX, YY) \
		XX(Connected, "") \
		XX(Closed, "") \

//websocket握手
#define HANDSHAKE_MAP(XX, YY) \
		YY(WS_ERROR_WANT_MORE,   0x1001, "握手需要读取") \
		YY(WS_ERROR_PARSE,       0x1002, "握手解析失败") \
		YY(WS_ERROR_PACKSZ_LOW,  0x1003, "握手请求头长度限制") \
		YY(WS_ERROR_PACKSZ_HIG,  0x1004, "握手请求头长度限制") \
		YY(WS_ERROR_CRLFCRLF,    0x1005, "握手查找CRLFCRLF") \
		YY(WS_ERROR_CONTEXT,     0x1006, "无效websocket::context") \
		YY(WS_ERROR_PATH,        0x1007, "握手路径错误") \
		YY(WS_ERROR_HTTPCONTEXT, 0x1008, "无效websocket::httpContext") \
		YY(WS_ERROR_HTTPVERSION, 0x1009, "请求头必须HTTP/1.1版本") \
		YY(WS_ERROR_CONNECTION,  0x100A, "请求头字段错误:Connection") \
		YY(WS_ERROR_UPGRADE,     0x100B, "请求头字段错误:Upgrade") \
		YY(WS_ERROR_ORIGIN,      0x100C, "请求头字段错误:Origin") \
		YY(WS_ERROR_SECPROTOCOL, 0x100D, "请求头字段错误:Sec-WebSocket-Protocol") \
		YY(WS_ERROR_SECVERSION,  0x100E, "请求头字段错误:Sec-WebSocket-Version") \
		YY(WS_ERROR_SECKEY,      0x100F, "请求头字段错误:Sec-WebSocket-Key") \
		YY(WS_ERROR_VERIFY,      0x1010, "请求头校验错误:Sec-WebSocket-Verify") \

//大小端模式
#define ENDIAN_MAP(XX, YY) \
		XX(LittleEndian, "小端:低地址存低位") \
		XX(BigEndian, "大端:低地址存高位") \

//消息流解包步骤
#define STEP_MAP(XX, YY) \
			XX(ReadFrameHeader, "") \
			XX(ReadExtendedPayloadlenU16, "") \
			XX(ReadExtendedPayloadlenI64, "") \
			XX(ReadMaskingkey, "") \
			XX(ReadPayloadData, "") \
			XX(ReadExtendedPayloadDataU16, "") \
			XX(ReadExtendedPayloadDataI64, "") \

//FIN 帧结束标志位
#define FIN_MAP(XX, YY) \
			XX(FrameContinue, "分片连续帧") \
			XX(FrameFinished, "消息结束帧") \

//opcode 操作码，标识消息帧类型
#define OPCODE_MAP(XX, YY) \
			YY(SegmentMessage,0x0, "消息片段") \
			YY(TextMessage,   0x1, "文本消息") \
			YY(BinaryMessage, 0x2, "二进制消息") \
			YY(CloseMessage,  0x8, "连接关闭消息") \
			YY(PingMessage,   0x9, "心跳PING消息") \
			YY(PongMessage,   0xA, "心跳PONG消息") \

//帧控制类型 控制帧/非控制帧
#define FRAMECONTROL_MAP(XX, YY) \
			YY(FrameInvalid,   0, "无效帧") \
			YY(ControlFrame,   1, "控制帧") \
			YY(UnControlFrame, 2, "非控制帧") \

//消息帧类型
#define MESSAGEFRAME_MAP(XX, YY) \
			YY(UnknownFrame,  0x00,                "未知帧") \
			YY(HeadFrame,     0x01,                "头帧") \
			YY(MiddleFrame,   0x02,                "连续帧") \
			YY(TailFrame,     0x04,                "尾帧") \
			YY(HeadTailFrame, HeadFrame|TailFrame, "头尾帧") \

typedef uint8_t rsv123_t;

//websocket协议，遵循RFC6455规范
namespace muduo {
	namespace net {
		namespace websocket {

			//连接状态
			enum StateE {
				STATE_MAP(K_ENUM_X, K_ENUM_Y)
			};
			
			//握手错误码
			enum HandShakeE {
				HANDSHAKE_MAP(ENUM_X, ENUM_Y)
			};
			
			//大小端模式
			enum EndianE {
				ENDIAN_MAP(ENUM_X, ENUM_Y)
			};

			//消息流解包步骤
			enum StepE {
				STEP_MAP(ENUM_X, ENUM_Y)
			};

			//帧结束标志位
			enum FinE {
#if 0
				FrameContinue, //帧未结束
				FrameFinished, //帧已结束(最后一个消息帧)
#else
				FIN_MAP(ENUM_X, ENUM_Y)
#endif
			};

			//操作码，标识消息帧类型
			enum OpcodeE {
#if 0
				//非控制帧，携带应用/扩展数据
				SegmentMessage = 0x0, //分片消息(片段)，连续帧/中间帧/分片帧
				TextMessage    = 0x1, //文本消息，头帧/首帧/起始帧
				BinaryMessage  = 0x2, //二进制消息，头帧/首帧/起始帧

				//非控制帧，作为扩展预留
				Reserved1 = 0x3,
				Reserved2 = 0x4,
				Reserved3 = 0x5,
				Reserved4 = 0x6,
				Reserved5 = 0x7,

				//控制帧，既是头帧/首帧/起始帧，也是尾帧/结束帧
				//可以被插入到消息片段中，如果非控制帧分片传输的话
				//必须满足Payload len<=126字节，且不能被分片

				//发送了连接关闭帧后禁止再[发送]任何数据帧
				//收到了连接关闭帧但之前没有发送连接关闭帧，则必须回应连接关闭帧
				CloseMessage = 0x8, //连接关闭帧，如果应用数据(body)有的话，表明连接关闭的reason

				//可能包含应用数据(body)
				PingMessage  = 0x9, //心跳PING探测帧

				//作为回应发送PONG帧必须包含PING帧传过来的应用数据(body)
				PongMessage  = 0xA, //心跳PONG探测帧，可以被主动发送(单向心跳)

				//控制帧，作为扩展预留
				Reserved6  = 0xB,
				Reserved7  = 0xC,
				Reserved8  = 0xD,
				Reserved9  = 0xE,
				Reserved10 = 0xF,
#else
				OPCODE_MAP(ENUM_X, ENUM_Y)
#endif
			};
			typedef OpcodeE MessageE;

			//帧控制类型 控制帧/非控制帧
			enum FrameControlE {
				FRAMECONTROL_MAP(ENUM_X, ENUM_Y)
			};

			//消息帧类型
			enum MessageFrameE {
#if 0
				UnknownFrame                   = 0x00,
				HeadFrame                      = 0x01, //头帧/首帧/起始帧
				MiddleFrame                    = 0x02, //连续帧/中间帧/分片帧
				TailFrame                      = 0x04, //尾帧/结束帧
				HeadTailFrame = HeadFrame | TailFrame, //未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
#else
				MESSAGEFRAME_MAP(ENUM_X, ENUM_Y)
#endif
			};
			
			//格式化字符串 websocket连接状态
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(STATE_MAP, K_DETAIL_X, K_DETAIL_Y, NAME, State);
			
			//格式化字符串 websocket握手
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(HANDSHAKE_MAP, DETAIL_X, DETAIL_Y, NAME, Handshake);
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(HANDSHAKE_MAP, DETAIL_X, DETAIL_Y, MSG, Msg_Handshake);

			//格式化字符串 大小端模式
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(ENDIAN_MAP, DETAIL_X, DETAIL_Y, NAME, Endian);
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(ENDIAN_MAP, DETAIL_X, DETAIL_Y, MSG, Msg_Endian);

			//格式化字符串 消息流解包步骤
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(STEP_MAP, DETAIL_X, DETAIL_Y, NAME, Step);

			//格式化字符串 Fin 帧结束标志位
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(FIN_MAP, DETAIL_X, DETAIL_Y, NAME, Fin);
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(FIN_MAP, DETAIL_X, DETAIL_Y, MSG, Msg_Fin);
			
			//格式化字符串 opcode 操作码，标识消息帧类型
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(OPCODE_MAP, DETAIL_X, DETAIL_Y, NAME, Opcode);
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(OPCODE_MAP, DETAIL_X, DETAIL_Y, MSG, Msg_Opcode);
			
			//格式化字符串 帧控制类型 控制帧/非控制帧
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(FRAMECONTROL_MAP, DETAIL_X, DETAIL_Y, NAME, FrameControl);
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(FRAMECONTROL_MAP, DETAIL_X, DETAIL_Y, MSG, MSG_FrameControl);
			
			//格式化字符串 消息帧类型
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(MESSAGEFRAME_MAP, DETAIL_X, DETAIL_Y, NAME, MessageFrame);
			STATIC_FUNCTON_TO_STRING_IMPLEMENT(MESSAGEFRAME_MAP, DETAIL_X, DETAIL_Y, MSG, MSG_MessageFrame);

#ifdef _NETTOHOST_BIGENDIAN_
			//BigEndian
			//header_t websocket
			//         L                       H
			//buf[0] = FIN|RSV1|RSV2|RSV3|opcode
			//         L              H
			//buf[1] = MASK|Payload len
			//
			// 基础协议头(RFC6455规范) uint16_t(16bit)
			struct header_t {
				uint16_t FIN        : 1; //1bit 帧结束标志位(0/1)
				uint16_t RSV1       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV2       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV3       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t opcode     : 4; //4bit 操作码，标识消息帧类型
				uint16_t MASK       : 1; //1bit 掩码标志位(0/1)
				uint16_t Payloadlen : 7; //7bit 负载数据长度(扩展数据长度 + 应用数据长度)，取值范围0~127(2^7-1)
			};
#else		//LittleEndian
			//header_t websocket
			//         L                       H
			//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
			//         L              H
			//buf[1] = Payload len|MASK
			//
			// 基础协议头(RFC6455规范) uint16_t(16bit)
			struct header_t {
				uint16_t opcode     : 4; //4bit 操作码，标识消息帧类型
				uint16_t RSV3       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV2       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV1       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t FIN        : 1; //1bit 帧结束标志位(0/1)
				uint16_t Payloadlen : 7; //7bit 负载数据长度(扩展数据长度 + 应用数据长度)，取值范围0~127(2^7-1)
				uint16_t MASK       : 1; //1bit 掩码标志位(0/1)
			};
#endif
			// 基础协议头大小
			static const size_t kHeaderLen = sizeof(header_t);
			static const size_t kMaskingkeyLen = 4;
			static const size_t kExtendedPayloadlenU16 = sizeof(uint16_t);
			static const size_t kExtendedPayloadlenI64 = sizeof(int64_t);
			
			class Context_;
			
			// 带扩展协议头
			struct extended_header_t {
			public:
				// header_t，uint16_t
				inline void set_header(websocket::header_t const& header) {
#if 0
					memcpy(&this->header, &header, kHeaderLen);
#else
					this->header = header;
#endif
				}
				// header_t，uint16_t
				inline websocket::header_t& get_header() {
					return header;
				}
				// header_t，uint16_t
				inline websocket::header_t const& get_header() const {
					return header;
				}
				// header_t，uint16_t
				inline void reset_header() {
#if 0
					memset(&header, 0, kHeaderLen);
#else
					header = { 0 };
#endif
				}
			public:
				inline size_t getPayloadlen() const {
					return header.Payloadlen;
				}
				inline void setExtendedPayloadlen(uint16_t ExtendedPayloadlen) {
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("ExtendedPayloadlen.u16 = %d", ExtendedPayloadlen);
#endif
					this->ExtendedPayloadlen.u16 = ExtendedPayloadlen;
				}
				inline void setExtendedPayloadlen(int64_t ExtendedPayloadlen) {
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("ExtendedPayloadlen.i64 = %d", ExtendedPayloadlen);
#endif
					this->ExtendedPayloadlen.i64 = ExtendedPayloadlen;
				}
				inline uint16_t getExtendedPayloadlenU16() const {
					switch (header.Payloadlen) {
					case 126:
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("ExtendedPayloadlen.u16 = %d", ExtendedPayloadlen.u16);
#endif
						//ASSERT(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//ASSERT(false);
						//ASSERT(ExtendedPayloadlen.i64 > 0);
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("ExtendedPayloadlen.i64 = %d", ExtendedPayloadlen.i64);
#endif
						return ExtendedPayloadlen.i64;
					default:
						//ASSERT(false);
						//ASSERT(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return 0;
					}
				}
				inline int64_t getExtendedPayloadlenI64() const {
					switch (header.Payloadlen) {
					case 126:
						//ASSERT(false);
						//ASSERT(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//ASSERT(ExtendedPayloadlen.i64 > 0);
						return ExtendedPayloadlen.i64;
					default:
						//ASSERT(false);
						//ASSERT(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return 0;
					}
				}
				inline size_t getExtendedPayloadlen() const {
					switch (header.Payloadlen) {
					case 126:
						//ASSERT(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//BUG ???
						//ASSERT(ExtendedPayloadlen.i64 > 0);
						return ExtendedPayloadlen.i64;
					default:
						//ASSERT(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return 0;
					}
				}
				inline bool existExtendedPayloadlen() const {
					switch (header.Payloadlen) {
					case 126:
						//ASSERT(ExtendedPayloadlen.u16 > 0);
						return true;
					case 127:
						//ASSERT(ExtendedPayloadlen.i64 > 0);
						return true;
					default:
						//ASSERT(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return false;
					}
				}
				inline size_t getRealDatalen() const {
					switch (header.Payloadlen) {
					case 126:
						//ASSERT(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//ASSERT(ExtendedPayloadlen.i64 > 0);
						return ExtendedPayloadlen.i64;
					default:
						//ASSERT(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return header.Payloadlen;
					}
				}
			public:
				inline void setMaskingkey(uint8_t const Masking_key[kMaskingkeyLen], size_t size) {
					ASSERT(size == websocket::kMaskingkeyLen);
#if 0
					uint8_t const Masking_key_[kMaskingkeyLen] = { 0 };
					ASSERT(memcmp(
						this->Masking_key,
						Masking_key_, kMaskingkeyLen) == 0);
#endif
					memcpy(this->Masking_key, Masking_key, websocket::kMaskingkeyLen);
				}
				inline uint8_t const* getMaskingkey() const {
					return Masking_key;
				}
				inline bool existMaskingkey() const {
					switch (header.MASK)
					{
					case 1:
						//BUG???
						//ASSERT(Masking_key[0] != '\0');
						return true;
					case 0:
						ASSERT(Masking_key[0] == '\0');
						return false;
					default:
						ASSERT_V(header.MASK == 0 || header.MASK == 1, "header.MASK=%d", header.MASK);
						return false;
					}
				}
				inline void reset() {
					reset_header();
					static const size_t kExtendedPayloadlenUnion = sizeof ExtendedPayloadlen;
					ASSERT(kExtendedPayloadlenUnion == kExtendedPayloadlenI64);
					memset(&ExtendedPayloadlen, 0, kExtendedPayloadlenI64);
					memset(Masking_key, 0, kMaskingkeyLen);
				}
			public:
				//uint16_t 基础协议头
				websocket::header_t header;

				//字节对齐关系，在ExtendedPayloadlen之前定义占用空间更小
				uint8_t Masking_key[kMaskingkeyLen];

				union {
					uint16_t u16;
					int64_t i64;
				}ExtendedPayloadlen;
				
				//Payload Data
				//uint8_t data[0];
			};
			
			//带扩展协议头大小
			static const size_t kExtendedHeaderLen = sizeof(extended_header_t);

			// 分片/未分片每帧头
			struct frame_header_t {
			public:
				explicit frame_header_t(
					FrameControlE frameControlType,
					MessageFrameE messageFrameType)
					: frameControlType(frameControlType)
					, messageFrameType(messageFrameType)
					, header({ 0 }) {

				}
				explicit frame_header_t(
					FrameControlE frameControlType,
					MessageFrameE messageFrameType, websocket::extended_header_t const& ref)
					: frameControlType(frameControlType)
					, messageFrameType(messageFrameType)
					, header({ 0 }) {
					header.set_header(ref.get_header());
					header.setExtendedPayloadlen(ref.getExtendedPayloadlenI64());
					header.setMaskingkey(ref.getMaskingkey(), websocket::kMaskingkeyLen);
				}
				explicit frame_header_t(frame_header_t const& ref) {
					copy(ref);
				}
				inline frame_header_t const& operator=(frame_header_t const& ref) {
					copy(ref); return *this;
				}
				inline void copy(frame_header_t const& ref) {
					setFrameControlType(ref.getFrameControlType());
					setMessageFrameType(ref.getMessageFrameType());
					//header.reset();
					resetExtendedHeader();
					header.set_header(ref.get_header());
					header.setExtendedPayloadlen(ref.getExtendedPayloadlenI64());
					header.setMaskingkey(ref.getMaskingkey(), websocket::kMaskingkeyLen);
				}
				// header_t，uint16_t
				inline void set_header(websocket::header_t const& header) {
					this->header.set_header(header);
				}
				// header_t，uint16_t
				inline websocket::header_t& get_header() {
					return header.get_header();
				}
				// header_t，uint16_t
				inline websocket::header_t const& get_header() const {
					return header.get_header();
				}
				inline size_t getPayloadlen() const {
					return header.getPayloadlen();
				}
				inline void setExtendedPayloadlen(uint16_t ExtendedPayloadlen) {
					header.setExtendedPayloadlen(ExtendedPayloadlen);
				}
				inline void setExtendedPayloadlen(int64_t ExtendedPayloadlen) {
					header.setExtendedPayloadlen(ExtendedPayloadlen);
				}
				inline uint16_t getExtendedPayloadlenU16() const {
					return header.getExtendedPayloadlenU16();
				}
				inline int64_t getExtendedPayloadlenI64() const {
					return header.getExtendedPayloadlenI64();
				}
				inline size_t getExtendedPayloadlen() const {
					return header.getExtendedPayloadlen();
				}
				inline bool existExtendedPayloadlen() const {
					return header.existExtendedPayloadlen();
				}
				inline size_t getRealDatalen() const {
					return header.getRealDatalen();
				}
				inline void setMaskingkey(uint8_t const Masking_key[kMaskingkeyLen], size_t size) {
					header.setMaskingkey(Masking_key, size);
				}
				inline uint8_t const* getMaskingkey() const {
					return header.getMaskingkey();
				}
				inline bool existMaskingkey() const {
					return header.existMaskingkey();
				}
				inline void resetExtendedHeader() {
#if 0
					memset(&header, 0, kExtendedHeaderLen);
#elif 1
					header = { 0 };
#else
					header.reset();
#endif
				}
			public:
				// 帧控制类型 控制帧/非控制帧
				inline void setFrameControlType(FrameControlE frameControlType) {
					this->frameControlType = frameControlType;
				}
				inline FrameControlE getFrameControlType() const {
					return frameControlType;
				}
				// 消息帧类型
				//头帧/首帧/起始帧
				//连续帧/中间帧/分片帧
				//尾帧/结束帧
				//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
				inline void setMessageFrameType(MessageFrameE messageFrameType) {
					this->messageFrameType = messageFrameType;
				}
				//getMessageFrameType
				inline MessageFrameE getMessageFrameType() const {
					return messageFrameType;
				}
			public:
				// 测试帧头合法性
				inline bool testValidate(websocket::Context_& context);

			public:
				//带扩展协议头
				extended_header_t header;
				//帧控制类型 控制帧/非控制帧
				FrameControlE frameControlType;
				//消息帧类型
				MessageFrameE messageFrameType;
			};

			// 完整websocket header
			struct message_header_t {
			public:
				message_header_t(MessageE messageType)
					: messageType_(messageType) {
				}
				~message_header_t() {
					reset();
				}
			public:
				// 指定消息类型
				inline void setMessageType(MessageE messageType) {
					messageType_ = messageType;
				}
				// 返回消息类型
				inline MessageE getMessageType() const {
					return messageType_;
				}
				inline void reset() {
					frameHeaders_.clear();
				}
				// 返回帧头数
				inline size_t getFrameHeaderSize() const {
					return frameHeaders_.size();
				}
				// 返回第i个帧头
				//@param i 0-返回首帧 否则返回中间帧或尾帧
				inline frame_header_t const* getFrameHeader(size_t i) const {
					int size = frameHeaders_.size();
					if (size > 0) {
						return i < size ?
							&frameHeaders_[i] :
							&frameHeaders_[size - 1];
					}
					return NULL;
				}
				// 返回首个帧头
				inline frame_header_t* getFirstFrameHeader() {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[0];
					}
					return NULL;
				}
				// 返回首个帧头
				inline frame_header_t const* getFirstFrameHeader() const {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[0];
					}
					return NULL;
				}
				// 返回最新帧头
				inline frame_header_t* getLastFrameHeader() {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[size - 1];
					}
					return NULL;
				}
				// 返回最新帧头
				inline frame_header_t const* getLastFrameHeader() const {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[size - 1];
					}
					return NULL;
				}
				// 添加单个帧头
				inline MessageFrameE addFrameHeader(websocket::Context_& context, websocket::extended_header_t& header);

				// 打印全部帧头信息
				inline void dumpFrameHeaders(std::string const& name) {
					int i = 0;
					Tracef("%s\n" \
						"+-----------------------------------------------------------------------------------------------------------------+"
						, name.c_str());
					for (std::vector<frame_header_t>::const_iterator it = frameHeaders_.begin();
						it != frameHeaders_.end(); ++it) {
#if 0
						if (it->existExtendedPayloadlen()) {
							if (it->existMaskingkey()) {
								//Extended payload length & Masking-key
							}
							else {
								//Extended payload length
							}
						}
						else {
							if (it->existMaskingkey()) {
								//Masking-key
							}
							else {
							}
						}
#endif
						printf("Frame[%d][%s][%s] FIN = [%s] opcode = [%s] MASK[%d][%d][%d] {%s}\n",
							i++,
							FrameControl_to_string(it->frameControlType).c_str(),
							Opcode_to_string(messageType_).c_str(),
							Fin_to_string(it->get_header().FIN).c_str(),
							Opcode_to_string(it->get_header().opcode).c_str(),
							it->get_header().MASK,
							it->getPayloadlen(), it->getExtendedPayloadlen(),
							MessageFrame_to_string(it->messageFrameType).c_str());
					}
					printf("+-----------------------------------------------------------------------------------------------------------------+\n");
				}
			private:
				//消息类型 TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
				MessageE messageType_;
				//消息全部帧头
				std::vector<frame_header_t> frameHeaders_;
			};

			class Message : public base::noncopyable {
			public:
				explicit Message(MessageE messageType)
					: messageHeader_(messageType)
					//, messageBuffer_(NULL)
					/*, header_({ 0 })*/
					, segmentOffset_(0) {
				}
				explicit Message(MessageE messageType,
					IBytesBufferPtr& messageBuffer)
					: messageHeader_(messageType)
					, messageBuffer_(std::move(messageBuffer))
					/*, header_({ 0 })*/
					/*, unMask_c_(0)*/
					, segmentOffset_(0) {
					//assert must not be NULL
					ASSERT(messageBuffer_);
				}
				~Message() {
					messageHeader_.reset();
					if (messageBuffer_) {
						messageBuffer_->retrieveAll();
						segmentOffset_ = 0;
						//unMask_c_ = 0;
						//messageBuffer_.reset();
					}
				}
				// 完整websocket消息头(header)
				inline message_header_t& getMessageHeader() {
					return messageHeader_;
				}
				// 完整websocket消息体(body)
				inline void setMessageBuffer(IBytesBufferPtr& buf) {
					//assert must be NULL
					ASSERT(!messageBuffer_);
					messageBuffer_ = std::move(buf);
					//assert must not be NULL
					ASSERT(messageBuffer_);
				}
				// 完整websocket消息体(body)
				inline IBytesBufferPtr& getMessageBuffer() {
					//assert must not be NULL
					ASSERT(messageBuffer_);
					return messageBuffer_;
				}
				// 指定消息类型
				inline void setMessageType(MessageE messageType) {
					messageHeader_.setMessageType(messageType);
				}
				// 返回消息类型
				inline MessageE getMessageType() const {
					return messageHeader_.getMessageType();
				}

				// 返回帧头数
				inline size_t getFrameHeaderSize() const {
					return messageHeader_.getFrameHeaderSize();
				}
				// 返回第i个帧头
				//@param i 0-返回首帧 否则返回中间帧或尾帧
				inline frame_header_t const* getFrameHeader(size_t i) const {
					return messageHeader_.getFrameHeader(i);
				}
				// 返回首个帧头
				inline frame_header_t* getFirstFrameHeader() {
					return messageHeader_.getFirstFrameHeader();
				}
				// 返回首个帧头
				inline frame_header_t const* getFirstFrameHeader() const {
					return messageHeader_.getFirstFrameHeader();
				}
				// 返回最新帧头
				inline frame_header_t* getLastFrameHeader() {
					return messageHeader_.getLastFrameHeader();
				}
				// 返回最新帧头
				inline frame_header_t const* getLastFrameHeader() const {
					return messageHeader_.getLastFrameHeader();
				}
				// 添加单个帧头
				inline MessageFrameE addFrameHeader(websocket::Context_& context, websocket::extended_header_t& header) {
					return messageHeader_.addFrameHeader(context, header);
				}
				// 重置消息
				inline void resetMessage(bool release = false) {
					//重置正处于解析当中的帧头(当前帧头)
					//memset(&header_, 0, kHeaderLen);
					//重置消息头(header)/消息体(body)
					messageHeader_.reset();
					//FIXME crash!!!
					//assert must not be NULL
					//ASSERT(messageBuffer_);
					if (messageBuffer_) {
						messageBuffer_->retrieveAll();
						segmentOffset_ = 0;
						//unMask_c_ = 0;
						//断开连接析构时候调用释放
						if (release) messageBuffer_.reset();
					}
				}
				// 打印全部帧头信息
				inline void dumpFrameHeaders(std::string const& name) {
					messageHeader_.dumpFrameHeaders(name);
				}
				// Payload Data做UnMask计算
				//@param unMask bool
				//@param Masking_key uint8_t const*
				inline bool unMaskPayloadData(bool unMask, IBytesBuffer* buf, uint8_t const Masking_key[kMaskingkeyLen]) {
					if (unMask &&
						buf->readableBytes() > 0 &&
						buf->readableBytes() > segmentOffset_) {
						for (ssize_t i = segmentOffset_;
							i < buf->readableBytes(); ++i) {
							*((uint8_t*)buf->peek() + i) ^= Masking_key[i % websocket::kMaskingkeyLen];
						}
#if 0
						if (unMask_c_++ == 0) {
							ASSERT(segmentOffset_ == 0);
						}
#endif
						segmentOffset_ = buf->readableBytes();
						return true;
					}
					return false;
				}
			private:
				//完整websocket消息头(header)
				message_header_t messageHeader_;
				//完整websocket消息体(body)，存放接收数据解析之后的buffer
				IBytesBufferPtr messageBuffer_;
				//消息分片数据UnMask Masking-key操作段偏移，UnMask操作次数
				size_t segmentOffset_/*, unMask_c_*/;
			};

			class Context_ : public base::noncopyable, public IContext {
			public:
				explicit Context_(
					ICallback* handler,
					IHttpContextPtr& context,
					IBytesBufferPtr& dataBuffer,
					IBytesBufferPtr& controlBuffer,
					EndianE endian = EndianE::LittleEndian)
					: step_(StepE::ReadFrameHeader)
					, state_(StateE::kClosed)
					, handler_(NULL)
					, header_({ 0 })
					, endian_(endian)
					, dataMessage_(OpcodeE::TextMessage)
					, controlMessage_(OpcodeE::CloseMessage)
					//, callbackHandler_(NULL)
					/*, httpContext_(NULL)*/ {
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("...");
#endif
					init(handler, context, dataBuffer, controlBuffer);
				}
				
				// ~Context -> ~Context_ -> ~IContext
				~Context_() {
//#ifdef LIBWEBSOCKET_DEBUG
					Tracef("...");
//#endif
					resetAll();
				}

				//@init
				inline void init(
					ICallback* handler,
					IHttpContextPtr& context,
					IBytesBufferPtr& dataBuffer,
					IBytesBufferPtr& controlBuffer) {
					setHttpContext(context);
					setDataBuffer(dataBuffer);
					setControlBuffer(controlBuffer);
					setCallbackHandler(handler);
				}

				inline void setDataBuffer(IBytesBufferPtr& buf) {
					dataMessage_.setMessageBuffer(buf);
				}

				inline void setControlBuffer(IBytesBufferPtr& buf) {
					controlMessage_.setMessageBuffer(buf);
				}

				inline void setCallbackHandler(ICallbackHandler* handler) {
					handler_ = handler;
					ASSERT(handler_);
				}

				inline ICallbackHandler* getCallbackHandler() {
					ASSERT(handler_);
					return handler_;
				}

				inline void setHttpContext(IHttpContextPtr& context) {
					httpContext_ = std::move(context);
					ASSERT(httpContext_);
				}

				inline IHttpContextPtr& getHttpContext() {
					//ASSERT(httpContext_);
					return httpContext_;
				}

				inline void resetHttpContext() {
					if (httpContext_) {
						httpContext_.reset();
					}
				}
				
				// 正处于解析当中的帧头(当前帧头)
				inline websocket::extended_header_t& getExtendedHeader() {
					return header_;
				}

				// 正处于解析当中的帧头(当前帧头)
				inline websocket::extended_header_t const& getExtendedHeader() const {
					return header_;
				}

				// 数据帧(非控制帧)，携带应用/扩展数据
				inline Message& getDataMessage() {
					return dataMessage_;
				}

				// 控制帧，Payload len<=126字节，且不能被分片
				inline Message& getControlMessage() {
					return controlMessage_;
				}

				inline void resetExtendedHeader() {
#if 0
					//重置正处于解析当中的帧头(当前帧头)
					memset(&header_, 0, kExtendedHeaderLen);
#elif 1
					header_ = { 0 };
#else
					header_.reset();
#endif
				}

				inline void resetDataMessage() {
					dataMessage_.resetMessage();
				}

				inline void resetControlMessage() {
					controlMessage_.resetMessage();
				}

				inline void resetAll() {
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("...");
#endif
					resetHttpContext();
					resetExtendedHeader();
					dataMessage_.resetMessage(true);
					controlMessage_.resetMessage(true);
					step_ = StepE::ReadFrameHeader;
					state_ = StateE::kClosed;
					handler_ = NULL;
				}

				inline void setWebsocketState(StateE state) {
					state_ = state;
				}
				inline StateE getWebsocketState() const {
					return state_;
				}

				inline void setWebsocketStep(StepE step) {
					step_ = step;
				}
				inline StepE getWebsocketStep() const {
					return step_;
				}
			private:
				//解析步骤
				StepE step_;
				//连接状态
				StateE state_;
				//大小端模式
				EndianE endian_;
				//正处于解析当中的帧头(当前帧头)，uint16_t
				websocket::extended_header_t header_;
				//数据帧(非控制帧)，携带应用/扩展数据
				websocket::Message dataMessage_;
				//控制帧，既是头帧/首帧/起始帧，也是尾帧/结束帧
				//可以被插入到消息片段中，如果非控制帧分片传输的话
				//必须满足Payload len<=126字节，且不能被分片
				websocket::Message controlMessage_;
				//HTTP handshake for websocket
				IHttpContextPtr httpContext_;
				//WeakICallbackPtr for callback
				ICallbackHandler* handler_;
			};

			// 测试帧头合法性
			bool frame_header_t::testValidate(websocket::Context_& context) {
				header_t const& header = this->header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				switch (header.opcode)
				{
					//非控制帧，携带应用/扩展数据
				case OpcodeE::SegmentMessage:
					//分片消息帧(消息片段)，中间数据帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//连续帧/中间帧/分片帧
						ASSERT(messageFrameType == MessageFrameE::MiddleFrame);
						break;
					}
					case FinE::FrameFinished: {
						//尾帧/结束帧
						ASSERT(messageFrameType == MessageFrameE::TailFrame);
						break;
					}
					}
					//控制类型安全性检查
					ASSERT(frameControlType == FrameControlE::UnControlFrame);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::TextMessage:
					//文本消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						ASSERT(messageFrameType == MessageFrameE::HeadFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					ASSERT(frameControlType == FrameControlE::UnControlFrame);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::BinaryMessage:
					//二进制消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						ASSERT(messageFrameType == MessageFrameE::HeadFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					ASSERT(frameControlType == FrameControlE::UnControlFrame);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::CloseMessage:
					//连接关闭帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧 bug??????
						//ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					ASSERT(frameControlType == FrameControlE::ControlFrame);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PingMessage:
					//心跳PING探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					ASSERT(frameControlType == FrameControlE::ControlFrame);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PongMessage:
					//心跳PONG探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						ASSERT(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					ASSERT(frameControlType == FrameControlE::ControlFrame);
					break;
				default:
					//FIXME crash!!!
					//ASSERT_V(false, "header.opcode=%d", header.opcode);
					if (handler) {
						Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
						//不再发送数据
						handler->shutdown();
#elif 1
						handler->forceClose();
#else
						handler->forceCloseWithDelay(0.2f);
#endif
						return false;
					}
					break;
				}
				return true;
			}

			// 添加单个帧头
			MessageFrameE message_header_t::addFrameHeader(websocket::Context_& context, websocket::extended_header_t& header) {
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//帧控制类型 控制帧/非控制帧
				FrameControlE frameControlType = FrameControlE::FrameInvalid;
				//消息帧类型
				MessageFrameE messageFrameType = MessageFrameE::UnknownFrame;
				switch (header.get_header().opcode)
				{
					//非控制帧，携带应用/扩展数据
				case OpcodeE::SegmentMessage://分片消息
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//连续帧/中间帧/分片帧
						messageFrameType = MessageFrameE::MiddleFrame;
						break;
					}
					case FinE::FrameFinished: {
						//尾帧/结束帧
						messageFrameType = MessageFrameE::TailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					ASSERT(
						messageType_ == MessageE::TextMessage ||
						messageType_ == MessageE::BinaryMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::TextMessage:
					//文本消息帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					ASSERT(messageType_ == MessageE::TextMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::BinaryMessage:
					//二进制消息帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					ASSERT(messageType_ == MessageE::BinaryMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::CloseMessage:
					//连接关闭帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.get_header().FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					ASSERT(messageType_ == MessageE::CloseMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PingMessage:
					//心跳PING探测帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.get_header().FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					ASSERT(messageType_ == MessageE::PingMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PongMessage:
					//心跳PONG探测帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.get_header().FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					ASSERT(messageType_ == MessageE::PongMessage);
					break;
				default:
					//FIXME crash!!!
					//ASSERT_V(false, "header.opcode=%d", header.get_header().opcode);
					if (handler) {
						Errorf("%s\n%s", Msg_Fin_to_string(header.get_header().FIN).c_str(), Msg_Opcode_to_string(header.get_header().opcode).c_str());
#if 0
						//不再发送数据
						handler->shutdown();
#elif 1
						handler->forceClose();
#else
						handler->forceCloseWithDelay(0.2f);
#endif
						return MessageFrameE::UnknownFrame;
					}
					break;
				}
				//只有数据帧(非控制帧)可以分片，控制帧不能被分片
				if (frameHeaders_.size() > 0) {
					for (std::vector<frame_header_t>::const_iterator it = frameHeaders_.begin();
						it != frameHeaders_.end(); ++it) {
						ASSERT(it->getFrameControlType() == FrameControlE::UnControlFrame);
					}
				}
#if 0
				frame_header_t frameHeader = {
								frameControlType:frameControlType,
								messageFrameType:messageFrameType,
				};
#elif 0
				frame_header_t frameHeader = { 0 };
				frameHeader.setFrameControlType(frameControlType);
				frameHeader.setMessageFrameType(messageFrameType);
				frameHeader.set_header(header.get_header());
				frameHeader.setExtendedPayloadlen(header.getExtendedPayloadlenI64());
				frameHeader.setMaskingkey(header.getMaskingkey(), websocket::kMaskingkeyLen);
#else
				frame_header_t frameHeader(frameControlType, messageFrameType, header);
#endif
				//测试帧头合法性
				switch (frameHeader.testValidate(context)) {
				case true:
					//添加帧头
					frameHeaders_.emplace_back(frameHeader);
					return messageFrameType;
				default:
					return MessageFrameE::UnknownFrame;
				}
			}

			static std::string getAcceptKey(std::string const& key) {
				//sha-1
				std::string KEY = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
				unsigned char sha1_md[20] = { 0 };
				SHA1((const unsigned char*)KEY.c_str(), KEY.length(), sha1_md);
				return utils::base64::Encode(sha1_md, 20);
			}

			static std::string webtime(time_t time) {
				struct tm timeValue;
				gmtime_r(&time, &timeValue);
				char buf[1024];
				// Wed, 20 Apr 2011 17:31:28 GMT
				strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %H:%M:%S %Z", &timeValue);
				return buf;
			}

			static std::string webtime_now() {
				return webtime(time(NULL));
			}

			static void validate_uncontrol_frame_header(websocket::header_t& header) {
				ASSERT(header.opcode >= OpcodeE::SegmentMessage &&
					header.opcode <= OpcodeE::BinaryMessage);
			}

			static void validate_control_frame_header(websocket::header_t& header) {
				//控制帧，Payload len<=126字节，且不能被分片
				ASSERT(header.Payloadlen <= 126);
				ASSERT(header.FIN == FinE::FrameFinished);
				ASSERT(header.opcode >= OpcodeE::CloseMessage &&
					header.opcode <= OpcodeE::PongMessage);
			}

			static void parse_frame_header(websocket::header_t& header, IBytesBuffer* buf) {
#if 1
				//websocket::header_t header = { 0 };
				memcpy(&header, buf->peek(), websocket::kHeaderLen);
				buf->retrieve(websocket::kHeaderLen);
#else
				//         L                       H
				//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
				uint8_t c1 = *buf->peek();
				header.opcode = c1 & 0x0F;
				header.FIN = (c1 >> 7) & 0xFF;
				//         L              H
				//buf[1] = Payload len|MASK
				uint8_t c2 = *(buf->peek() + 1);
				header.Payloadlen = c2 & 0x7F;
				header.MASK = (c2 >> 7) & 0xFF;
				buf->retrieve(websocket::kHeaderLen);
#endif
			}

			static websocket::header_t pack_unmask_uncontrol_frame_header(
				IBytesBuffer* buf,
				websocket::OpcodeE opcode, websocket::FinE fin, size_t Payloadlen) {
				//0~127(2^7-1) 0x7F
				ASSERT(Payloadlen <= 127);
				//websocket::header_t int16_t
				websocket::header_t header = { 0 };
#if 1
				header.opcode = opcode;
				header.FIN = fin;
				header.Payloadlen = Payloadlen;
				header.MASK = 0;//S2C-0
#else
				//         L                       H
				//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
				header.opcode = opcode & 0x0F;
				header.FIN = 0x0 | (fin << 7);
				//         L              H
				//buf[1] = Payload len|MASK
				header.Payloadlen = Payloadlen & 0x7F;
				header.MASK = 0;//S2C-0
#endif
			//验证非控制帧头有效性
				validate_uncontrol_frame_header(header);
				buf->append(&header, websocket::kHeaderLen);
				return header;
			}

			static websocket::header_t pack_unmask_control_frame_header(
				IBytesBuffer* buf,
				websocket::OpcodeE opcode, websocket::FinE fin, size_t Payloadlen) {
				//0~127(2^7-1) 0x7F
				ASSERT(Payloadlen <= 127);
				//websocket::header_t int16_t
				websocket::header_t header = { 0 };
#if 1
				header.opcode = opcode;
				header.FIN = fin;
				header.Payloadlen = Payloadlen;
				header.MASK = 0;//S2C-0
#else
				//         L                       H
				//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
				header.opcode = opcode & 0x0F;
				header.FIN = 0x0 | (fin << 7);
				//         L              H
				//buf[1] = Payload len|MASK
				header.Payloadlen = Payloadlen & 0x7F;
				header.MASK = 0;//S2C-0
#endif
			//验证控制帧头有效性
				validate_control_frame_header(header);
				buf->append(&header, websocket::kHeaderLen);
				return header;
			}

			//pack_unmask_uncontrol_frame S2C
			static websocket::header_t pack_unmask_uncontrol_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::OpcodeE opcode, websocket::FinE fin) {
				//websocket::header_t int16_t
				websocket::header_t header = { 0 };
				//Masking_key
				uint8_t Masking_key[kMaskingkeyLen] = { 0 };
				size_t Payloadlen = 0;
				uint16_t ExtendedPayloadlenU16 = 0;
				int64_t ExtendedPayloadlenI64 = 0;
				//如果值为0~125，那么表示负载数据长度
				if (len < 126) {
					Payloadlen = len;
				}
				//若能表示short不溢出0~65535(2^16-1)
				else if (len <= (uint16_t)65535/*0xFFFF*/) {
					Payloadlen = 126;
					ExtendedPayloadlenU16 = Endian::hostToNetwork16(len);
				}
				else /*if (len <= (int64_t)0x7FFFFFFFFFFFFFFF)*/ {
					Payloadlen = 127;
					ExtendedPayloadlenI64 = (int16_t)Endian::hostToNetwork64(len);
				}
				header = pack_unmask_uncontrol_frame_header(buf, opcode, fin, Payloadlen);
				if (Payloadlen == 126) {
					buf->append(&ExtendedPayloadlenU16, websocket::kExtendedPayloadlenU16);
				}
				else if (Payloadlen == 127) {
					buf->append(&ExtendedPayloadlenI64, websocket::kExtendedPayloadlenI64);
				}
				if (header.MASK == 1) {
					buf->append(Masking_key, websocket::kMaskingkeyLen);
				}
				//带帧头的空包
				if (data && len > 0) {
					buf->append(data, len);
				}
				return header;
			}

			websocket::header_t pack_unmask_control_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::OpcodeE opcode, websocket::FinE fin) {
				//websocket::header_t int16_t
				websocket::header_t header = { 0 };
				//Masking_key
				uint8_t Masking_key[kMaskingkeyLen] = { 0 };
				size_t Payloadlen = 0;
				uint16_t ExtendedPayloadlenU16 = 0;
				int64_t ExtendedPayloadlenI64 = 0;
				//如果值为0~125，那么表示负载数据长度
				if (len < 126) {
					Payloadlen = len;
				}
				//若能表示short不溢出0~65535(2^16-1)
				else if (len <= (uint16_t)65535/*0xFFFF*/) {
					Payloadlen = 126;
					ExtendedPayloadlenU16 = Endian::hostToNetwork16(len);
				}
				else /*if (len <= (int64_t)0x7FFFFFFFFFFFFFFF)*/ {
					Payloadlen = 127;
					ExtendedPayloadlenI64 = (int16_t)Endian::hostToNetwork64(len);
				}
				header = pack_unmask_control_frame_header(buf, opcode, fin, Payloadlen);
				if (Payloadlen == 126) {
					buf->append(&ExtendedPayloadlenU16, websocket::kExtendedPayloadlenU16);
				}
				else if (Payloadlen == 127) {
#ifdef LIBWEBSOCKET_DEBUG
					//控制帧，Payload len<=126字节，且不能被分片
					Tracef("Payloadlen =%d ExtendedPayloadlenI64 = %lld",
						Payloadlen, ExtendedPayloadlenI64);
#endif
					ASSERT_V(false, "Payloadlen=%d", Payloadlen);
					buf->append(&ExtendedPayloadlenI64, websocket::kExtendedPayloadlenI64);
				}
				if (header.MASK == 1) {
					buf->append(Masking_key, websocket::kMaskingkeyLen);
				}
				//带帧头的空包
				if (data && len > 0) {
					buf->append(data, len);
				}
				return header;
			}
			
			//FrameControlE MessageFrameE use MAKEWORD best
			static std::pair<FrameControlE, MessageFrameE>
				get_frame_control_message_type(websocket::Context_& context, websocket::header_t const& header) {
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//帧控制类型 控制帧/非控制帧
				websocket::FrameControlE frameControlType = FrameControlE::FrameInvalid;
				//消息帧类型
				websocket::MessageFrameE messageFrameType = MessageFrameE::UnknownFrame;
				switch (header.opcode)
				{
					//非控制帧，携带应用/扩展数据
				case OpcodeE::SegmentMessage://分片消息
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//连续帧/中间帧/分片帧
						messageFrameType = MessageFrameE::MiddleFrame;
						break;
					}
					case FinE::FrameFinished: {
						//尾帧/结束帧
						messageFrameType = MessageFrameE::TailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					//ASSERT(
					//	messageType_ == MessageE::TextMessage ||
					//	messageType_ == MessageE::BinaryMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::TextMessage:
					//文本消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					//ASSERT(messageType_ == MessageE::TextMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::BinaryMessage:
					//二进制消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					//ASSERT(messageType_ == MessageE::BinaryMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::CloseMessage:
					//连接关闭帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					//ASSERT(messageType_ == MessageE::CloseMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PingMessage:
					//心跳PING探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					//ASSERT(messageType_ == MessageE::PingMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PongMessage:
					//心跳PONG探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查
						ASSERT(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					//ASSERT(messageType_ == MessageE::PongMessage);
					break;
				default:
					//FIXME crash!!!
					//ASSERT_V(false, "header.opcode=%d", header.opcode);
					if (handler) {
						Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
						//不再发送数据
						handler->shutdown();
#elif 1
						handler->forceClose();
#else
						handler->forceCloseWithDelay(0.2f);
#endif
						return std::pair<FrameControlE, MessageFrameE>(FrameControlE::FrameInvalid, MessageFrameE::UnknownFrame);
					}
					break;
				}
				return std::pair<FrameControlE, MessageFrameE>(frameControlType, messageFrameType);
			}

			// S2C
			static void pack_unmask_data_frame_chunk(
				websocket::Context_& context,
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::MessageE messageType/* = MessageE::TextMessage*/, size_t chunksz/* = 1024*/) {
				ASSERT(len > chunksz);
				ASSERT(messageType == OpcodeE::TextMessage ||
					messageType == OpcodeE::BinaryMessage);
				ssize_t left = (ssize_t)len;
				ssize_t n = 0;
				websocket::header_t header = pack_unmask_uncontrol_frame(
					buf, data + n, chunksz,
					messageType, websocket::FinE::FrameContinue);
				n += chunksz;
				left -= chunksz;
#ifdef LIBWEBSOCKET_DEBUG
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(context, header);
				Tracef("\n" \
				"+-----------------------------------------------------------------------------------------------------------------+" \
				"Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(messageType).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
#endif
				while (left > 0) {
					size_t size = left > chunksz ? chunksz : left;
					if (left > chunksz) {
						websocket::header_t header = pack_unmask_uncontrol_frame(
							buf, data + n, chunksz,
							websocket::OpcodeE::SegmentMessage,
							websocket::FinE::FrameContinue);
						n += chunksz;
						left -= chunksz;
#ifdef LIBWEBSOCKET_DEBUG
						std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(context, header);
						printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
							i++,
							websocket::FrameControl_to_string(ty.first).c_str(),
							websocket::Opcode_to_string(messageType).c_str(),
							websocket::MessageFrame_to_string(ty.second).c_str(),
							websocket::Opcode_to_string(header.opcode).c_str(),
							websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
#endif
					}
					else {
						websocket::header_t header = pack_unmask_uncontrol_frame(
							buf, data + n, left,
							websocket::OpcodeE::SegmentMessage,
							websocket::FinE::FrameFinished);
						n += left;
						left -= left;
#ifdef LIBWEBSOCKET_DEBUG
						std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(context, header);
						Tracef("\nFrame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n" \
							"+-----------------------------------------------------------------------------------------------------------------+",
							i++,
							websocket::FrameControl_to_string(ty.first).c_str(),
							websocket::Opcode_to_string(messageType).c_str(),
							websocket::MessageFrame_to_string(ty.second).c_str(),
							websocket::Opcode_to_string(header.opcode).c_str(),
							websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
#endif
						ASSERT(left == 0);
						break;
					}
				}
			}

			// S2C
			void pack_unmask_data_frame(
				IContext* context_,
				IBytesBuffer* buf,
				char const* data, size_t len,
				MessageT msgType /*= MessageT::TyTextMessage*/, bool chunk/* = false*/) {
				MessageE messageType = (MessageE)msgType;
				ASSERT(
					messageType == MessageE::TextMessage ||
					messageType == MessageE::BinaryMessage ||
					messageType == MessageE::CloseMessage ||
					messageType == MessageE::PingMessage ||
					messageType == MessageE::PongMessage);
				static const size_t chunksz = 1024;
				if (chunk && len > chunksz) {
					websocket::Context_* context = reinterpret_cast<websocket::Context_*>(context_);
					ASSERT(context);
					pack_unmask_data_frame_chunk(*context,
						buf, data, len, messageType, chunksz);
				}
				else {
					websocket::header_t header = pack_unmask_uncontrol_frame(
						buf, data, len,
						messageType, websocket::FinE::FrameFinished);
#ifdef LIBWEBSOCKET_DEBUG
					websocket::Context_* context = reinterpret_cast<websocket::Context_*>(context_);
					ASSERT(context);
					int i = 0;
					std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(*context, header);
					Tracef("\n" \
						"+-----------------------------------------------------------------------------------------------------------------+\n" \
						"Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n" \
						"+-----------------------------------------------------------------------------------------------------------------+",
						i++,
						websocket::FrameControl_to_string(ty.first).c_str(),
						websocket::Opcode_to_string(messageType).c_str(),
						websocket::MessageFrame_to_string(ty.second).c_str(),
						websocket::Opcode_to_string(header.opcode).c_str(),
						websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
#endif
				}
			}

			
			// S2C
			void pack_unmask_close_frame(
				IContext* context_,
				IBytesBuffer* buf,
				char const* data, size_t len) {
				websocket::header_t header = pack_unmask_control_frame(
					buf, data, len,
					websocket::OpcodeE::CloseMessage,
					websocket::FinE::FrameFinished);
#ifdef LIBWEBSOCKET_DEBUG
				websocket::Context_* context = reinterpret_cast<websocket::Context_*>(context_);
				ASSERT(context);
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(*context, header);
				Tracef("\n" \
					"+-----------------------------------------------------------------------------------------------------------------+\n" \
					"Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n" \
					"+-----------------------------------------------------------------------------------------------------------------+",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(websocket::OpcodeE::CloseMessage).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
#endif
			}

			// S2C
			void pack_unmask_ping_frame(
				IContext* context_,
				IBytesBuffer* buf,
				char const* data, size_t len) {
				websocket::header_t header = pack_unmask_control_frame(
					buf, data, len,
					websocket::OpcodeE::PingMessage,
					websocket::FinE::FrameFinished);
#ifdef LIBWEBSOCKET_DEBUG
				websocket::Context_* context = reinterpret_cast<websocket::Context_*>(context_);
				ASSERT(context);
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(*context, header);
				Tracef("\n" \
					"+-----------------------------------------------------------------------------------------------------------------+\n" \
					"Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n" \
					"+-----------------------------------------------------------------------------------------------------------------+",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(websocket::OpcodeE::PingMessage).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
#endif
			}

			// S2C
			void pack_unmask_pong_frame(
				IContext* context_,
				IBytesBuffer* buf,
				char const* data, size_t len) {
				websocket::header_t header = pack_unmask_control_frame(
					buf, data, len,
					websocket::OpcodeE::PongMessage,
					websocket::FinE::FrameFinished);
#ifdef LIBWEBSOCKET_DEBUG
				websocket::Context_* context = reinterpret_cast<websocket::Context_*>(context_);
				ASSERT(context);
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(*context, header);
				Tracef("\n" \
					"+-----------------------------------------------------------------------------------------------------------------+\n" \
					"Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n" \
					"+-----------------------------------------------------------------------------------------------------------------+",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(websocket::OpcodeE::PongMessage).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
#endif
			}

			static uint8_t get_frame_FIN(websocket::header_t const& header) {
				return header.FIN;
			}
			static rsv123_t get_frame_RSV123(websocket::header_t const& header) {
				return (header.RSV1 << 2) | (header.RSV2 << 1) | (header.RSV3);
			}
			static uint8_t get_frame_RSV1(websocket::header_t const& header) {
				return header.RSV1;
			}
			static uint8_t get_frame_RSV2(websocket::header_t const& header) {
				return header.RSV2;
			}
			static uint8_t get_frame_RSV3(websocket::header_t const& header) {
				return header.RSV3;
			}
			static uint8_t get_frame_opcode(websocket::header_t const& header) {
				return header.opcode;
			}
			static uint8_t get_frame_MASK(websocket::header_t const& header) {
				return header.MASK;
			}
			static uint8_t get_frame_Payloadlen(websocket::header_t const& header) {
				return header.Payloadlen;
			}
			
			static void dump_header_info(websocket::header_t const& header) {
				rsv123_t RSV123 = get_frame_RSV123(header);
				uint8_t RSV1 = get_frame_RSV1(header);
				uint8_t RSV2 = get_frame_RSV2(header);
				uint8_t RSV3 = get_frame_RSV3(header);
				//show hex to the left
				Tracef("\n" \
					"+---------------------------------------------------------------+\n" \
					"|0                   1                   2                   3  |\n" \
					"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
					"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
					"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
					"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
					"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
					"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
					"| |1|2|3|       |K|             |                               |\n" \
					"|%d|%01d|%01d|%01d|%07x|%01x|%013d|",
					header.FIN,
					RSV1, RSV2, RSV3,
					header.opcode,
					header.MASK,
					header.Payloadlen);
			}
			
			static void dump_extended_header_info(websocket::extended_header_t const& header) {
				rsv123_t RSV123 = get_frame_RSV123(header.get_header());
				uint8_t RSV1 = get_frame_RSV1(header.get_header());
				uint8_t RSV2 = get_frame_RSV2(header.get_header());
				uint8_t RSV3 = get_frame_RSV3(header.get_header());
				if (header.existExtendedPayloadlen()) {
					if (header.existMaskingkey()) {
						//Extended payload length & Masking-key
						Tracef("\n" \
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|%031d|\n" \
							"+-------------------------------+-------------------------------+\n" \
							"| Masking-key (continued)       |          Payload Data         |\n" \
							"+-------------------------------- - - - - - - - - - - - - - - - +\n" \
							"|%063u|",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen,
							header.getExtendedPayloadlen(),
							Endian::be32BigEndian(header.getMaskingkey()));
					}
					else {
						//Extended payload length
						Tracef("\n" \
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|%031d|",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen,
							header.getExtendedPayloadlen());
					}
				}
				else {
					if (header.existMaskingkey()) {
						//Masking-key
						Tracef("\n" \
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|\n" \
							"+-------------------------------+-------------------------------+\n" \
							"| Masking-key (continued)       |          Payload Data         |\n" \
							"+-------------------------------- - - - - - - - - - - - - - - - +\n" \
							"|%063u|",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen,
							Endian::be32BigEndian(header.getMaskingkey()));
					}
					else {
						//show hex to the left
						Tracef("\n" \
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen);
					}
				}
			}

			// Payload data 非控制帧(数据帧)
// 			static bool parse_control_frame_body_payload_data(
// 				websocket::Context_& context,
// 				IBytesBuffer /*const*/* buf,
// 				ITimestamp* receiveTime, int* saveErrno) {
// 				
// 				
// 			}

			// 消息帧有效性安全检查
			static bool validate_message_frame(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {

				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();

				//数据帧(非控制帧)，携带应用/扩展数据
				websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();

				//控制帧，Payload len<=126字节，且不能被分片
				websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();

				bool ok = true;
				do {
					//消息帧类型
					switch (header.opcode)
					{
					case OpcodeE::SegmentMessage: {
						//分片消息帧(消息片段)，中间数据帧
#ifdef LIBWEBSOCKET_DEBUG
						ASSERT(controlMessage.getMessageHeader().getFrameHeaderSize() == 0);
						ASSERT(dataMessage.getMessageHeader().getFrameHeaderSize() > 0);
#endif
						ASSERT(
							dataMessage.getMessageHeader().getMessageType() == MessageE::TextMessage ||
							dataMessage.getMessageHeader().getMessageType() == MessageE::BinaryMessage);
						//websocket::MessageFrameE messageFrameType = dataMessage.addFrameHeader(extended_header);
						//一定不是头帧
						//ASSERT(!(messageFrameType & websocket::MessageFrameE::HeadFrame));
						//////////////////////////////////////////////////////////////////////////
						//非控制帧(数据帧) frame body
						//Maybe include Extended payload length
						//Maybe include Masking-key
						//Must include Payload data
						break;
					}
					case OpcodeE::TextMessage: {
						//文本消息帧
						//开始解析消息体，解析buffer务必已置空
						ASSERT(dataMessage.getMessageBuffer());
						ASSERT(dataMessage.getMessageBuffer()->readableBytes() == 0);
						//数据帧消息类型
						dataMessage.setMessageType(websocket::OpcodeE::TextMessage);
						//初始数据帧头
						//websocket::MessageFrameE messageFrameType = dataMessage.addFrameHeader(header);
						//一定是头帧
						//ASSERT(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//////////////////////////////////////////////////////////////////////////
						//非控制帧(数据帧) frame body
						//Maybe include Extended payload length
						//Maybe include Masking-key
						//Must include Payload data
						break;
					}
					case OpcodeE::BinaryMessage: {
						//二进制消息帧
						//开始解析消息体，解析buffer务必已置空
						ASSERT(dataMessage.getMessageBuffer());
						ASSERT(dataMessage.getMessageBuffer()->readableBytes() == 0);
						//数据帧消息类型
						dataMessage.setMessageType(websocket::OpcodeE::BinaryMessage);
						//初始数据帧头
						//websocket::MessageFrameE messageFrameType = dataMessage.addFrameHeader(header);
						//一定是头帧
						//ASSERT(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//////////////////////////////////////////////////////////////////////////
						//非控制帧(数据帧) frame body
						//Maybe include Extended payload length
						//Maybe include Masking-key
						//Must include Payload data
						break;
					}
					case OpcodeE::CloseMessage: {
						//连接关闭帧
						//开始解析消息体，解析buffer务必已置空
						ASSERT(controlMessage.getMessageBuffer());
						//FIXME crash!!!
						ASSERT(controlMessage.getMessageBuffer()->readableBytes() == 0);
						//控制帧消息类型
						controlMessage.setMessageType(websocket::OpcodeE::CloseMessage);
						//初始控制帧头
						//websocket::MessageFrameE messageFrameType = controlMessage.addFrameHeader(header);
						//既是是头帧也是尾帧
						//ASSERT(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//ASSERT(messageFrameType & websocket::MessageFrameE::TailFrame);
						//////////////////////////////////////////////////////////////////////////
						//控制帧 frame body
						//Maybe include Extended payload length(<=126)
						//Maybe include Masking-key
						//Maybe include Payload data
						break;
					}
					case OpcodeE::PingMessage: {
						//心跳PING探测帧
						//开始解析消息体，解析buffer务必已置空
						ASSERT(controlMessage.getMessageBuffer());
						ASSERT(controlMessage.getMessageBuffer()->readableBytes() == 0);
						//控制帧消息类型
						controlMessage.setMessageType(websocket::OpcodeE::PingMessage);
						//初始控制帧头
						//websocket::MessageFrameE messageFrameType = controlMessage.addFrameHeader(header);
						//既是是头帧也是尾帧
						//ASSERT(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//ASSERT(messageFrameType & websocket::MessageFrameE::TailFrame);
						//////////////////////////////////////////////////////////////////////////
						//控制帧 frame body
						//Maybe include Extended payload length(<=126)
						//Maybe include Masking-key
						//Maybe include Payload data
						break;
					}
					case OpcodeE::PongMessage: {
						//心跳PONG探测帧
						//开始解析消息体，解析buffer务必已置空
						ASSERT(controlMessage.getMessageBuffer());
						ASSERT(controlMessage.getMessageBuffer()->readableBytes() == 0);
						//控制帧消息类型
						controlMessage.setMessageType(websocket::OpcodeE::PongMessage);
						//初始控制帧头
						//websocket::MessageFrameE messageFrameType = controlMessage.addFrameHeader(header);
						//////////////////////////////////////////////////////////////////////////
						//控制帧 frame body
						//Maybe include Extended payload length(<=126)
						//Maybe include Masking-key
						//Maybe include Payload data
						break;
					}
					}
				} while (0);
				return ok;
			}

			static int parse_frame_ReadExtendedPayloadlenU16(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);
			
			static int parse_frame_ReadExtendedPayloadlenI64(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);
			
			static int parse_frame_ReadMaskingkey(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);
			
			static bool parse_control_frame_ReadPayloadData(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);
			
			static bool parse_uncontrol_frame_ReadPayloadData(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);

			static bool parse_uncontrol_frame_ReadExtendedPayloadDataU16(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);

			static bool parse_uncontrol_frame_ReadExtendedPayloadDataI64(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);

			static bool parse_control_frame_ReadPayloadData(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);

			static bool parse_control_frame_ReadExtendedPayloadDataU16(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);

			static bool parse_control_frame_ReadExtendedPayloadDataI64(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno);

			// 更新帧体(body)消息流解析步骤step
			//	非控制帧(数据帧) frame body
			//		Maybe include Extended payload length
			//		Maybe include Masking-key
			//		Must include Payload data
			//	控制帧 frame body
			//		Maybe include Extended payload length(<=126)
			//		Maybe include Masking-key
			//		Maybe include Payload data
			static bool update_frame_body_parse_step(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool ok = true;
				do {
					//之前解析基础协议头/帧头(header) header_t，uint16_t
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadFrameHeader);
					//Payloadlen 判断
					switch (header.Payloadlen) {
					case 126: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadExtendedPayloadlenU16
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadlenU16);
						{
							parse_frame_ReadExtendedPayloadlenU16(context, buf, receiveTime, saveErrno);
						}
						break;
					}
					case 127: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadExtendedPayloadlenI64
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadlenI64);
						{
							parse_frame_ReadExtendedPayloadlenI64(context, buf, receiveTime, saveErrno);
						}
						break;
					}
					default: {
						ASSERT(header.Payloadlen < 126);
						//判断 MASK = 1，读取Masking_key
						if (header.MASK == 1) {
							//////////////////////////////////////////////////////////////////////////
							//StepE::ReadMaskingkey
							//////////////////////////////////////////////////////////////////////////
							context.setWebsocketStep(websocket::StepE::ReadMaskingkey);
							{
								parse_frame_ReadMaskingkey(context, buf, receiveTime, saveErrno);
							}
							break;
						}
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadPayloadData);
						{
							parse_control_frame_ReadPayloadData(context, buf, receiveTime, saveErrno);
						}
						break;
					}
					}
				} while (0);
				return ok;
			}

			//	解析基础协议头/帧头(header)
			//	输出基础协议头信息
			//	消息帧有效性安全检查
			//	更新帧体(body)消息流解析步骤step
			//@param websocket::Context_& 组件内部私有接口
			static bool parse_frame_ReadFrameHeader(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadFrameHeader
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadFrameHeader);
					//数据包不足够解析，等待下次接收再解析
					if (buf->readableBytes() < websocket::kHeaderLen) {
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kHeaderLen, buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kHeaderLen, buf->readableBytes());
#endif
					//解析基础协议头/帧头(header)
					parse_frame_header(header, buf);
#ifdef LIBWEBSOCKET_DEBUG
					//输出基础协议头信息
					dump_header_info(header);
#endif
					//消息帧有效性安全检查
					//判断并指定帧消息类型
					validate_message_frame(context, buf, receiveTime, saveErrno);
					//更新帧体(body)消息流解析步骤step
					//	非控制帧(数据帧) frame body
					//		Maybe include Extended payload length
					//		Maybe include Masking-key
					//		Must include Payload data
					//	控制帧 frame body
					//		Maybe include Extended payload length(<=126)
					//		Maybe include Masking-key
					//		Maybe include Payload data
					update_frame_body_parse_step(context, buf, receiveTime, saveErrno);
				} while (0);
				return enough;
			}

			static int parse_frame_ReadExtendedPayloadlenU16(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//判断 Payloadlen = 126，读取后续2字节
					ASSERT(header.Payloadlen == 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadExtendedPayloadlenU16
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadlenU16);
					if (buf->readableBytes() < websocket::kExtendedPayloadlenU16) {
						//读取后续2字节，不够
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kExtendedPayloadlenU16, buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kExtendedPayloadlenU16, buf->readableBytes());
#endif
					//读取后续2字节 Extended payload length real Payload Data length
					extended_header.setExtendedPayloadlen(Endian::networkToHost16(buf->readInt16(true)));
					//判断 MASK = 1，读取Masking_key
					if (header.MASK == 1) {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadMaskingkey
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadMaskingkey);
						break;
					}
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataU16
					//////////////////////////////////////////////////////////////////////////
					context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataU16);
				} while (0);
				return enough;
			}
			
			static int parse_frame_ReadExtendedPayloadlenI64(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//判断 Payloadlen = 127，读取后续8字节
					ASSERT(header.Payloadlen == 127);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadExtendedPayloadlenI64
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadlenI64);
					if (buf->readableBytes() < websocket::kExtendedPayloadlenI64) {
						//读取后续8字节，不够
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kExtendedPayloadlenI64, buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kMaskingkeyLen, buf->readableBytes());
#endif
					//读取后续8字节 Extended payload length real Payload Data length
					extended_header.setExtendedPayloadlen((int64_t)Endian::networkToHost64(buf->readInt64(true)));
					//判断 MASK = 1，读取Masking_key
					if (header.MASK == 1) {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadMaskingkey
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadMaskingkey);
						break;
					}
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
					//////////////////////////////////////////////////////////////////////////
					context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataI64);
				} while (0);
				return enough;
			}

			static int parse_frame_ReadMaskingkey(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//判断 MASK = 1，读取Masking_key
					ASSERT(header.MASK == 1);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadMaskingkey
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadMaskingkey);
					if (buf->readableBytes() < websocket::kMaskingkeyLen) {
						//读取不够
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kMaskingkeyLen, buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kMaskingkeyLen, buf->readableBytes());
#endif
					//读取Masking_key
					extended_header.setMaskingkey((uint8_t const*)buf->peek(), websocket::kMaskingkeyLen);
					buf->retrieve(websocket::kMaskingkeyLen);
					//Payloadlen 判断
					switch (header.Payloadlen) {
					case 126: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData/ReadExtendedPayloadDataU16
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataU16);
						{
							std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(context, header);
							//ASSERT(ty.first != FrameControlE::FrameInvalid);
							switch (ty.first) {
							case FrameControlE::FrameInvalid:
								break;
							default:
								enough = (ty.first == FrameControlE::UnControlFrame) ?
									parse_uncontrol_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, saveErrno) :
									parse_control_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, saveErrno);
								break;
							}
						}
						break;
					}
					case 127: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataI64);
						{
							std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(context, header);
							//ASSERT(ty.first != FrameControlE::FrameInvalid);
							switch (ty.first) {
							case FrameControlE::FrameInvalid:
								break;
							default:
								enough = (ty.first == FrameControlE::UnControlFrame) ?
									parse_uncontrol_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, saveErrno) :
									parse_control_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, saveErrno);
								break;
							}
						}
						break;
					}
					default: {
						ASSERT(header.Payloadlen < 126);
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadPayloadData);
						{
							std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(context, header);
							//ASSERT(ty.first != FrameControlE::FrameInvalid);
							switch (ty.first) {
							case FrameControlE::FrameInvalid:
								break;
							default:
								enough = (ty.first == FrameControlE::UnControlFrame) ?
									parse_uncontrol_frame_ReadPayloadData(context, buf, receiveTime, saveErrno) :
									parse_control_frame_ReadPayloadData(context, buf, receiveTime, saveErrno);
								break;
							}
						}
						break;
					}
					}
				} while (0);
				return enough;
			}

			static bool parse_uncontrol_frame_ReadPayloadData(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//数据帧(非控制帧)，携带应用/扩展数据
				//websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();
				bool enough = true;
				do {
					ASSERT(header.Payloadlen < 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadPayloadData);
					//读取Payload Data
					if (buf->readableBytes() < header.Payloadlen) {
						//读取不够
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					//添加消息帧头
					websocket::MessageFrameE messageFrameType = context.getDataMessage().addFrameHeader(context, extended_header);
					switch (messageFrameType)
					{
					case MessageFrameE::UnknownFrame:
						return false;
					default:
						break;
					}
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
#endif
					{
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());
#endif
						//读取Payload Data
						context.getDataMessage().getMessageBuffer()->append(buf->peek(), header.Payloadlen);
						buf->retrieve(header.Payloadlen);
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算
						context.getDataMessage().unMaskPayloadData(
							header.MASK,
							context.getDataMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况
						ASSERT(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置数据帧消息buffer
							//context.resetDataMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
#ifdef LIBWEBSOCKET_DEBUG
							//打印数据帧全部帧头信息
							context.getDataMessage().dumpFrameHeaders("websocket::parse_uncontrol_frame_ReadPayloadData");
#endif
							switch (header.opcode) {
							case OpcodeE::SegmentMessage: {
								//分片消息帧(消息片段)，中间数据帧
								break;
							}
							case OpcodeE::TextMessage: {
								//文本消息帧
								break;
							}
							case OpcodeE::BinaryMessage: {
								//二进制消息帧
								break;
							}
							default:
								//FIXME crash!!!
								//ASSERT_V(false, "header.opcode=%d", header.opcode);
								if (handler) {
									Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									return enough;
								}
								break;
							}
							if (handler) {
								handler->onMessageCallback(
									context.getDataMessage().getMessageBuffer().get(),
									context.getDataMessage().getMessageType(), receiveTime);
							}
							if (context.getDataMessage().getMessageBuffer()->readableBytes() == header.Payloadlen) {
								//context.getDataMessage().getMessageBuffer()没有分片情况
							}
							else {
								//context.getDataMessage().getMessageBuffer()必是分片消息
							}
							//重置数据帧消息buffer
							context.resetDataMessage();
							ASSERT(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			static bool parse_uncontrol_frame_ReadExtendedPayloadDataU16(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//数据帧(非控制帧)，携带应用/扩展数据
				//websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();
				bool enough = true;
				do {
					ASSERT(header.Payloadlen == 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataU16
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadDataU16);
					//读取Payload Data
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenU16()) {
						//读取不够
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					//添加消息帧头
					websocket::MessageFrameE messageFrameType = context.getDataMessage().addFrameHeader(context, extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
#endif
					{
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());
#endif
						//读取Payload Data
						context.getDataMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenU16());
						buf->retrieve(extended_header.getExtendedPayloadlenU16());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算
						context.getDataMessage().unMaskPayloadData(
							header.MASK,
							context.getDataMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况
						ASSERT(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置数据帧消息buffer
							//context.resetDataMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
#ifdef LIBWEBSOCKET_DEBUG
							//打印数据帧全部帧头信息
							context.getDataMessage().dumpFrameHeaders("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataU16");
#endif
							switch (header.opcode) {
							case OpcodeE::SegmentMessage: {
								//分片消息帧(消息片段)，中间数据帧
								break;
							}
							case OpcodeE::TextMessage: {
								//文本消息帧
								break;
							}
							case OpcodeE::BinaryMessage: {
								//二进制消息帧
								break;
							}
							default:
								//FIXME crash!!!
								//ASSERT_V(false, "header.opcode=%d", header.opcode);
								if (handler) {
									Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									return enough;
								}
								break;
							}
							if (handler) {
								handler->onMessageCallback(
									context.getDataMessage().getMessageBuffer().get(),
									context.getDataMessage().getMessageType(), receiveTime);
							}
							if (context.getDataMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenU16()) {
								//context.getDataMessage().getMessageBuffer()没有分片情况
							}
							else {
								//context.getDataMessage().getMessageBuffer()必是分片消息
							}
							//重置数据帧消息buffer
							context.resetDataMessage();
							ASSERT(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			static bool parse_uncontrol_frame_ReadExtendedPayloadDataI64(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//数据帧(非控制帧)，携带应用/扩展数据
				//websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();
				bool enough = true;
				do {
					ASSERT(header.Payloadlen == 127);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadDataI64);
					//读取Payload Data
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenI64()) {
						//读取不够
#ifdef LIBWEBSOCKET_DEBUG
						//parse_uncontrol_frame_ReadExtendedPayloadDataI64[bad][127]: StepE::ReadPayloadData(131072) readableBytes(16370)
						Errorf("[bad][%d]: %s(%lld) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					//添加消息帧头
					websocket::MessageFrameE messageFrameType = context.getDataMessage().addFrameHeader(context, extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
#endif
					{
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("[ok][%d]: %s(%lld) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());
#endif
						//读取Payload Data
						context.getDataMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenI64());
						buf->retrieve(extended_header.getExtendedPayloadlenI64());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算
						context.getDataMessage().unMaskPayloadData(
							header.MASK,
							context.getDataMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况
						//ASSERT(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置数据帧消息buffer
							//context.resetDataMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
#ifdef LIBWEBSOCKET_DEBUG
							//打印数据帧全部帧头信息
							context.getDataMessage().dumpFrameHeaders("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataI64");
#endif
							switch (header.opcode) {
							case OpcodeE::SegmentMessage: {
								//分片消息帧(消息片段)，中间数据帧
								break;
							}
							case OpcodeE::TextMessage: {
								//文本消息帧
								break;
							}
							case OpcodeE::BinaryMessage: {
								//二进制消息帧
								break;
							}
							default:
								//FIXME crash!!!
								//ASSERT_V(false, "header.opcode=%d", header.opcode);
								if (handler) {
									Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									return enough;
								}
								break;
							}
							if (handler) {
								handler->onMessageCallback(
 									context.getDataMessage().getMessageBuffer().get(),
									context.getDataMessage().getMessageType(), receiveTime);
							}
							if (context.getDataMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenI64()) {
								//context.getDataMessage().getMessageBuffer()没有分片情况
							}
							else {
								//context.getDataMessage().getMessageBuffer()必是分片消息
							}
							//重置数据帧消息buffer
							context.resetDataMessage();
							ASSERT(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			static bool parse_control_frame_ReadPayloadData(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//控制帧，携带应用/扩展数据
				//websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();
				bool enough = true;
				do {
					ASSERT(header.Payloadlen < 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadPayloadData);
					//读取Payload Data
					if (buf->readableBytes() < header.Payloadlen) {
						//读取不够
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					//添加消息帧头
					websocket::MessageFrameE messageFrameType = context.getControlMessage().addFrameHeader(context, extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
#endif
					{
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());
#endif
						//读取Payload Data
						context.getControlMessage().getMessageBuffer()->append(buf->peek(), header.Payloadlen);
						buf->retrieve(header.Payloadlen);
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算
						context.getControlMessage().unMaskPayloadData(
							header.MASK,
							context.getControlMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况
						ASSERT(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置控制帧消息buffer
							//context.resetControlMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
#ifdef LIBWEBSOCKET_DEBUG
							//打印控制帧全部帧头信息
							context.getControlMessage().dumpFrameHeaders("websocket::parse_control_frame_ReadPayloadData");
#endif
							switch (header.opcode) {
							case OpcodeE::CloseMessage: {
								//连接关闭帧
								if (handler) {
#if 0
									//BytesBuffer rspdata;
									pack_unmask_close_frame(&rspdata,
										context.getControlMessage().getMessageBuffer()->peek(),
										context.getControlMessage().getMessageBuffer()->readableBytes());
									handler->sendMessage(&rspdata);
#endif
									handler->onClosedCallback(context.getControlMessage().getMessageBuffer().get(), receiveTime);
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									//重置数据帧消息buffer
									context.resetDataMessage();
									ASSERT(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
									//设置连接关闭状态
									context.setWebsocketState(websocket::StateE::kClosed);
								}
								break;
							}
							case OpcodeE::PingMessage: {
								//心跳PING探测帧
								if (handler) {
									handler->onMessageCallback(
 										context.getControlMessage().getMessageBuffer().get(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							case OpcodeE::PongMessage: {
								//心跳PONG探测帧
								if (handler) {
									handler->onMessageCallback(
 										context.getControlMessage().getMessageBuffer().get(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							default:
								//FIXME crash!!!
								//ASSERT_V(false, "header.opcode=%d", header.opcode);
								if (handler) {
									Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									return enough;
								}
								break;
							}
							if (context.getControlMessage().getMessageBuffer()->readableBytes() == header.Payloadlen) {
								//context.getControlMessage().getMessageBuffer()->readableBytes()没有分片情况
							}
							else {
								//context.getControlMessage().getMessageBuffer()->readableBytes()必是分片消息
							}
							//重置控制帧消息buffer
							context.resetControlMessage();
							ASSERT(context.getControlMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			static bool parse_control_frame_ReadExtendedPayloadDataU16(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//控制帧，携带应用/扩展数据
				//websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();
				bool enough = true;
				do {
					ASSERT(header.Payloadlen == 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadPayloadData);
					//读取Payload Data
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenU16()) {
						//读取不够
#ifdef LIBWEBSOCKET_DEBUG
						Errorf("[bad][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					//添加消息帧头
					websocket::MessageFrameE messageFrameType = context.getControlMessage().addFrameHeader(context, extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
#endif
					{
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("[ok][%d]: %s(%d) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());
#endif
						//读取Payload Data
						context.getControlMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenU16());
						buf->retrieve(extended_header.getExtendedPayloadlenU16());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算
						context.getControlMessage().unMaskPayloadData(
							header.MASK,
							context.getControlMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况
						ASSERT(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置控制帧消息buffer
							//context.resetControlMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
#ifdef LIBWEBSOCKET_DEBUG
							//打印控制帧全部帧头信息
							context.getControlMessage().dumpFrameHeaders("websocket::parse_control_frame_ReadExtendedPayloadDataU16");
#endif
							switch (header.opcode) {
							case OpcodeE::CloseMessage: {
								//连接关闭帧
								if (handler) {
#if 0
									//BytesBuffer rspdata;
									pack_unmask_close_frame(&rspdata,
										context.getControlMessage().getMessageBuffer()->peek(),
										context.getControlMessage().getMessageBuffer()->readableBytes());
									handler->sendMessage(&rspdata);
#endif
									handler->onClosedCallback(context.getControlMessage().getMessageBuffer().get(), receiveTime);
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									//重置数据帧消息buffer
									context.resetDataMessage();
									ASSERT(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
									//设置连接关闭状态
									context.setWebsocketState(websocket::StateE::kClosed);
								}
								break;
							}
							case OpcodeE::PingMessage: {
								//心跳PING探测帧
								if (handler) {
									handler->onMessageCallback(
 										context.getControlMessage().getMessageBuffer().get(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							case OpcodeE::PongMessage: {
								//心跳PONG探测帧
								if (handler) {
									handler->onMessageCallback(
 										context.getControlMessage().getMessageBuffer().get(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							default:
								//FIXME crash!!!
								//ASSERT_V(false, "header.opcode=%d", header.opcode);
								if (handler) {
									Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									return enough;
								}
								break;
							}
							if (context.getControlMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenU16()) {
								//context.getControlMessage().getMessageBuffer()没有分片情况
							}
							else {
								//context.getControlMessage().getMessageBuffer()必是分片消息
							}
							//重置控制帧消息buffer
							context.resetControlMessage();
							ASSERT(context.getControlMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			static bool parse_control_frame_ReadExtendedPayloadDataI64(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//控制帧，携带应用/扩展数据
				//websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();
				bool enough = true;
				do {
					ASSERT(header.Payloadlen == 127);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
					//////////////////////////////////////////////////////////////////////////
					ASSERT(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadDataI64);
					//读取Payload Data
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenI64()) {
						//读取不够
#ifdef LIBWEBSOCKET_DEBUG
						//parse_control_frame_ReadExtendedPayloadDataI64[bad][127]: StepE::ReadPayloadData(131072) readableBytes(16370)
						Errorf("[bad][%d]: %s(%lld) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());
#endif
						enough = false;
						break;
					}
#ifdef LIBWEBSOCKET_DEBUG
					//添加消息帧头
					websocket::MessageFrameE messageFrameType = context.getControlMessage().addFrameHeader(context, extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
#endif
					{
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("[ok][%d]: %s(%lld) readableBytes(%d)",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());
#endif
						//读取Payload Data
						context.getControlMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenI64());
						buf->retrieve(extended_header.getExtendedPayloadlenI64());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算
						context.getControlMessage().unMaskPayloadData(
							header.MASK,
							context.getControlMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况
						//ASSERT(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置控制帧消息buffer
							//context.resetControlMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
#ifdef LIBWEBSOCKET_DEBUG
							//打印控制帧全部帧头信息
							context.getControlMessage().dumpFrameHeaders("websocket::parse_control_frame_ReadExtendedPayloadDataI64");
#endif
							switch (header.opcode) {
							case OpcodeE::CloseMessage: {
								//连接关闭帧
								if (handler) {
#if 0
									//BytesBuffer rspdata;
									pack_unmask_close_frame(&rspdata,
										context.getControlMessage().getMessageBuffer()->peek(),
										context.getControlMessage().getMessageBuffer()->readableBytes());
									handler->sendMessage(&rspdata);
#endif
									handler->onClosedCallback(context.getControlMessage().getMessageBuffer().get(), receiveTime);
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									//重置数据帧消息buffer
									context.resetDataMessage();
									ASSERT(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
									//设置连接关闭状态
									context.setWebsocketState(websocket::StateE::kClosed);
								}
								break;
							}
							case OpcodeE::PingMessage: {
								//心跳PING探测帧
								if (handler) {
									handler->onMessageCallback(
 										context.getControlMessage().getMessageBuffer().get(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							case OpcodeE::PongMessage: {
								//心跳PONG探测帧
								if (handler) {
									handler->onMessageCallback(
 										context.getControlMessage().getMessageBuffer().get(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							default:
								//FIXME crash!!!
								//ASSERT_V(false, "header.opcode=%d", header.opcode);
								if (handler) {
									Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									handler->forceClose();
#else
									handler->forceCloseWithDelay(0.2f);
#endif
									return enough;
								}
								break;
							}
							if (context.getControlMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenI64()) {
								//context.getControlMessage().getMessageBuffer()没有分片情况
							}
							else {
								//context.getControlMessage().getMessageBuffer()必是分片消息
							}
							//重置控制帧消息buffer
							context.resetControlMessage();
							ASSERT(context.getControlMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			static int parse_frame(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				//数据帧(非控制帧)，携带应用/扩展数据
				websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				IBytesBuffer* messageBuffer = dataMessage.getMessageBuffer().get();
				ASSERT(messageBuffer);
				//控制帧，Payload len<=126字节，且不能被分片
				websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				IBytesBuffer* controlBuffer = controlMessage.getMessageBuffer().get();
				ASSERT(controlBuffer);
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(context, header);
				//ASSERT(ty.first != FrameControlE::FrameInvalid);
				switch (ty.first) {
				case FrameControlE::FrameInvalid:
					return 0;
				}
				bool enough = true;
#ifdef LIBWEBSOCKET_DEBUG
				int i = 0;
#endif
				//关闭消息帧 PayloadDatalen == 0/buf->readableBytes() == 0
				while (enough && buf->readableBytes() > 0) {
#ifdef LIBWEBSOCKET_DEBUG
					//websocket::parse_frame loop[1] StepE::ReadFrameHeader readableBytes(6)
					//websocket::parse_frame loop[2] StepE::ReadMaskingkey readableBytes(4)
					Tracef("loop[%d] %s readableBytes(%d)",
						++i, websocket::Step_to_string(context.getWebsocketStep()).c_str(), buf->readableBytes());
#endif
					//消息流解析步骤step，先解析包头(header)，再解析包体(body)
					websocket::StepE step = context.getWebsocketStep();
					switch (step) {
					case StepE::ReadFrameHeader: {
						//解析基础协议头/帧头(header) header_t，uint16_t
						//输出基础协议头信息
						//消息帧有效性安全检查
						//更新帧体(body)消息流解析步骤step
						enough = parse_frame_ReadFrameHeader(context, buf, receiveTime, saveErrno);
						break;
					}
					case StepE::ReadExtendedPayloadlenU16: {
						enough = parse_frame_ReadExtendedPayloadlenU16(context, buf, receiveTime, saveErrno);
						break;
					}
					case StepE::ReadExtendedPayloadlenI64: {
						enough = parse_frame_ReadExtendedPayloadlenI64(context, buf, receiveTime, saveErrno);
						break;
					}
					case StepE::ReadMaskingkey: {
						enough = parse_frame_ReadMaskingkey(context, buf, receiveTime, saveErrno);
						break;
					}
					case StepE::ReadPayloadData: {
						ASSERT(ty.first != FrameControlE::FrameInvalid);
						enough = (ty.first == FrameControlE::UnControlFrame) ?
							parse_uncontrol_frame_ReadPayloadData(context, buf, receiveTime, saveErrno):
							parse_control_frame_ReadPayloadData(context, buf, receiveTime, saveErrno);
						break;
					}
					case StepE::ReadExtendedPayloadDataU16:
						ASSERT(ty.first != FrameControlE::FrameInvalid);
						enough = (ty.first == FrameControlE::UnControlFrame) ?
							parse_uncontrol_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, saveErrno) :
							parse_control_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, saveErrno);
						break;
					case StepE::ReadExtendedPayloadDataI64:
						ASSERT(ty.first != FrameControlE::FrameInvalid);
						enough = (ty.first == FrameControlE::UnControlFrame) ?
							parse_uncontrol_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, saveErrno) :
							parse_control_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, saveErrno);
						break;
					default:
						//FIXME crash!!!
						//ASSERT_V(false, "header.opcode=%d", header.opcode);
						if (handler) {
							Errorf("%s\n%s", Msg_Fin_to_string(header.FIN).c_str(), Msg_Opcode_to_string(header.opcode).c_str());
#if 0
							//不再发送数据
							handler->shutdown();
#elif 1
							handler->forceClose();
#else
							handler->forceCloseWithDelay(0.2f);
#endif
							return 0;
						}
						break;
					}
				}
				return 0;
			}
			
			// https://developer.aliyun.com/article/229594
			
			/* websocket 握手协议
			*
			* 握手请求 GET
			* GET / HTTP/1.1
			* Host: 192.168.2.93:10000
			* Connection: Upgrade
			* Pragma: no-cache
			* Cache-Control: no-cache
			* User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 Safari/537.36
			* Upgrade: websocket
			* Origin: http://localhost:7456
			* Sec-WebSocket-Protocol: chat, superchat, default-protocol
			* Sec-WebSocket-Version: 13
			* Accept-Encoding: gzip, deflate, br
			* Accept-Language: zh-CN,zh;q=0.9
			* Sec-WebSocket-Key: ylPFmimkYxdg/eh968/lHQ==
			* Sec-WebSocket-Location: 192.168.2.93:10000
			* Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits

			* 握手成功 SHA-1 Base64
			* HTTP/1.1 101 Switching Protocols
			* Connection: Upgrade
			* Content-Type: application/octet-stream
			* Upgrade: websocket
			* Sec-WebSocket-Version: 13
			* Sec-WebSocket-Accept: RnZfgbNALFZVuKAtOjstw6SacYU=
			* Sec-WebSocket-Protocol: chat, superchat, default-protocol
			* Server: cppserver
			* Timing-Allow-Origin: *
			* Date: Sat, 29 Feb 2020 07:07:34 GMT
			*
			*/
			// 填充websocket握手成功响应头信息
			static inline void create_websocket_response(http::IRequest const* req, std::string const& SecWebSocketProtocol, std::string const& SecWebSocketKey, std::string& rsp) {
				rsp = "HTTP/1.1 101 Switching Protocols\r\n";
				rsp.append("Connection: Upgrade\r\n");
				rsp.append("Content-Type: application/octet-stream\r\n");
				rsp.append("Upgrade: websocket\r\n");
				rsp.append("Sec-WebSocket-Version: 13\r\n");
#if 0
				rsp.append("Sec-WebSocket-Protocol: chat, superchat\r\n");
#else
				if (!SecWebSocketProtocol.empty()) {
					rsp.append("Sec-WebSocket-Protocol: ").append(SecWebSocketProtocol).append("\r\n");
				}
#endif
				rsp.append("Sec-WebSocket-Accept: ");
				rsp.append(getAcceptKey(SecWebSocketKey)).append("\r\n");
 				//rsp.append("Sec-WebSocket-Extensions: " + context.request().getHeader("Sec-WebSocket-Extensions") + "\r\n");
				//rsp.append("Sec-WebSocket-Extensions: x-webkit-deflate-frame\r\n");
				//rsp.append("Sec-WebSocket-Extensions: permessage-deflate\r\n");
				//rsp.append("Sec-WebSocket-Extensions: deflate-stream\r\n");
				rsp.append("Server: cppserver\r\n");
				rsp.append("Timing-Allow-Origin: *\r\n");
				rsp.append("Date: ").append(webtime_now()).append("\r\n\r\n");
			}

			static bool do_handshake(
				websocket::Context_& context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime, int* saveErrno,
				std::string const& path_handshake) {
				websocket::ICallbackHandler* handler = context.getCallbackHandler();
				do {
					//先确定是HTTP数据报文，再解析
					//ASSERT(buf->readableBytes() > 4 && buf->findCRLFCRLF());
//#ifdef LIBWEBSOCKET_DEBUG
					Tracef("bufsize=%d\n\n%.*s", buf->readableBytes(), buf->readableBytes(), buf->peek());
//#endif
					//数据包太小
					if (buf->readableBytes() <= CRLFCRLFSZ) {
						*saveErrno = HandShakeE::WS_ERROR_PACKSZ_LOW/*WS_ERROR_WANT_MORE*/;
#ifdef LIBWEBSOCKET_DEBUG
						Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
						break;
					}
					//数据包太大
					else if (buf->readableBytes() > 1024) {
						*saveErrno = HandShakeE::WS_ERROR_PACKSZ_HIG;
#ifdef LIBWEBSOCKET_DEBUG
						Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
						break;
					}
					//查找\r\n\r\n，务必先找到CRLFCRLF结束符得到完整HTTP请求，否则unique_ptr对象失效
					const char* crlfcrlf = buf->findCRLFCRLF();
					if (!crlfcrlf) {
						*saveErrno = HandShakeE::WS_ERROR_CRLFCRLF;
#ifdef LIBWEBSOCKET_DEBUG
						Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
						break;
					}
					//务必先找到CRLFCRLF结束符得到完整HTTP请求，否则unique_ptr对象失效
					if (!context.getHttpContext()) {
						*saveErrno = HandShakeE::WS_ERROR_HTTPCONTEXT;
#ifdef LIBWEBSOCKET_DEBUG
						Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
						break;
					}
					//ASSERT(context.getHttpContext());
					if (!context.getHttpContext()->parseRequestPtr(buf, receiveTime)) {
						*saveErrno = HandShakeE::WS_ERROR_PARSE;
#ifdef LIBWEBSOCKET_DEBUG
						Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
						break;
					}
					else if (context.getHttpContext()->gotAll()) {
						std::string ipaddr;
						std::string rspdata;
						//填充websocket握手成功响应头信息
						if (context.getHttpContext()->requestConstPtr()->path() != path_handshake) {
							*saveErrno = HandShakeE::WS_ERROR_PATH;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						//HTTP/1.1 - built-in
						muduo::net::http::IRequest::Version version = context.getHttpContext()->requestConstPtr()->getVersion();
						if (version != muduo::net::http::IRequest::kHttp11) {
							*saveErrno = HandShakeE::WS_ERROR_HTTPVERSION;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						//Connection: Upgrade - built-in
						std::string connection = context.getHttpContext()->requestConstPtr()->getHeader("Connection");
						if (connection.empty() || connection != "Upgrade") {
							*saveErrno = HandShakeE::WS_ERROR_CONNECTION;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						//Upgrade: websocket - built-in
						std::string upgrade = context.getHttpContext()->requestConstPtr()->getHeader("Upgrade");
						if (upgrade.empty() || upgrade != "websocket") {
							*saveErrno = HandShakeE::WS_ERROR_UPGRADE;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						//Origin: *
						std::string origin = context.getHttpContext()->requestConstPtr()->getHeader("Origin");
						if (origin.empty()/* || origin != "*"*/) {
							*saveErrno = HandShakeE::WS_ERROR_ORIGIN;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						//Sec-WebSocket-Protocol: chat, superchat, default-protocol
						std::string SecWebSocketProtocol = context.getHttpContext()->requestConstPtr()->getHeader("Sec-WebSocket-Protocol");
						if (!SecWebSocketProtocol.empty() &&
							(SecWebSocketProtocol != "default-protocol" &&
								SecWebSocketProtocol != "chat" &&
								SecWebSocketProtocol != "superchat" &&
								SecWebSocketProtocol != "chat, superchat" &&
								SecWebSocketProtocol != "chat,superchat")) {
							*saveErrno = HandShakeE::WS_ERROR_SECPROTOCOL;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						//Sec-WebSocket-Version: 13 - built-in
						std::string SecWebSocketVersion = context.getHttpContext()->requestConstPtr()->getHeader("Sec-WebSocket-Version");
						if (SecWebSocketVersion.empty() || SecWebSocketVersion != "13") {
							*saveErrno = HandShakeE::WS_ERROR_SECVERSION;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						//Sec-WebSocket-Key: ylPFmimkYxdg/eh968/lHQ== - built-in
						std::string SecWebSocketKey = context.getHttpContext()->requestConstPtr()->getHeader("Sec-WebSocket-Key");
						if (SecWebSocketKey.empty()) {
							*saveErrno = HandShakeE::WS_ERROR_SECKEY;
#ifdef LIBWEBSOCKET_DEBUG
							Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
							break;
						}
						if (handler) {
							if (!handler->onVerifyCallback(context.getHttpContext()->requestConstPtr())) {
								*saveErrno = HandShakeE::WS_ERROR_VERIFY;
#ifdef LIBWEBSOCKET_DEBUG
								Errorf(Msg_Handshake_to_string(*saveErrno).c_str());
#endif
								break;
							}
						}
						//Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
						//std::string SecWebSocketExtensions = context.getHttpContext()->requestConstPtr()->getHeader("Sec-WebSocket-Extensions");
						//if (SecWebSocketExtensions.empty()) {
						//	break;
						//}
						create_websocket_response(context.getHttpContext()->requestConstPtr(), SecWebSocketProtocol, SecWebSocketKey, rspdata);
						std::string ipaddrs = context.getHttpContext()->requestPtr()->getHeader("X-Forwarded-For");
						if (ipaddrs.empty()) {
							if (handler) {
								ipaddr = handler->peerIpAddrToString();
							}
						}
						else {
#if 0
							//第一个IP为客户端真实IP，可伪装，第二个IP为一级代理IP，第三个IP为二级代理IP
							std::string::size_type spos = ipaddrs.find_first_of(',');
							if (spos == std::string::npos) {
							}
							else {
								ipaddr = ipaddrs.substr(0, spos);
							}
#else
							boost::replace_all<std::string>(ipaddrs, " ", "");
							std::vector<std::string> vec;
							boost::algorithm::split(vec, ipaddrs, boost::is_any_of(","));
							for (std::vector<std::string>::const_iterator it = vec.begin();
								it != vec.end(); ++it) {
								if (!it->empty() /*&&
									boost::regex_match(*it, boost::regex(
										"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
										"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
										"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
										"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))*/) {

									if (strncasecmp(it->c_str(), "10.", 3) != 0 &&
										strncasecmp(it->c_str(), "192.168", 7) != 0 &&
										strncasecmp(it->c_str(), "172.16.", 7) != 0) {
										ipaddr = *it;
										break;
									}
								}
							}
#endif
						}
						//ASSERT(!buf->readableBytes());
						buf->retrieveAll();
						//设置连接建立状态
						context.setWebsocketState(websocket::StateE::kConnected);
						if (handler) {
							handler->sendMessage(rspdata);
							handler->onConnectedCallback(ipaddr);
						}
						//握手成功
#ifdef LIBWEBSOCKET_DEBUG
						Tracef("succ");
#endif
						//////////////////////////////////////////////////////////////////////////
						//shared_ptr/weak_ptr 引用计数lock持有/递减是读操作，线程安全!
						//shared_ptr/unique_ptr new创建与reset释放是写操作，非线程安全，操作必须在同一线程!
						//new时内部引用计数递增，reset时递减，递减为0时销毁对象释放资源
						//////////////////////////////////////////////////////////////////////////
						//释放HttpContext资源
						context.getHttpContext().reset();
						return true;
					}
				} while (0);
				switch (*saveErrno)
				{
				case HandShakeE::WS_ERROR_HTTPCONTEXT:
				case HandShakeE::WS_ERROR_HTTPVERSION:
				case HandShakeE::WS_ERROR_CONNECTION:
				case HandShakeE::WS_ERROR_UPGRADE:
				case HandShakeE::WS_ERROR_ORIGIN:
				case HandShakeE::WS_ERROR_SECPROTOCOL:
				case HandShakeE::WS_ERROR_SECVERSION:
				case HandShakeE::WS_ERROR_SECKEY:
				case HandShakeE::WS_ERROR_VERIFY:
				case HandShakeE::WS_ERROR_PATH:
				case HandShakeE::WS_ERROR_PARSE:
				case HandShakeE::WS_ERROR_PACKSZ_LOW:
				case HandShakeE::WS_ERROR_PACKSZ_HIG:
					//握手失败
//#ifdef LIBWEBSOCKET_DEBUG
					Errorf("failed: %s", Msg_Handshake_to_string(*saveErrno).c_str());
//#endif
					break;
				}
				//////////////////////////////////////////////////////////////////////////
				//shared_ptr/weak_ptr 引用计数lock持有/递减是读操作，线程安全!
				//shared_ptr/unique_ptr new创建与reset释放是写操作，非线程安全，操作必须在同一线程!
				//new时内部引用计数递增，reset时递减，递减为0时销毁对象释放资源
				//////////////////////////////////////////////////////////////////////////
				//释放HttpContext资源
				context.getHttpContext().reset();
				return false;
			}

			int parse_message_frame(
				IContext* context_,
				IBytesBuffer* buf,
				ITimestamp* receiveTime,
				std::string const& path_handshake) {
				if (context_) {
					//ASSERT(dynamic_cast<websocket::Context_*>(ctx));
					websocket::Context_* context = reinterpret_cast<websocket::Context_*>(context_);
					int saveErrno = 0;
					if (context->getWebsocketState() == websocket::StateE::kClosed) {
						// 握手
						bool wsConnected = websocket::do_handshake(*context, buf, receiveTime, &saveErrno, path_handshake);
						switch (saveErrno)
						{
						case HandShakeE::WS_ERROR_WANT_MORE:
						case HandShakeE::WS_ERROR_CRLFCRLF:
							break;
						case HandShakeE::WS_ERROR_HTTPCONTEXT:
						case HandShakeE::WS_ERROR_HTTPVERSION:
						case HandShakeE::WS_ERROR_CONNECTION:
						case HandShakeE::WS_ERROR_UPGRADE:
						case HandShakeE::WS_ERROR_ORIGIN:
						case HandShakeE::WS_ERROR_SECPROTOCOL:
						case HandShakeE::WS_ERROR_SECVERSION:
						case HandShakeE::WS_ERROR_SECKEY:
						case HandShakeE::WS_ERROR_VERIFY:
						case HandShakeE::WS_ERROR_PATH:
						case HandShakeE::WS_ERROR_PARSE:
						case HandShakeE::WS_ERROR_PACKSZ_LOW:
						case HandShakeE::WS_ERROR_PACKSZ_HIG: {
							websocket::ICallbackHandler* handler(context->getCallbackHandler());
							if (handler) {
								handler->sendMessage("HTTP/1.1 400 Bad Request\r\n\r\n");
#if 0
								//不再发送数据
								handler->shutdown();
#elif 1
								handler->forceClose();
#else
								handler->forceCloseWithDelay(0.2f);
#endif
							}
							break;
						}
						case 0:
							ASSERT(wsConnected);
							//succ
							break;
						}
					}
					else {
						ASSERT(context->getWebsocketState() == websocket::StateE::kConnected);
						// 解析消息帧
						websocket::parse_frame(*context, buf, receiveTime, &saveErrno);
					}
					return saveErrno;
				}
				return HandShakeE::WS_ERROR_CONTEXT;
			}

			IContext* create(
				ICallback* handler,
				IHttpContextPtr& context,
				IBytesBufferPtr& dataBuffer,
				IBytesBufferPtr& controlBuffer) {
				return new Context_(
					handler,
					context,
					dataBuffer,
					controlBuffer);
			}
			
			// ~Context -> ~Context_ -> resetAll -> ~IContext
			void free(IContext* context) {
				Tracef("...");
				ASSERT(context);
				context->resetAll();
				delete context;
			}

		}//namespace websocket
	}//namespace net
}//namespace muduo