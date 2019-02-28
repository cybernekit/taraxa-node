// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: proto/transaction.proto

#include "proto/transaction.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// This is a temporary google only hack
#ifdef GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS
#include "third_party/protobuf/version.h"
#endif
// @@protoc_insertion_point(includes)

namespace taraxa_ledger {
class TransactionDefaultTypeInternal {
 public:
  ::google::protobuf::internal::ExplicitlyConstructed<Transaction>
      _instance;
} _Transaction_default_instance_;
class SendTransactionResponseDefaultTypeInternal {
 public:
  ::google::protobuf::internal::ExplicitlyConstructed<SendTransactionResponse>
      _instance;
} _SendTransactionResponse_default_instance_;
}  // namespace taraxa_ledger
namespace protobuf_proto_2ftransaction_2eproto {
static void InitDefaultsTransaction() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::taraxa_ledger::_Transaction_default_instance_;
    new (ptr) ::taraxa_ledger::Transaction();
    ::google::protobuf::internal::OnShutdownDestroyMessage(ptr);
  }
  ::taraxa_ledger::Transaction::InitAsDefaultInstance();
}

::google::protobuf::internal::SCCInfo<0> scc_info_Transaction =
    {{ATOMIC_VAR_INIT(::google::protobuf::internal::SCCInfoBase::kUninitialized), 0, InitDefaultsTransaction}, {}};

static void InitDefaultsSendTransactionResponse() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::taraxa_ledger::_SendTransactionResponse_default_instance_;
    new (ptr) ::taraxa_ledger::SendTransactionResponse();
    ::google::protobuf::internal::OnShutdownDestroyMessage(ptr);
  }
  ::taraxa_ledger::SendTransactionResponse::InitAsDefaultInstance();
}

::google::protobuf::internal::SCCInfo<0> scc_info_SendTransactionResponse =
    {{ATOMIC_VAR_INIT(::google::protobuf::internal::SCCInfoBase::kUninitialized), 0, InitDefaultsSendTransactionResponse}, {}};

void InitDefaults() {
  ::google::protobuf::internal::InitSCC(&scc_info_Transaction.base);
  ::google::protobuf::internal::InitSCC(&scc_info_SendTransactionResponse.base);
}

::google::protobuf::Metadata file_level_metadata[2];

const ::google::protobuf::uint32 TableStruct::offsets[] GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, type_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, nounce_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, value_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, gas_price_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, gas_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, receiver_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, signature_),
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::Transaction, data_),
  ~0u,  // no _has_bits_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::SendTransactionResponse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(::taraxa_ledger::SendTransactionResponse, success_),
};
static const ::google::protobuf::internal::MigrationSchema schemas[] GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::taraxa_ledger::Transaction)},
  { 13, -1, sizeof(::taraxa_ledger::SendTransactionResponse)},
};

static ::google::protobuf::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::google::protobuf::Message*>(&::taraxa_ledger::_Transaction_default_instance_),
  reinterpret_cast<const ::google::protobuf::Message*>(&::taraxa_ledger::_SendTransactionResponse_default_instance_),
};

void protobuf_AssignDescriptors() {
  AddDescriptors();
  AssignDescriptors(
      "proto/transaction.proto", schemas, file_default_instances, TableStruct::offsets,
      file_level_metadata, NULL, NULL);
}

