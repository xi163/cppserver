// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: rpc.proto

#ifndef PROTOBUF_INCLUDED_rpc_2eproto
#define PROTOBUF_INCLUDED_rpc_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3007000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3007000 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_rpc_2eproto

// Internal implementation detail -- do not use these members.
struct TableStruct_rpc_2eproto {
  static const ::google::protobuf::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors_rpc_2eproto();
namespace muduo {
namespace net {
class RpcMessage;
class RpcMessageDefaultTypeInternal;
extern RpcMessageDefaultTypeInternal _RpcMessage_default_instance_;
}  // namespace net
}  // namespace muduo
namespace google {
namespace protobuf {
template<> ::muduo::net::RpcMessage* Arena::CreateMaybeMessage<::muduo::net::RpcMessage>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace muduo {
namespace net {

enum MessageType {
  REQUEST = 0,
  RESPONSE = 1,
  ERROR = 2,
  MessageType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::min(),
  MessageType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::max()
};
bool MessageType_IsValid(int value);
const MessageType MessageType_MIN = REQUEST;
const MessageType MessageType_MAX = ERROR;
const int MessageType_ARRAYSIZE = MessageType_MAX + 1;

const ::google::protobuf::EnumDescriptor* MessageType_descriptor();
inline const ::std::string& MessageType_Name(MessageType value) {
  return ::google::protobuf::internal::NameOfEnum(
    MessageType_descriptor(), value);
}
inline bool MessageType_Parse(
    const ::std::string& name, MessageType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<MessageType>(
    MessageType_descriptor(), name, value);
}
enum ErrorCode {
  NO_ERROR = 0,
  WRONG_PROTO = 1,
  NO_SERVICE = 2,
  NO_METHOD = 3,
  INVALID_REQUEST = 4,
  INVALID_RESPONSE = 5,
  TIMEOUT = 6,
  ErrorCode_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::min(),
  ErrorCode_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::max()
};
bool ErrorCode_IsValid(int value);
const ErrorCode ErrorCode_MIN = NO_ERROR;
const ErrorCode ErrorCode_MAX = TIMEOUT;
const int ErrorCode_ARRAYSIZE = ErrorCode_MAX + 1;

const ::google::protobuf::EnumDescriptor* ErrorCode_descriptor();
inline const ::std::string& ErrorCode_Name(ErrorCode value) {
  return ::google::protobuf::internal::NameOfEnum(
    ErrorCode_descriptor(), value);
}
inline bool ErrorCode_Parse(
    const ::std::string& name, ErrorCode* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ErrorCode>(
    ErrorCode_descriptor(), name, value);
}
// ===================================================================

class RpcMessage final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:muduo.net.RpcMessage) */ {
 public:
  RpcMessage();
  virtual ~RpcMessage();

  RpcMessage(const RpcMessage& from);

  inline RpcMessage& operator=(const RpcMessage& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  RpcMessage(RpcMessage&& from) noexcept
    : RpcMessage() {
    *this = ::std::move(from);
  }

  inline RpcMessage& operator=(RpcMessage&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor() {
    return default_instance().GetDescriptor();
  }
  static const RpcMessage& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const RpcMessage* internal_default_instance() {
    return reinterpret_cast<const RpcMessage*>(
               &_RpcMessage_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(RpcMessage* other);
  friend void swap(RpcMessage& a, RpcMessage& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline RpcMessage* New() const final {
    return CreateMaybeMessage<RpcMessage>(nullptr);
  }

  RpcMessage* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<RpcMessage>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const RpcMessage& from);
  void MergeFrom(const RpcMessage& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  #if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  static const char* _InternalParse(const char* begin, const char* end, void* object, ::google::protobuf::internal::ParseContext* ctx);
  ::google::protobuf::internal::ParseFunc _ParseFunc() const final { return _InternalParse; }
  #else
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  #endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(RpcMessage* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return nullptr;
  }
  inline void* MaybeArenaPtr() const {
    return nullptr;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string service = 3;
  void clear_service();
  static const int kServiceFieldNumber = 3;
  const ::std::string& service() const;
  void set_service(const ::std::string& value);
  #if LANG_CXX11
  void set_service(::std::string&& value);
  #endif
  void set_service(const char* value);
  void set_service(const char* value, size_t size);
  ::std::string* mutable_service();
  ::std::string* release_service();
  void set_allocated_service(::std::string* service);

  // string method = 4;
  void clear_method();
  static const int kMethodFieldNumber = 4;
  const ::std::string& method() const;
  void set_method(const ::std::string& value);
  #if LANG_CXX11
  void set_method(::std::string&& value);
  #endif
  void set_method(const char* value);
  void set_method(const char* value, size_t size);
  ::std::string* mutable_method();
  ::std::string* release_method();
  void set_allocated_method(::std::string* method);

  // bytes request = 5;
  void clear_request();
  static const int kRequestFieldNumber = 5;
  const ::std::string& request() const;
  void set_request(const ::std::string& value);
  #if LANG_CXX11
  void set_request(::std::string&& value);
  #endif
  void set_request(const char* value);
  void set_request(const void* value, size_t size);
  ::std::string* mutable_request();
  ::std::string* release_request();
  void set_allocated_request(::std::string* request);

  // bytes response = 6;
  void clear_response();
  static const int kResponseFieldNumber = 6;
  const ::std::string& response() const;
  void set_response(const ::std::string& value);
  #if LANG_CXX11
  void set_response(::std::string&& value);
  #endif
  void set_response(const char* value);
  void set_response(const void* value, size_t size);
  ::std::string* mutable_response();
  ::std::string* release_response();
  void set_allocated_response(::std::string* response);

  // fixed64 id = 2;
  void clear_id();
  static const int kIdFieldNumber = 2;
  ::google::protobuf::uint64 id() const;
  void set_id(::google::protobuf::uint64 value);

  // .muduo.net.MessageType type = 1;
  void clear_type();
  static const int kTypeFieldNumber = 1;
  ::muduo::net::MessageType type() const;
  void set_type(::muduo::net::MessageType value);

  // .muduo.net.ErrorCode error = 7;
  void clear_error();
  static const int kErrorFieldNumber = 7;
  ::muduo::net::ErrorCode error() const;
  void set_error(::muduo::net::ErrorCode value);

  // @@protoc_insertion_point(class_scope:muduo.net.RpcMessage)
 private:
  class HasBitSetters;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr service_;
  ::google::protobuf::internal::ArenaStringPtr method_;
  ::google::protobuf::internal::ArenaStringPtr request_;
  ::google::protobuf::internal::ArenaStringPtr response_;
  ::google::protobuf::uint64 id_;
  int type_;
  int error_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_rpc_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// RpcMessage

// .muduo.net.MessageType type = 1;
inline void RpcMessage::clear_type() {
  type_ = 0;
}
inline ::muduo::net::MessageType RpcMessage::type() const {
  // @@protoc_insertion_point(field_get:muduo.net.RpcMessage.type)
  return static_cast< ::muduo::net::MessageType >(type_);
}
inline void RpcMessage::set_type(::muduo::net::MessageType value) {
  
  type_ = value;
  // @@protoc_insertion_point(field_set:muduo.net.RpcMessage.type)
}

// fixed64 id = 2;
inline void RpcMessage::clear_id() {
  id_ = PROTOBUF_ULONGLONG(0);
}
inline ::google::protobuf::uint64 RpcMessage::id() const {
  // @@protoc_insertion_point(field_get:muduo.net.RpcMessage.id)
  return id_;
}
inline void RpcMessage::set_id(::google::protobuf::uint64 value) {
  
  id_ = value;
  // @@protoc_insertion_point(field_set:muduo.net.RpcMessage.id)
}

// string service = 3;
inline void RpcMessage::clear_service() {
  service_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& RpcMessage::service() const {
  // @@protoc_insertion_point(field_get:muduo.net.RpcMessage.service)
  return service_.GetNoArena();
}
inline void RpcMessage::set_service(const ::std::string& value) {
  
  service_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:muduo.net.RpcMessage.service)
}
#if LANG_CXX11
inline void RpcMessage::set_service(::std::string&& value) {
  
  service_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:muduo.net.RpcMessage.service)
}
#endif
inline void RpcMessage::set_service(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  service_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:muduo.net.RpcMessage.service)
}
inline void RpcMessage::set_service(const char* value, size_t size) {
  
  service_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:muduo.net.RpcMessage.service)
}
inline ::std::string* RpcMessage::mutable_service() {
  
  // @@protoc_insertion_point(field_mutable:muduo.net.RpcMessage.service)
  return service_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* RpcMessage::release_service() {
  // @@protoc_insertion_point(field_release:muduo.net.RpcMessage.service)
  
  return service_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void RpcMessage::set_allocated_service(::std::string* service) {
  if (service != nullptr) {
    
  } else {
    
  }
  service_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), service);
  // @@protoc_insertion_point(field_set_allocated:muduo.net.RpcMessage.service)
}

// string method = 4;
inline void RpcMessage::clear_method() {
  method_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& RpcMessage::method() const {
  // @@protoc_insertion_point(field_get:muduo.net.RpcMessage.method)
  return method_.GetNoArena();
}
inline void RpcMessage::set_method(const ::std::string& value) {
  
  method_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:muduo.net.RpcMessage.method)
}
#if LANG_CXX11
inline void RpcMessage::set_method(::std::string&& value) {
  
  method_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:muduo.net.RpcMessage.method)
}
#endif
inline void RpcMessage::set_method(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  method_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:muduo.net.RpcMessage.method)
}
inline void RpcMessage::set_method(const char* value, size_t size) {
  
  method_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:muduo.net.RpcMessage.method)
}
inline ::std::string* RpcMessage::mutable_method() {
  
  // @@protoc_insertion_point(field_mutable:muduo.net.RpcMessage.method)
  return method_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* RpcMessage::release_method() {
  // @@protoc_insertion_point(field_release:muduo.net.RpcMessage.method)
  
  return method_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void RpcMessage::set_allocated_method(::std::string* method) {
  if (method != nullptr) {
    
  } else {
    
  }
  method_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), method);
  // @@protoc_insertion_point(field_set_allocated:muduo.net.RpcMessage.method)
}

// bytes request = 5;
inline void RpcMessage::clear_request() {
  request_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& RpcMessage::request() const {
  // @@protoc_insertion_point(field_get:muduo.net.RpcMessage.request)
  return request_.GetNoArena();
}
inline void RpcMessage::set_request(const ::std::string& value) {
  
  request_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:muduo.net.RpcMessage.request)
}
#if LANG_CXX11
inline void RpcMessage::set_request(::std::string&& value) {
  
  request_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:muduo.net.RpcMessage.request)
}
#endif
inline void RpcMessage::set_request(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  request_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:muduo.net.RpcMessage.request)
}
inline void RpcMessage::set_request(const void* value, size_t size) {
  
  request_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:muduo.net.RpcMessage.request)
}
inline ::std::string* RpcMessage::mutable_request() {
  
  // @@protoc_insertion_point(field_mutable:muduo.net.RpcMessage.request)
  return request_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* RpcMessage::release_request() {
  // @@protoc_insertion_point(field_release:muduo.net.RpcMessage.request)
  
  return request_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void RpcMessage::set_allocated_request(::std::string* request) {
  if (request != nullptr) {
    
  } else {
    
  }
  request_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), request);
  // @@protoc_insertion_point(field_set_allocated:muduo.net.RpcMessage.request)
}

// bytes response = 6;
inline void RpcMessage::clear_response() {
  response_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& RpcMessage::response() const {
  // @@protoc_insertion_point(field_get:muduo.net.RpcMessage.response)
  return response_.GetNoArena();
}
inline void RpcMessage::set_response(const ::std::string& value) {
  
  response_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:muduo.net.RpcMessage.response)
}
#if LANG_CXX11
inline void RpcMessage::set_response(::std::string&& value) {
  
  response_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:muduo.net.RpcMessage.response)
}
#endif
inline void RpcMessage::set_response(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  response_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:muduo.net.RpcMessage.response)
}
inline void RpcMessage::set_response(const void* value, size_t size) {
  
  response_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:muduo.net.RpcMessage.response)
}
inline ::std::string* RpcMessage::mutable_response() {
  
  // @@protoc_insertion_point(field_mutable:muduo.net.RpcMessage.response)
  return response_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* RpcMessage::release_response() {
  // @@protoc_insertion_point(field_release:muduo.net.RpcMessage.response)
  
  return response_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void RpcMessage::set_allocated_response(::std::string* response) {
  if (response != nullptr) {
    
  } else {
    
  }
  response_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), response);
  // @@protoc_insertion_point(field_set_allocated:muduo.net.RpcMessage.response)
}

// .muduo.net.ErrorCode error = 7;
inline void RpcMessage::clear_error() {
  error_ = 0;
}
inline ::muduo::net::ErrorCode RpcMessage::error() const {
  // @@protoc_insertion_point(field_get:muduo.net.RpcMessage.error)
  return static_cast< ::muduo::net::ErrorCode >(error_);
}
inline void RpcMessage::set_error(::muduo::net::ErrorCode value) {
  
  error_ = value;
  // @@protoc_insertion_point(field_set:muduo.net.RpcMessage.error)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace net
}  // namespace muduo

namespace google {
namespace protobuf {

template <> struct is_proto_enum< ::muduo::net::MessageType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::muduo::net::MessageType>() {
  return ::muduo::net::MessageType_descriptor();
}
template <> struct is_proto_enum< ::muduo::net::ErrorCode> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::muduo::net::ErrorCode>() {
  return ::muduo::net::ErrorCode_descriptor();
}

}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // PROTOBUF_INCLUDED_rpc_2eproto
