#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <nlohmann/json.hpp>

#include <string>

/**
 * Synchronously POSTs a JSON payload to a test harness callback URI. Errors
 * are ignored. Blocks the calling thread until the response is read, so
 * sequential calls arrive at the harness in order.
 * @param executor Executor used to construct the socket.
 * @param callback_uri The URI to post to.
 * @param payload The JSON body.
 */
void PostJsonCallback(boost::asio::any_io_executor executor,
                      std::string const& callback_uri,
                      nlohmann::json const& payload);
