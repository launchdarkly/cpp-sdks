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
      multi_handle_(curl_multi_init()),
      timer_(executor_) {

    if (!multi_handle_) {
        throw std::runtime_error("Failed to initialize CURL multi handle");
    }

    // Set callbacks for socket and timer notifications
    curl_multi_setopt(multi_handle_, CURLMOPT_SOCKETFUNCTION, socket_callback);
    curl_multi_setopt(multi_handle_, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(multi_handle_, CURLMOPT_TIMERFUNCTION, timer_callback);
    curl_multi_setopt(multi_handle_, CURLMOPT_TIMERDATA, this);
}

CurlMultiManager::~CurlMultiManager() {
    if (multi_handle_) {
        curl_multi_cleanup(multi_handle_);
    }
}

void CurlMultiManager::add_handle(CURL* easy, CompletionCallback callback) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_[easy] = std::move(callback);
    }

    CURLMcode rc = curl_multi_add_handle(multi_handle_, easy);
    if (rc != CURLM_OK) {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.erase(easy);
        std::cerr << "Failed to add handle to multi: "
                  << curl_multi_strerror(rc) << std::endl;
    }
}

void CurlMultiManager::remove_handle(CURL* easy) {
    curl_multi_remove_handle(multi_handle_, easy);

    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.erase(easy);
}

int CurlMultiManager::socket_callback(CURL* easy, curl_socket_t s, int what,
                                     void* userp, void* socketp) {
    auto* manager = static_cast<CurlMultiManager*>(userp);
    auto* socket_info = static_cast<SocketInfo*>(socketp);

    if (what == CURL_POLL_REMOVE) {
        if (socket_info) {
            manager->stop_socket_monitor(socket_info);
            curl_multi_assign(manager->multi_handle_, s, nullptr);
            delete socket_info;
        }
    } else {
        if (!socket_info) {
            // New socket
            socket_info = new SocketInfo{s, nullptr, 0};
            curl_multi_assign(manager->multi_handle_, s, socket_info);
        }

        manager->start_socket_monitor(socket_info, what);
    }

    return 0;
}

int CurlMultiManager::timer_callback(CURLM* multi, long timeout_ms, void* userp) {
    auto* manager = static_cast<CurlMultiManager*>(userp);

    // Cancel any existing timer
    manager->timer_.cancel();

    if (timeout_ms > 0) {
        // Set new timer
        manager->timer_.expires_after(std::chrono::milliseconds(timeout_ms));
        manager->timer_.async_wait([weak_self = manager->weak_from_this()](
            const boost::system::error_code& ec) {
            if (!ec) {
                if (auto self = weak_self.lock()) {
                    self->handle_timeout();
                }
            }
        });
    } else if (timeout_ms == 0) {
        // Call socket_action immediately
        boost::asio::post(manager->executor_, [weak_self = manager->weak_from_this()]() {
            if (auto self = weak_self.lock()) {
                self->handle_timeout();
            }
        });
    }
    // If timeout_ms < 0, no timeout (delete timer)

    return 0;
}

void CurlMultiManager::handle_socket_action(curl_socket_t s, int event_bitmask) {
    int running_handles = 0;
    CURLMcode rc = curl_multi_socket_action(multi_handle_, s, event_bitmask,
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

    while ((msg = curl_multi_info_read(multi_handle_, &msgs_in_queue))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            CURLcode result = msg->data.result;

            CompletionCallback callback;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = callbacks_.find(easy);
                if (it != callbacks_.end()) {
                    callback = std::move(it->second);
                    callbacks_.erase(it);
                }
            }

            // Remove from multi handle
            curl_multi_remove_handle(multi_handle_, easy);

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

void CurlMultiManager::start_socket_monitor(SocketInfo* socket_info, int action) {
    if (!socket_info->descriptor) {
        // Create descriptor for this socket
        socket_info->descriptor = std::make_unique<
            boost::asio::posix::stream_descriptor>(executor_);
        socket_info->descriptor->assign(socket_info->sockfd);
    }

    socket_info->action = action;

    auto weak_self = weak_from_this();
    curl_socket_t sockfd = socket_info->sockfd;

    // Monitor for read events
    if (action & CURL_POLL_IN) {
        socket_info->descriptor->async_wait(
            boost::asio::posix::stream_descriptor::wait_read,
            [weak_self, sockfd](const boost::system::error_code& ec) {
                if (!ec) {
                    if (auto self = weak_self.lock()) {
                        self->handle_socket_action(sockfd, CURL_CSELECT_IN);
                    }
                }
            });
    }

    // Monitor for write events
    if (action & CURL_POLL_OUT) {
        socket_info->descriptor->async_wait(
            boost::asio::posix::stream_descriptor::wait_write,
            [weak_self, sockfd](const boost::system::error_code& ec) {
                if (!ec) {
                    if (auto self = weak_self.lock()) {
                        self->handle_socket_action(sockfd, CURL_CSELECT_OUT);
                    }
                }
            });
    }
}

void CurlMultiManager::stop_socket_monitor(SocketInfo* socket_info) {
    if (socket_info->descriptor) {
        socket_info->descriptor->cancel();
        socket_info->descriptor->release();
        socket_info->descriptor.reset();
    }
}

} // namespace launchdarkly::network

#endif  // LD_CURL_NETWORKING