void protobuf_AssignDescriptorsOnce() {
  static ::google::protobuf::internal::once_flag once;
  ::google::protobuf::internal::call_once(once, protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_PROTOBUF_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::internal::RegisterAllTypes(file_level_metadata, 2);
}

void AddDescriptorsImpl() {
  InitDefaults();
  static const char descriptor[] GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
      "\n\027proto/transaction.proto\022\rtaraxa_ledger"
      "\032\033google/protobuf/empty.proto\"\215\001\n\013Transa"
      "ction\022\014\n\004type\030\001 \001(\r\022\016\n\006nounce\030\002 \001(\004\022\r\n\005v"
      "alue\030\003 \001(\004\022\021\n\tgas_price\030\004 \001(\004\022\013\n\003gas\030\005 \001"
      "(\004\022\020\n\010receiver\030\006 \001(\t\022\021\n\tsignature\030\007 \001(\t\022"
      "\014\n\004data\030\010 \001(\014\"*\n\027SendTransactionResponse"
      "\022\017\n\007success\030\001 \001(\0102\266\001\n\023TaraxaLedgerServic"
      "e\022W\n\017SendTransaction\022\032.taraxa_ledger.Tra"
      "nsaction\032&.taraxa_ledger.SendTransaction"
      "Response\"\000\022F\n\016GetTransaction\022\026.google.pr"
      "otobuf.Empty\032\032.taraxa_ledger.Transaction"
      "\"\000b\006proto3"
  };
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 450);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "proto/transaction.proto", &protobuf_RegisterTypes);
  ::protobuf_google_2fprotobuf_2fempty_2eproto::AddDescriptors();
}

void AddDescriptors() {
  static ::google::protobuf::internal::once_flag once;
  ::google::protobuf::internal::call_once(once, AddDescriptorsImpl);
}
// Force AddDescriptors() to be called at dynamic initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;
}  // namespace protobuf_proto_2ftransaction_2eproto
namespace taraxa_ledger {

// ===================================================================

void Transaction::InitAsDefaultInstance() {
}
#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int Transaction::kTypeFieldNumber;
const int Transaction::kNounceFieldNumber;
const int Transaction::kValueFieldNumber;
const int Transaction::kGasPriceFieldNumber;
const int Transaction::kGasFieldNumber;
const int Transaction::kReceiverFieldNumber;
const int Transaction::kSignatureFieldNumber;
const int Transaction::kDataFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

Transaction::Transaction()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  ::google::protobuf::internal::InitSCC(
      &protobuf_proto_2ftransaction_2eproto::scc_info_Transaction.base);
  SharedCtor();
  // @@protoc_insertion_point(constructor:taraxa_ledger.Transaction)
}
Transaction::Transaction(const Transaction& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(NULL) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  receiver_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.receiver().size() > 0) {
    receiver_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.receiver_);
  }
  signature_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.signature().size() > 0) {
    signature_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.signature_);
  }
  data_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.data().size() > 0) {
    data_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.data_);
  }
  ::memcpy(&nounce_, &from.nounce_,
    static_cast<size_t>(reinterpret_cast<char*>(&type_) -
    reinterpret_cast<char*>(&nounce_)) + sizeof(type_));
  // @@protoc_insertion_point(copy_constructor:taraxa_ledger.Transaction)
}

void Transaction::SharedCtor() {
  receiver_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  signature_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  data_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&nounce_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&type_) -
      reinterpret_cast<char*>(&nounce_)) + sizeof(type_));
}

Transaction::~Transaction() {
  // @@protoc_insertion_point(destructor:taraxa_ledger.Transaction)
  SharedDtor();
}

void Transaction::SharedDtor() {
  receiver_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  signature_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  data_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}

void Transaction::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const ::google::protobuf::Descriptor* Transaction::descriptor() {
  ::protobuf_proto_2ftransaction_2eproto::protobuf_AssignDescriptorsOnce();
  return ::protobuf_proto_2ftransaction_2eproto::file_level_metadata[kIndexInFileMessages].descriptor;
}

const Transaction& Transaction::default_instance() {
  ::google::protobuf::internal::InitSCC(&protobuf_proto_2ftransaction_2eproto::scc_info_Transaction.base);
  return *internal_default_instance();
}


void Transaction::Clear() {
// @@protoc_insertion_point(message_clear_start:taraxa_ledger.Transaction)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  receiver_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  signature_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  data_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&nounce_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&type_) -
      reinterpret_cast<char*>(&nounce_)) + sizeof(type_));
  _internal_metadata_.Clear();
}

