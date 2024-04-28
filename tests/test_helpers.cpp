#include <webdav/client.hpp>

bool is_not_found(WebDAV::error err){
  return std::get<WebDAV::http_error>(err).code == 404;
}
