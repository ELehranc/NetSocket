#pragma once

#include <memory>
#include <functional>

class Buffer;
class Connection;
class Timestamp;

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionCallback = std::function<void(const ConnectionPtr &)>;
using CloseCallback = std::function<void(const ConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(ConnectionPtr &)>;

using MessageCallback = std::function<void(const ConnectionPtr &, Buffer *, Timestamp)>;
using HighWaterMarkCallback = std::function<void(const ConnectionPtr, size_t)>;