bool Transaction::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:taraxa_ledger.Transaction)
  for (;;) {
    ::std::pair<::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // uint32 type = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(8u /* 8 & 0xFF */)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &type_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // uint64 nounce = 2;
      case 2: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(16u /* 16 & 0xFF */)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &nounce_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // uint64 value = 3;
      case 3: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(24u /* 24 & 0xFF */)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &value_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // uint64 gas_price = 4;
      case 4: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(32u /* 32 & 0xFF */)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &gas_price_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // uint64 gas = 5;
      case 5: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(40u /* 40 & 0xFF */)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &gas_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // string receiver = 6;
      case 6: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(50u /* 50 & 0xFF */)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_receiver()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->receiver().data(), static_cast<int>(this->receiver().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "taraxa_ledger.Transaction.receiver"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // string signature = 7;
      case 7: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(58u /* 58 & 0xFF */)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_signature()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->signature().data(), static_cast<int>(this->signature().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "taraxa_ledger.Transaction.signature"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // bytes data = 8;
      case 8: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(66u /* 66 & 0xFF */)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_data()));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, _internal_metadata_.mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:taraxa_ledger.Transaction)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:taraxa_ledger.Transaction)
  return false;
#undef DO_
}

void Transaction::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:taraxa_ledger.Transaction)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // uint32 type = 1;
  if (this->type() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(1, this->type(), output);
  }

  // uint64 nounce = 2;
  if (this->nounce() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(2, this->nounce(), output);
  }

  // uint64 value = 3;
  if (this->value() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(3, this->value(), output);
  }

  // uint64 gas_price = 4;
  if (this->gas_price() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(4, this->gas_price(), output);
  }

  // uint64 gas = 5;
  if (this->gas() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(5, this->gas(), output);
  }

  // string receiver = 6;
  if (this->receiver().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->receiver().data(), static_cast<int>(this->receiver().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "taraxa_ledger.Transaction.receiver");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      6, this->receiver(), output);
  }

  // string signature = 7;
  if (this->signature().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->signature().data(), static_cast<int>(this->signature().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "taraxa_ledger.Transaction.signature");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      7, this->signature(), output);
  }

  // bytes data = 8;
  if (this->data().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::WriteBytesMaybeAliased(
      8, this->data(), output);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), output);
  }
  // @@protoc_insertion_point(serialize_end:taraxa_ledger.Transaction)
}

::google::protobuf::uint8* Transaction::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::google::protobuf::uint8* target) const {
  (void)deterministic; // Unused
  // @@protoc_insertion_point(serialize_to_array_start:taraxa_ledger.Transaction)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // uint32 type = 1;
  if (this->type() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(1, this->type(), target);
  }

  // uint64 nounce = 2;
  if (this->nounce() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(2, this->nounce(), target);
  }

  // uint64 value = 3;
  if (this->value() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(3, this->value(), target);
  }

  // uint64 gas_price = 4;
  if (this->gas_price() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(4, this->gas_price(), target);
  }

  // uint64 gas = 5;
  if (this->gas() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(5, this->gas(), target);
  }

  // string receiver = 6;
  if (this->receiver().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->receiver().data(), static_cast<int>(this->receiver().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "taraxa_ledger.Transaction.receiver");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        6, this->receiver(), target);
  }

  // string signature = 7;
  if (this->signature().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->signature().data(), static_cast<int>(this->signature().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "taraxa_ledger.Transaction.signature");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        7, this->signature(), target);
  }

  // bytes data = 8;
  if (this->data().size() > 0) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        8, this->data(), target);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:taraxa_ledger.Transaction)
  return target;
}

