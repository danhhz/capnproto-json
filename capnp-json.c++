// Copyright 2013 Daniel Harrison. All Rights Reserved.

#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.capnp.h>
#include <capnp/schema.h>
#include <capnp/serialize-packed.h>
#include <iostream>

using ::capnp::DynamicValue;
using ::capnp::DynamicStruct;
using ::capnp::DynamicEnum;
using ::capnp::DynamicList;
using ::capnp::List;
using ::capnp::Schema;
using ::capnp::StructSchema;
using ::capnp::EnumSchema;

using ::capnp::Void;
using ::capnp::Text;
using ::capnp::MallocMessageBuilder;
using ::capnp::PackedFdMessageReader;

void dynamicPrintValue(DynamicValue::Reader value) {
  // Print an arbitrary message via the dynamic API by
  // iterating over the schema.  Look at the handling
  // of STRUCT in particular.

  switch (value.getType()) {
    case DynamicValue::VOID:
      std::cout << "null";
      break;
    case DynamicValue::BOOL:
      std::cout << (value.as<bool>() ? "true" : "false");
      break;
    case DynamicValue::INT:
      std::cout << value.as<int64_t>();
      break;
    case DynamicValue::UINT:
      std::cout << value.as<uint64_t>();
      break;
    case DynamicValue::FLOAT:
      std::cout << value.as<double>();
      break;
    case DynamicValue::TEXT:
      std::cout << '\"' << value.as<Text>().cStr() << '\"';
      break;
    case DynamicValue::LIST: {
      std::cout << "[";
      bool first = true;
      for (auto element: value.as<DynamicList>()) {
        if (first) {
          first = false;
        } else {
          std::cout << ", ";
        }
        dynamicPrintValue(element);
      }
      std::cout << "]";
      break;
    }
    case DynamicValue::ENUM: {
      auto enumValue = value.as<DynamicEnum>();
      KJ_IF_MAYBE(enumerant, enumValue.getEnumerant()) {
        std::cout << "\"" << enumerant->getProto().getName().cStr() << "\"";
      } else {
        // Unknown enum value; output raw number.
        std::cout << "\"" << enumValue.getRaw() << "\"";
      }
      break;
    }
    case DynamicValue::STRUCT: {
      std::cout << "{";
      auto structValue = value.as<DynamicStruct>();
      bool first = true;
      KJ_IF_MAYBE(w, structValue.which()) {
        std::cout << "\"which\" : \"" << w->getProto().getName().cStr() << "\", ";
      }
      for (auto field: structValue.getSchema().getFields()) {
        KJ_IF_MAYBE(w, structValue.which()) {
          if (w->getProto().getName() != field.getProto().getName() &&
              !structValue.has(field)) {
            continue;
          }
        } else {
          if (!structValue.has(field)) continue;
        }
        if (first) {
          first = false;
        } else {
          std::cout << ", ";
        }
        std::cout << "\"" << field.getProto().getName().cStr() << "\""
                  << " : ";
        dynamicPrintValue(structValue.get(field));
      }
      std::cout << "}";
      break;
    }
    default:
      // There are other types, we aren't handling them.
      std::cout << "\"?\"";
      break;
  }
}

void dynamicPrintMessage(int fd, StructSchema schema) {
  ::capnp::StreamFdMessageReader message(fd);
  dynamicPrintValue(message.getRoot<DynamicStruct>(schema));
  std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  StructSchema schema = Schema::from<::capnp::schema::CodeGeneratorRequest>();
  dynamicPrintMessage(0, schema);
  return 0;
}

