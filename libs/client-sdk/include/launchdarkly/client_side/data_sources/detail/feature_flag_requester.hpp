#pragma once

#include <future>
#include <string>
#include <map>


class HttpResult {
   public:
    using StatusCode = uint64_t;
    using HeadersType = std::map<std::string, std::string>;

    StatusCode Status() const;
    std::string Body() const;
    HeadersType Headers() const;

    HttpResult(StatusCode status, std::string body, HeadersType headers) :
 status_(status), body_(std::move(body)), headers_(std::move(headers)){}

   private:
    StatusCode status_;
    std::string body_;
    HeadersType headers_;
};

class IHttpRequester {

};

class IFeatureFlagRequester {
};