size_t Transaction::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:taraxa_ledger.Transaction)
  size_t total_size = 0;

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()));
  }
  // string receiver = 6;
  if (this->receiver().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->receiver());
  }

  // string signature = 7;
  if (this->signature().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->signature());
  }

  // bytes data = 8;
  if (this->data().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::BytesSize(
        this->data());
  }

  // uint64 nounce = 2;
  if (this->nounce() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::UInt64Size(
        this->nounce());
  }

  // uint64 value = 3;
  if (this->value() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::UInt64Size(
        this->value());
  }

  // uint64 gas_price = 4;
  if (this->gas_price() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::UInt64Size(
        this->gas_price());
  }

  // uint64 gas = 5;
  if (this->gas() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::UInt64Size(
        this->gas());
  }

  // uint32 type = 1;
  if (this->type() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::UInt32Size(
        this->type());
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void Transaction::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:taraxa_ledger.Transaction)
  GOOGLE_DCHECK_NE(&from, this);
  const Transaction* source =
      ::google::protobuf::internal::DynamicCastToGenerated<const Transaction>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:taraxa_ledger.Transaction)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:taraxa_ledger.Transaction)
    MergeFrom(*source);
  }
}

void Transaction::MergeFrom(const Transaction& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:taraxa_ledger.Transaction)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.receiver().size() > 0) {

    receiver_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.receiver_);
  }
  if (from.signature().size() > 0) {

    signature_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.signature_);
  }
  if (from.data().size() > 0) {

    data_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.data_);
  }
  if (from.nounce() != 0) {
    set_nounce(from.nounce());
  }
  if (from.value() != 0) {
    set_value(from.value());
  }
  if (from.gas_price() != 0) {
    set_gas_price(from.gas_price());
  }
  if (from.gas() != 0) {
    set_gas(from.gas());
  }
  if (from.type() != 0) {
    set_type(from.type());
  }
}

void Transaction::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:taraxa_ledger.Transaction)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Transaction::CopyFrom(const Transaction& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:taraxa_ledger.Transaction)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Transaction::IsInitialized() const {
  return true;
}

void Transaction::Swap(Transaction* other) {
  if (other == this) return;
  InternalSwap(other);
}
void Transaction::InternalSwap(Transaction* other) {
  using std::swap;
  receiver_.Swap(&other->receiver_, &::google::protobuf::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  signature_.Swap(&other->signature_, &::google::protobuf::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  data_.Swap(&other->data_, &::google::protobuf::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  swap(nounce_, other->nounce_);
  swap(value_, other->value_);
  swap(gas_price_, other->gas_price_);
  swap(gas_, other->gas_);
  swap(type_, other->type_);
  _internal_metadata_.Swap(&other->_internal_metadata_);
}

::google::protobuf::Metadata Transaction::GetMetadata() const {
  protobuf_proto_2ftransaction_2eproto::protobuf_AssignDescriptorsOnce();
  return ::protobuf_proto_2ftransaction_2eproto::file_level_metadata[kIndexInFileMessages];
}


// ===================================================================

void SendTransactionResponse::InitAsDefaultInstance() {
}
#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int SendTransactionResponse::kSuccessFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

SendTransactionResponse::SendTransactionResponse()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  ::google::protobuf::internal::InitSCC(
      &protobuf_proto_2ftransaction_2eproto::scc_info_SendTransactionResponse.base);
  SharedCtor();
  // @@protoc_insertion_point(constructor:taraxa_ledger.SendTransactionResponse)
}
SendTransactionResponse::SendTransactionResponse(const SendTransactionResponse& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(NULL) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  success_ = from.success_;
  // @@protoc_insertion_point(copy_constructor:taraxa_ledger.SendTransactionResponse)
}

void SendTransactionResponse::SharedCtor() {
  success_ = false;
}

SendTransactionResponse::~SendTransactionResponse() {
  // @@protoc_insertion_point(destructor:taraxa_ledger.SendTransactionResponse)
  SharedDtor();
}

void SendTransactionResponse::SharedDtor() {
}

void SendTransactionResponse::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const ::google::protobuf::Descriptor* SendTransactionResponse::descriptor() {
  ::protobuf_proto_2ftransaction_2eproto::protobuf_AssignDescriptorsOnce();
  return ::protobuf_proto_2ftransaction_2eproto::file_level_metadata[kIndexInFileMessages].descriptor;
}

const SendTransactionResponse& SendTransactionResponse::default_instance() {
  ::google::protobuf::internal::InitSCC(&protobuf_proto_2ftransaction_2eproto::scc_info_SendTransactionResponse.base);
  return *internal_default_instance();
}


void SendTransactionResponse::Clear() {
// @@protoc_insertion_point(message_clear_start:taraxa_ledger.SendTransactionResponse)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  success_ = false;
  _internal_metadata_.Clear();
}

bool SendTransactionResponse::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:taraxa_ledger.SendTransactionResponse)
  for (;;) {
    ::std::pair<::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // bool success = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(8u /* 8 & 0xFF */)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &success_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, _internal_metadata_.mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:taraxa_ledger.SendTransactionResponse)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:taraxa_ledger.SendTransactionResponse)
  return false;
#undef DO_
}

void SendTransactionResponse::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:taraxa_ledger.SendTransactionResponse)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // bool success = 1;
  if (this->success() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(1, this->success(), output);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), output);
  }
  // @@protoc_insertion_point(serialize_end:taraxa_ledger.SendTransactionResponse)
}

