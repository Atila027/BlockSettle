#ifndef __ZEROMQ_CONTEXT_H__
#define __ZEROMQ_CONTEXT_H__

#include <string>
#include <memory>

#include "IdStringGenerator.h"

namespace spdlog
{
   class logger;
}


class ZmqContext
{
private:
   using ctx_ptr = std::unique_ptr<void, int (*)(void*)>;

public:
   using sock_ptr = std::unique_ptr<void, int (*)(void*)>;

public:
   ZmqContext(const std::shared_ptr<spdlog::logger>& logger);
   ~ZmqContext() noexcept = default;

   ZmqContext(const ZmqContext&) = delete;
   ZmqContext& operator = (const ZmqContext&) = delete;

   ZmqContext(ZmqContext&&) = delete;
   ZmqContext& operator = (ZmqContext&&) = delete;

   std::string GenerateConnectionName(const std::string& host, const std::string& port);
public:
   sock_ptr    CreateInternalControlSocket();
   sock_ptr    CreateMonitorSocket();
   sock_ptr    CreateStreamSocket();

   sock_ptr    CreateServerSocket();
   sock_ptr    CreateClientSocket();

   static sock_ptr  CreateNullSocket();

private:
   std::shared_ptr<spdlog::logger> logger_;

   ctx_ptr                          context_;

   IdStringGenerator                idGenerator_;
};

#endif // __ZEROMQ_CONTEXT_H__
