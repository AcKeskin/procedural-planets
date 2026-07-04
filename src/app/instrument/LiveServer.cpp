#include "LiveServer.h"

#include "TimelineParser.h"

#include <nlohmann/json.hpp>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

namespace planets::app::instrument
{

std::future<std::string> LiveCommandSource::Push(Command cmd)
{
    auto promise = std::make_shared<std::promise<std::string>>();
    std::future<std::string> fut = promise->get_future();
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push_back(LiveItem{std::move(cmd), std::move(promise)});
    }
    return fut;
}

bool LiveCommandSource::NextItem(LiveItem& out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_queue.empty())
        return false;
    out = std::move(_queue.front());
    _queue.pop_front();
    return true;
}

bool LiveCommandSource::Next(Command& out)
{
    LiveItem item;
    if (!NextItem(item))
        return false;
    out = std::move(item.cmd);
    if (item.done)
        item.done->set_value("ok");
    return true;
}

void LiveCommandSource::Close()
{
    _closed.store(true);
}

LiveServer::~LiveServer()
{
    Stop();
}

bool LiveServer::Start(unsigned short port)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cerr << "[Live] WSAStartup failed" << std::endl;
        return false;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
    {
        std::cerr << "[Live] socket() failed" << std::endl;
        WSACleanup();
        return false;
    }

    BOOL yes = TRUE;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        std::cerr << "[Live] bind() failed on port " << port << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return false;
    }
    if (listen(listenSock, 1) == SOCKET_ERROR)
    {
        std::cerr << "[Live] listen() failed" << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return false;
    }

    _listenSock = listenSock;
    _port = port;
    _running.store(true);
    _thread = std::thread(&LiveServer::Serve, this);
    std::cout << "[Live] listening on 127.0.0.1:" << port << std::endl;
    return true;
}

void LiveServer::Stop()
{
    if (!_running.exchange(false))
        return;
    _source.Close();
    if (_listenSock != ~0ull)
    {
        closesocket(static_cast<SOCKET>(_listenSock));
        _listenSock = ~0ull;
    }
    if (_thread.joinable())
        _thread.join();
    WSACleanup();
}

void LiveServer::Serve()
{
    const SOCKET listenSock = static_cast<SOCKET>(_listenSock);

    while (_running.load())
    {
        SOCKET client = accept(listenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET)
        {
            if (!_running.load())
                break;
            continue;
        }
        std::cout << "[Live] client connected" << std::endl;

        // Line-buffered: parse each newline-terminated JSON command, enqueue, wait for the
        // render thread, then reply with the response line.
        std::string buffer;
        char chunk[4096];
        bool clientOpen = true;
        while (clientOpen && _running.load())
        {
            const int n = recv(client, chunk, sizeof(chunk), 0);
            if (n <= 0)
                break;
            buffer.append(chunk, n);

            size_t nl;
            while ((nl = buffer.find('\n')) != std::string::npos)
            {
                std::string line = buffer.substr(0, nl);
                buffer.erase(0, nl + 1);
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (line.empty())
                    continue;

                std::string reply;
                try
                {
                    auto j = nlohmann::json::parse(line);
                    Command cmd;
                    if (ParseCommand(j, cmd))
                    {
                        auto fut = _source.Push(std::move(cmd));
                        reply = fut.get(); // blocks until the render thread responds
                    }
                    else
                    {
                        reply = "error: could not parse command";
                    }
                }
                catch (const std::exception& e)
                {
                    reply = std::string("error: ") + e.what();
                }

                reply.push_back('\n');
                if (send(client, reply.data(), static_cast<int>(reply.size()), 0) == SOCKET_ERROR)
                {
                    clientOpen = false;
                    break;
                }
            }
        }

        closesocket(client);
        std::cout << "[Live] client disconnected" << std::endl;
    }
}

} // namespace planets::app::instrument
