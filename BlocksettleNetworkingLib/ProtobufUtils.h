#ifndef PROTOBUF_UTILS_H
#define PROTOBUF_UTILS_H

#include <google/protobuf/message.h>

#include <disable_warnings.h>
#include <spdlog/logger.h>
#include <enable_warnings.h>

class ProtobufUtils
{
public:
   static std::string toJson(const google::protobuf::Message &msg, bool addWhitespace = true);
   static std::string toJsonReadable(const google::protobuf::Message &msg);
   static std::string toJsonCompact(const google::protobuf::Message &msg);
   static std::string pbMessageToString(const google::protobuf::Message& msg);
};

#endif
