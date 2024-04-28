/*#***************************************************************************
#                         __    __   _____       _____
#   Project              |  |  |  | |     \     /  ___|
#                        |  |__|  | |  |\  \   /  /
#                        |        | |  | )  ) (  (
#                        |   /\   | |  |/  /   \  \___
#                         \_/  \_/  |_____/     \_____|
#
# Copyright (C) 2018, The WDC Project, <rusdevops@gmail.com>, et al.
#
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the LICENSE file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################*/

#ifndef WEBDAV_REQUEST_HPP
#define WEBDAV_REQUEST_HPP

#include <curl/curl.h>
#include <map>
#include <string>
#include "webdav/client.hpp"

namespace WebDAV
{
  enum OtherError{
    OTHER_OK,
    OTHER_MISSING_HANDLE,
  };
  bool inline check_curl_code(CURLcode code)
  {
    return code == CURLE_OK;
  }
  bool inline check_http_code(long http_code)
  {
    return http_code >= 200 && http_code < 300;
  }
  bool inline check_other_error(OtherError err){
    return err == OTHER_OK;
  }

  using dict_t = std::map<std::string, std::string>;

  struct RequestResult
  {
      CURLcode curlCode;
      OtherError otherError;
      long httpCode;


      bool is_success()
      {
        return check_curl_code(curlCode) && check_http_code(httpCode) && check_other_error(otherError);
      }
      std::optional<WebDAV::error> to_error(){
        if(!check_curl_code(curlCode)) return curl_error{curlCode};
        if(!check_other_error(otherError)) return unspecified_error{};
        if(!check_http_code(httpCode)) return http_error{httpCode};
        return {}; //not an error
      }
  };

  class Request
  {
  public:
    explicit Request(dict_t&& options_);
    Request(const Request& other) = delete;
    Request(Request&& other) noexcept;
    ~Request() noexcept;

    auto operator=(const Request& other) -> Request& = delete;
    auto operator=(Request&& other) noexcept -> Request &;

    template <typename T>
    auto set(CURLoption option, T value) const noexcept -> bool
    {
      if (this->handle == nullptr) return false;
      return check_curl_code(curl_easy_setopt(this->handle, option, value));
    }


    RequestResult perform() const noexcept;
    void* handle;

  private:
    const dict_t options;
    bool proxy_enabled() const noexcept;
    bool cert_required() const noexcept;
    auto swap(Request& other) noexcept -> void;
  };
} // namespace WebDAV

#endif
