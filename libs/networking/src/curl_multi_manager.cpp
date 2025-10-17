#ifdef LD_CURL_NETWORKING

#include "launchdarkly/network/curl_multi_manager.hpp"

#include <boost/asio/post.hpp>

#include <iostream>

namespace launchdarkly::network {
std::shared_ptr<CurlMultiManager> CurlMultiManager::create(
    boost::asio::any_io_executor executor) {
    // Can't use make_shared because constructor is private
    return std::shared_ptr<CurlMultiManager>(
        new CurlMultiManager(std::move(executor)));
}

CurlMultiManager::CurlMultiManager(boost::asio::any_io_executor executor)
    : executor_(std::move(executor)),
      multi_handle_(curl_multi_init(), &curl_multi_cleanup),
      timer_(executor_) {
    if (!multi_handle_) {
        throw std::runtime_error("Failed to initialize CURL multi handle");
    }

    CURLM* pmulti = multi_handle_.get();
    // Set callbacks for socket and timer notifications
    curl_multi_setopt(pmulti, CURLMOPT_SOCKETFUNCTION, socket_callback);
    curl_multi_setopt(pmulti, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(pmulti, CURLMOPT_TIMERFUNCTION, timer_callback);
    curl_multi_setopt(pmulti, CURLMOPT_TIMERDATA, this);
}

CurlMultiManager::~CurlMultiManager() {
    if (multi_handle_) {
        // Remove handles from multi and cleanup resources
        // Do NOT invoke callbacks as they may access destroyed objects
        for (auto& [easy, callback] : callbacks_) {
            curl_multi_remove_handle(multi_handle_.get(), easy);

            // Free headers if they exist for this handle
            if (auto header_it = headers_.find(easy);
                header_it != headers_.end() && header_it->second) {
                curl_slist_free_all(header_it->second);
            }

            curl_easy_cleanup(easy);
        }
    }
}

void CurlMultiManager::add_handle(CURL* easy,
                                  curl_slist* headers,
                                  CompletionCallback callback) {
    {
        std::lock_guard lock(mutex_);
        callbacks_[easy] = std::move(callback);
        headers_[easy] = headers;
    }

    if (const CURLMcode rc = curl_multi_add_handle(multi_handle_.get(), easy);
        rc != CURLM_OK) {
        std::lock_guard lock(mutex_);
        callbacks_.erase(easy);

        // Free headers on error
        if (const auto header_it = headers_.find(easy);
            header_it != headers_.end() && header_it->second) {
            curl_slist_free_all(header_it->second);
        }
        headers_.erase(easy);

        std::cerr << "Failed to add handle to multi: "
            << curl_multi_strerror(rc) << std::endl;
    }
}

void CurlMultiManager::remove_handle(CURL* easy) {
    curl_multi_remove_handle(multi_handle_.get(), easy);

    std::lock_guard lock(mutex_);
    callbacks_.erase(easy);

    // Free headers if they exist
    if (const auto header_it = headers_.find(easy);
        header_it != headers_.end() && header_it->second) {
        curl_slist_free_all(header_it->second);
    }
    headers_.erase(easy);
}

int CurlMultiManager::socket_callback(CURL* easy,
                                      curl_socket_t s,
                                      int what,
                                      void* userp,
                                      void* socketp) {
    auto* manager = static_cast<CurlMultiManager*>(userp);

    std::lock_guard lock(manager->mutex_);

    if (what == CURL_POLL_REMOVE) {
        // Remove socket from managed container
        if (const auto it = manager->sockets_.find(s);
            it != manager->sockets_.end()) {
            manager->sockets_.erase(it);
        }
    } else {
        // Add or update socket in managed container
        auto [it, inserted] = manager->sockets_.try_emplace(
            s, SocketInfo{s, nullptr, 0});
        manager->start_socket_monitor(&it->second, what);
    }

    return 0;
}

int CurlMultiManager::timer_callback(CURLM* multi,
                                     long timeout_ms,
                                     void* userp) {
    auto* manager = static_cast<CurlMultiManager*>(userp);

    // Cancel any existing timer
    manager->timer_.cancel();

    if (timeout_ms > 0) {
        // Set new timer
        manager->timer_.expires_after(std::chrono::milliseconds(timeout_ms));
        manager->timer_.async_wait([weak_self = manager->weak_from_this()](
            const boost::system::error_code& ec) {
                if (!ec) {
                    if (const auto self = weak_self.lock()) {
                        self->handle_timeout();
                    }
                }
            });
    } else if (timeout_ms == 0) {
        // Call socket_action immediately
        boost::asio::post(manager->executor_,
                          [weak_self = manager->weak_from_this()]() {
                              if (const auto self = weak_self.lock()) {
                                  self->handle_timeout();
                              }
                          });
    }
    // If timeout_ms < 0, no timeout (delete timer)

    return 0;
}

void CurlMultiManager::handle_socket_action(curl_socket_t s,
                                            const int event_bitmask) {
    int running_handles = 0;
    const CURLMcode rc = curl_multi_socket_action(multi_handle_.get(), s,
                                                  event_bitmask,
                                                  &running_handles);

    if (rc != CURLM_OK) {
        std::cerr << "curl_multi_socket_action failed: "
            << curl_multi_strerror(rc) << std::endl;
    }

    check_multi_info();

    if (running_handles != still_running_) {
        still_running_ = running_handles;
    }
}

void CurlMultiManager::handle_timeout() {
    handle_socket_action(CURL_SOCKET_TIMEOUT, 0);
}

void CurlMultiManager::check_multi_info() {
    int msgs_in_queue;
    CURLMsg* msg;

    while ((msg = curl_multi_info_read(multi_handle_.get(), &msgs_in_queue))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            CURLcode result = msg->data.result;

            CompletionCallback callback;
            curl_slist* headers = nullptr;
            {
                std::lock_guard lock(mutex_);
                if (auto it = callbacks_.find(easy); it != callbacks_.end()) {
                    callback = std::move(it->second);
                    callbacks_.erase(it);
                }

                // Get and remove headers
                if (auto header_it = headers_.find(easy);
                    header_it != headers_.end()) {
                    headers = header_it->second;
                    headers_.erase(header_it);
                }
            }

            // Remove from multi handle
            curl_multi_remove_handle(multi_handle_.get(), easy);

            // Free headers
            if (headers) {
                curl_slist_free_all(headers);
            }

            // Invoke completion callback
            if (callback) {
                boost::asio::post(executor_, [callback = std::move(callback),
                                      easy, result]() {
                                      callback(easy, result);
                                  });
            }
        }
    }
}

void CurlMultiManager::start_socket_monitor(SocketInfo* socket_info,
                                            const int action) {
    if (!socket_info->handle) {
        // Create tcp::socket and assign the native socket handle
        // This works cross-platform (Windows and POSIX)
        socket_info->handle = std::make_shared<SocketHandle>(executor_);

        // Assign the CURL socket to the ASIO socket
        // tcp::socket::assign works with native socket handles on both platforms
        boost::system::error_code ec;
        socket_info->handle->assign(boost::asio::ip::tcp::v4(), socket_info->sockfd, ec);

        if (ec) {
            std::cerr << "Failed to assign socket: " << ec.message() << std::endl;
            socket_info->handle.reset();
            return;
        }
    }

    // Check if action has changed
    const bool action_changed = (socket_info->action != action);
    socket_info->action = action;

    auto weak_self = weak_from_this();
    curl_socket_t sockfd = socket_info->sockfd;

    // Monitor for read events
    if (action & CURL_POLL_IN) {
        // Only create new handler if we don't have one or if action changed
        if (!socket_info->read_handler || action_changed) {
            // Use weak_ptr to safely detect when handle is deleted
            std::weak_ptr<SocketHandle> weak_handle = socket_info->handle;

            // Create and store handler in SocketInfo to keep it alive
            // Use weak_ptr in capture to avoid circular reference
            socket_info->read_handler = std::make_shared<std::function<void()>>();
            std::weak_ptr<std::function<void()>> weak_read_handler = socket_info->read_handler;
            *socket_info->read_handler = [weak_self, sockfd, weak_handle, weak_read_handler]() {
                // Check if manager and handle are still valid
                const auto self = weak_self.lock();
                const auto handle = weak_handle.lock();
                if (!self || !handle) {
                    return;
                }

                handle->async_wait(
                    boost::asio::ip::tcp::socket::wait_read,
                    [weak_self, sockfd, weak_handle, weak_read_handler](
                        const boost::system::error_code& ec) {
                        // If operation was canceled or had an error, don't re-register
                        if (ec) {
                            return;
                        }

                        if (const auto self = weak_self.lock()) {
                            self->handle_socket_action(sockfd, CURL_CSELECT_IN);

                            // Always try to re-register for continuous monitoring
                            // The validity check at the top of read_handler will stop it if needed
                            if (const auto handler = weak_read_handler.lock()) {
                                (*handler)(); // Recursive call
                            }
                        }
                    });
            };
            (*socket_info->read_handler)(); // Initial call
        }
    }

    // Monitor for write events
    if (action & CURL_POLL_OUT) {
        // Only create new handler if we don't have one or if action changed
        if (!socket_info->write_handler || action_changed) {
            // Use weak_ptr to safely detect when handle is deleted
            std::weak_ptr<SocketHandle> weak_handle = socket_info->handle;

            // Create and store handler in SocketInfo to keep it alive
            // Use weak_ptr in capture to avoid circular reference
            socket_info->write_handler = std::make_shared<std::function<void()>>();
            std::weak_ptr<std::function<void()>> weak_write_handler = socket_info->write_handler;
            *socket_info->write_handler = [weak_self, sockfd, weak_handle, weak_write_handler]() {
                // Check if manager and handle are still valid
                const auto self = weak_self.lock();
                const auto handle = weak_handle.lock();
                if (!self || !handle) {
                    return;
                }

                handle->async_wait(
                    boost::asio::ip::tcp::socket::wait_write,
                    [weak_self, sockfd, weak_handle, weak_write_handler](
                        const boost::system::error_code& ec) {
                        // If operation was canceled or had an error, don't re-register
                        if (ec) {
                            return;
                        }

                        if (const auto self = weak_self.lock()) {
                            self->handle_socket_action(sockfd, CURL_CSELECT_OUT);

                            // Always try to re-register for continuous monitoring
                            // The validity check at the top of write_handler will stop it if needed
                            if (const auto handler = weak_write_handler.lock()) {
                                (*handler)(); // Recursive call
                            }
                        }
                    });
            };
            (*socket_info->write_handler)(); // Initial call
        }
    }
}
} // namespace launchdarkly::network

#endif  // LD_CURL_NETWORKING