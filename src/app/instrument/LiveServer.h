#pragma once

#include "Command.h"
#include "ScriptRunner.h"

#include <atomic>
#include <deque>
#include <future>
#include <mutex>
#include <string>
#include <thread>

namespace planets::app::instrument
{

// A command plus a promise the render thread fulfills once the command is dispatched (with
// any result string, e.g. a written PNG path), so the transport can reply 1:1.
struct LiveItem
{
    Command cmd;
    std::shared_ptr<std::promise<std::string>> done;
};

// Thread-safe command queue: a background transport thread pushes, the render thread drains.
// The live counterpart to TimelineSource — same Command, same dispatch, different producer.
class LiveCommandSource : public ICommandSource
{
public:
    // Transport thread: enqueue a command; returns the future the caller waits on for the
    // render thread's response.
    std::future<std::string> Push(Command cmd);

    // Render thread: pop one item (non-blocking). Returns false when empty so the loop can
    // render and poll the window. The popped item's promise must be fulfilled via Respond.
    bool NextItem(LiveItem& out);

    // ICommandSource: not used by the live loop (it uses NextItem to keep the promise); kept
    // so a LiveCommandSource still satisfies the interface.
    bool Next(Command& out) override;

    void Close();
    bool IsClosed() const { return _closed.load(); }

private:
    std::mutex _mutex;
    std::deque<LiveItem> _queue;
    std::atomic<bool> _closed{false};
};

// Localhost TCP line server (Winsock): parses one JSON command per line, pushes it, and
// writes back the render thread's response line. Single client at a time. Parses + enqueues
// only — the render thread owns all GL/Application mutation.
class LiveServer
{
public:
    explicit LiveServer(LiveCommandSource& source)
        : _source(source)
    {
    }
    ~LiveServer();

    bool Start(unsigned short port);
    void Stop();

private:
    void Serve();

    LiveCommandSource& _source;
    std::thread _thread;
    std::atomic<bool> _running{false};
    unsigned short _port = 0;
    unsigned long long _listenSock = ~0ull; // SOCKET, kept opaque to avoid <winsock2.h> here
};

} // namespace planets::app::instrument