::google::protobuf::uint8* SendTransactionResponse::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::google::protobuf::uint8* target) const {
  (void)deterministic; // Unused
  // @@protoc_insertion_point(serialize_to_array_start:taraxa_ledger.SendTransactionResponse)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // bool success = 1;
  if (this->success() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteBoolToArray(1, this->success(), target);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:taraxa_ledger.SendTransactionResponse)
  return target;
}

size_t SendTransactionResponse::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:taraxa_ledger.SendTransactionResponse)
  size_t total_size = 0;

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()));
  }
  // bool success = 1;
  if (this->success() != 0) {
    total_size += 1 + 1;
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void SendTransactionResponse::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:taraxa_ledger.SendTransactionResponse)
  GOOGLE_DCHECK_NE(&from, this);
  const SendTransactionResponse* source =
      ::google::protobuf::internal::DynamicCastToGenerated<const SendTransactionResponse>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:taraxa_ledger.SendTransactionResponse)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:taraxa_ledger.SendTransactionResponse)
    MergeFrom(*source);
  }
}

void SendTransactionResponse::MergeFrom(const SendTransactionResponse& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:taraxa_ledger.SendTransactionResponse)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.success() != 0) {
    set_success(from.success());
  }
}

void SendTransactionResponse::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:taraxa_ledger.SendTransactionResponse)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void SendTransactionResponse::CopyFrom(const SendTransactionResponse& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:taraxa_ledger.SendTransactionResponse)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool SendTransactionResponse::IsInitialized() const {
  return true;
}

void SendTransactionResponse::Swap(SendTransactionResponse* other) {
  if (other == this) return;
  InternalSwap(other);
}
void SendTransactionResponse::InternalSwap(SendTransactionResponse* other) {
  using std::swap;
  swap(success_, other->success_);
  _internal_metadata_.Swap(&other->_internal_metadata_);
}

::google::protobuf::Metadata SendTransactionResponse::GetMetadata() const {
  protobuf_proto_2ftransaction_2eproto::protobuf_AssignDescriptorsOnce();
  return ::protobuf_proto_2ftransaction_2eproto::file_level_metadata[kIndexInFileMessages];
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace taraxa_ledger
namespace google {
namespace protobuf {
template<> GOOGLE_PROTOBUF_ATTRIBUTE_NOINLINE ::taraxa_ledger::Transaction* Arena::CreateMaybeMessage< ::taraxa_ledger::Transaction >(Arena* arena) {
  return Arena::CreateInternal< ::taraxa_ledger::Transaction >(arena);
}
template<> GOOGLE_PROTOBUF_ATTRIBUTE_NOINLINE ::taraxa_ledger::SendTransactionResponse* Arena::CreateMaybeMessage< ::taraxa_ledger::SendTransactionResponse >(Arena* arena) {
  return Arena::CreateInternal< ::taraxa_ledger::SendTransactionResponse >(arena);
}
}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)
