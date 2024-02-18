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

#include <webdav/client.hpp>

#include "callback.hpp"
#include "fsinfo.hpp"
#include "header.hpp"
#include "pugiext.hpp"
#include "request.hpp"
#include "urn.hpp"
#include "utils.hpp"

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <thread>
#include <optional>
#include <chrono>

namespace WebDAV
{
  auto inline get(const dict_t& options, const std::string&& name) -> std::string
  {
    auto it = options.find(name);
    if (it == options.end()) return "";
    else return it->second;
  }

  using Urn::Path;

  using progress_funptr = int(*)(void* context, size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow);

  dict_t
  Client::options() const
  {
    return dict_t
    {
      { "webdav_hostname", this->webdav_hostname },
      { "webdav_root", this->webdav_root },
      { "webdav_username", this->webdav_username },
      { "webdav_password", this->webdav_password },
      { "proxy_hostname", this->proxy_hostname },
      { "proxy_username", this->proxy_username },
      { "proxy_password", this->proxy_password },
      { "cert_path", this->cert_path },
      { "key_path", this->key_path },
    };
  }

  bool
  Client::sync_download(
    const std::string& remote_file,
    const std::string& local_file,
    callback_t callback,
    progress_t progress
  ) const
  {
    bool is_existed = this->check(remote_file);
    if (!is_existed) return false;

    auto root_urn = Path(this->webdav_root, true);
    auto file_urn = root_urn + remote_file;

    std::ofstream file_stream(local_file, std::ios::binary);

    Request request(this->options());

    auto url = this->webdav_hostname + file_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "GET");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HEADER, 0L);
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&file_stream));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Write::stream));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif
    if (progress != nullptr)
    {
      request.set(CURLOPT_XFERINFOFUNCTION, reinterpret_cast<size_t>(progress.target<progress_funptr>()));
      request.set(CURLOPT_NOPROGRESS, 0L);
    }

    bool is_performed = request.perform();

    if (callback != nullptr) callback(is_performed);
    return is_performed;
  }

  bool
  Client::sync_download_to(
    const std::string& remote_file,
    char*& buffer_ptr,
    unsigned long long& buffer_size,
    callback_t callback,
    progress_t progress
  ) const
  {
    bool is_existed = this->check(remote_file);
    if (!is_existed) return false;

    auto root_urn = Path(this->webdav_root, true);
    auto file_urn = root_urn + remote_file;

    Data data = { nullptr, 0, 0 };

    Request request(this->options());

    auto url = this->webdav_hostname + file_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "GET");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HEADER, 0L);
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&data));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif
    if (progress != nullptr)
    {
      request.set(CURLOPT_XFERINFOFUNCTION, reinterpret_cast<size_t>(progress.target<progress_funptr>()));
      request.set(CURLOPT_NOPROGRESS, 0L);
    }

    bool is_performed = request.perform();
    if (callback != nullptr) callback(is_performed);
    if (!is_performed) return false;

    buffer_ptr = data.buffer;
    buffer_size = data.size;
    data.reset();
    return true;
  }

  bool
  Client::sync_download_to(
    const std::string& remote_file,
    std::ostream& stream,
    callback_t callback,
    progress_t progress
  ) const
  {
    bool is_existed = this->check(remote_file);
    if (!is_existed) return false;

    auto root_urn = Path(this->webdav_root, true);
    auto file_urn = root_urn + remote_file;

    Request request(this->options());

    auto url = this->webdav_hostname + file_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "GET");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HEADER, 0L);
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&stream));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Write::stream));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif
    if (progress != nullptr)
    {
      request.set(CURLOPT_XFERINFOFUNCTION, reinterpret_cast<size_t>(progress.target<progress_funptr>()));
      request.set(CURLOPT_NOPROGRESS, 0L);
    }

    bool is_performed = request.perform();
    if (callback != nullptr) callback(is_performed);

    return is_performed;
  }

  bool
  Client::sync_upload(
    const std::string& remote_file,
    const std::string& local_file,
    callback_t callback,
    progress_t progress
  ) const
  {
    bool is_existed = FileInfo::exists(local_file);
    if (!is_existed) return false;

    auto root_urn = Path(this->webdav_root, true);
    auto file_urn = root_urn + remote_file;

    std::ifstream file_stream(local_file, std::ios::binary);
    auto size = FileInfo::size(local_file);

    Request request(this->options());

    auto url = this->webdav_hostname + file_urn.quote(request.handle);

    Data response = { nullptr, 0, 0 };

    request.set(CURLOPT_UPLOAD, 1L);
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_READDATA, reinterpret_cast<size_t>(&file_stream));
    request.set(CURLOPT_READFUNCTION, reinterpret_cast<size_t>(Callback::Read::stream));
    request.set(CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(size));
    request.set(CURLOPT_BUFFERSIZE, static_cast<long>(Client::buffer_size));
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&response));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif
    if (progress != nullptr)
    {
      request.set(CURLOPT_XFERINFOFUNCTION, reinterpret_cast<size_t>(progress.target<progress_funptr>()));
      request.set(CURLOPT_NOPROGRESS, 0L);
    }

    bool is_performed = request.perform();

    if (callback != nullptr) callback(is_performed);
    return is_performed;
  }

  bool
  Client::sync_upload_from(
    const std::string& remote_file,
    char* buffer_ptr,
    unsigned long long buffer_size,
    callback_t callback,
    progress_t progress
  ) const
  {
    auto root_urn = Path(this->webdav_root, true);
    auto file_urn = root_urn + remote_file;

    Data data = { buffer_ptr, 0, buffer_size };

    Request request(this->options());

    auto url = this->webdav_hostname + file_urn.quote(request.handle);

    Data response = { nullptr, 0, 0 };

    request.set(CURLOPT_UPLOAD, 1L);
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_READDATA, reinterpret_cast<size_t>(&data));
    request.set(CURLOPT_READFUNCTION, reinterpret_cast<size_t>(Callback::Read::buffer));
    request.set(CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(buffer_size));
    request.set(CURLOPT_BUFFERSIZE, static_cast<long>(Client::buffer_size));
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&response));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif
    if (progress != nullptr)
    {
      request.set(CURLOPT_XFERINFOFUNCTION, reinterpret_cast<size_t>(progress.target<progress_funptr>()));
      request.set(CURLOPT_NOPROGRESS, 0L);
    }

    bool is_performed = request.perform();

    if (callback != nullptr) callback(is_performed);

    data.reset();
    return is_performed;
  }

  bool
  Client::sync_upload_from(
    const std::string& remote_file,
    std::istream& stream,
    callback_t callback,
    progress_t progress
  ) const
  {
    auto root_urn = Path(this->webdav_root, true);
    auto file_urn = root_urn + remote_file;

    Request request(this->options());

    auto url = this->webdav_hostname + file_urn.quote(request.handle);
    stream.seekg(0, std::ios::end);
    size_t stream_size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    Data response = { nullptr, 0, 0 };

    request.set(CURLOPT_UPLOAD, 1L);
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_READDATA, reinterpret_cast<size_t>(&stream));
    request.set(CURLOPT_READFUNCTION, reinterpret_cast<size_t>(Callback::Read::stream));
    request.set(CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(stream_size));
    request.set(CURLOPT_BUFFERSIZE, static_cast<long>(Client::buffer_size));
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&response));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif
    if (progress != nullptr)
    {
      request.set(CURLOPT_XFERINFOFUNCTION, reinterpret_cast<size_t>(progress.target<progress_funptr>()));
      request.set(CURLOPT_NOPROGRESS, 0L);
    }

    bool is_performed = request.perform();

    if (callback != nullptr) callback(is_performed);
    return is_performed;
  }

  Client::Client(const dict_t& options)
  {
    this->webdav_hostname = get(options, "webdav_hostname");
    this->webdav_root = get(options, "webdav_root");
    this->webdav_username = get(options, "webdav_username");
    this->webdav_password = get(options, "webdav_password");

    this->proxy_hostname = get(options, "proxy_hostname");
    this->proxy_username = get(options, "proxy_username");
    this->proxy_password = get(options, "proxy_password");

    this->cert_path = get(options, "cert_path");
    this->key_path = get(options, "key_path");
  }

  unsigned long long
  Client::free_size() const
  {
    Header header =
    {
      "Accept: */*",
      "Depth: 0",
      "Content-Type: text/xml"
    };

    pugi::xml_document document;
    auto propfind = document.append_child("D:propfind");
    propfind.append_attribute("xmlns:D") = "DAV:";

    auto prop = propfind.append_child("D:prop");
    prop.append_child("D:quokta-available-bytes");
    prop.append_child("D:quota-used-bytes");

    auto document_print = pugi::node_to_string(document);
    size_t size = document_print.length() * sizeof((document_print.c_str())[0]);

    Data data = { nullptr, 0, 0 };

    Request request(this->options());

    request.set(CURLOPT_CUSTOMREQUEST, "PROPFIND");
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<struct curl_slist*>(header.handle));
    request.set(CURLOPT_POSTFIELDS, document_print.c_str());
    request.set(CURLOPT_POSTFIELDSIZE, static_cast<long>(size));
    request.set(CURLOPT_HEADER, 0);
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&data));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif

    auto is_performed = request.perform();
    if (!is_performed) return 0;

    document.load_buffer(data.buffer, static_cast<size_t>(data.size));

    pugi::xml_node multistatus = document.select_node("*[local-name()='multistatus']").node();
    pugi::xml_node response = multistatus.select_node("*[local-name()='response']").node();
    pugi::xml_node propstat = response.select_node("*[local-name()='propstat']").node();
    prop = propstat.select_node("*[local-name()='prop']").node();
    pugi::xml_node quota_available_bytes = prop.select_node("*[local-name()='quota-available-bytes']").node();
    std::string free_size_text = quota_available_bytes.first_child().value();

    return boost::lexical_cast<unsigned long long>(free_size_text);
  }

  bool
  Client::check(const std::string& remote_resource) const
  {
    auto root_urn = Path(this->webdav_root, true);
    auto resource_urn = root_urn + remote_resource;

    Header header =
    {
      "Accept: */*",
      "Depth: 1"
    };

    Data data = { nullptr, 0, 0 };

    Request request(this->options());

    auto url = this->webdav_hostname + resource_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "PROPFIND");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<curl_slist*>(header.handle));
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&data));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif

    return request.perform();
  }

  std::optional<resource> resource_from_xml_node(const pugi::xml_node &node){
      //todo handle errors
      pugi::xml_node href = node.select_node("*[local-name()='href']").node();
      std::string encode_file_name = href.first_child().value();
      std::string resource_path = curl_unescape(encode_file_name.c_str(), static_cast<int>(encode_file_name.length()));

      auto propstat = node.select_node("*[local-name()='propstat']").node();
      auto prop = propstat.select_node("*[local-name()='prop']").node();
      auto creation_date = prop.select_node("*[local-name()='creationdate']").node();
      auto display_name = prop.select_node("*[local-name()='displayname']").node();
      auto content_length = prop.select_node("*[local-name()='getcontentlength']").node();
      auto modified_date = prop.select_node("*[local-name()='getlastmodified']").node();
      auto resource_type = prop.select_node("*[local-name()='resourcetype']").node();
      auto etag = prop.select_node("*[local-name()='getetag']").node();

      std::optional<long long> size = {};
      try {
          size = std::stoll(content_length.text().get());
      }catch (std::invalid_argument &ignored){}

      // todo none instead of empty strings if tag missing

      resource resource{
              .href = href.text().get(), //todo remove root_urn prefix
              .display_name =  std::string{display_name.text().get()}, //todo does value do whai I want
              .size = size,
              .modified = utils::parse_tp_rfc2616(modified_date.text().get()),
              .created = utils::parse_tp_rfc2616(creation_date.text().get()),
              .type = resource_type.first_child().name(), //todo error check
              .etag = etag.text().get()
      };

      return resource;
  }

  std::optional<resource>
  Client::info(const std::string& remote_resource) const
  {
    auto root_urn = Path(this->webdav_root, true);
    auto target_urn = root_urn + remote_resource;

    Header header =
    {
      "Accept: */*",
      "Depth: 0"
    };

    Data data = { nullptr, 0, 0 };

    Request request(this->options());

    auto url = this->webdav_hostname + target_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "PROPFIND");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<curl_slist*>(header.handle));
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&data));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif
    bool is_performed = request.perform();

    if (!is_performed) return {};

    pugi::xml_document document;
    document.load_buffer(data.buffer, static_cast<size_t>(data.size));
#ifdef WDC_VERBOSE
    document.save(std::cout);
#endif
    auto multistatus = document.select_node("*[local-name()='multistatus']").node();
    auto responses = multistatus.select_nodes("*[local-name()='response']");
    for (auto response : responses)
    {
      auto resource_opt = resource_from_xml_node(response.node());
      if (!resource_opt.has_value()) continue;
      resource res = resource_opt.value();
      std::string target_path = target_urn.path();
      std::string target_path_without_sep = target_path;
      if (!target_path_without_sep.empty() && target_path_without_sep.back() == '/')
        target_path_without_sep.resize(target_path_without_sep.length() - 1);
      std::string resource_path_without_sep = res.href;
      if (!resource_path_without_sep.empty() && resource_path_without_sep.back() == '/')
        resource_path_without_sep.resize(resource_path_without_sep.length() - 1);
      if (resource_path_without_sep == target_path_without_sep)
      {
        return res;
      }
    }

    return {};
  }

  bool
  Client::is_directory(const std::string& remote_resource) const
  {
    auto information = this->info(remote_resource);
    std::optional<std::string> resource_type = information.has_value() ? information->type : std::optional<std::string>{};
    bool is_dir = resource_type.has_value() && (resource_type.value() == "d:collection" || resource_type.value() == "D:collection");
    return is_dir;
  }

  std::optional<resources_t>
  Client::list(const std::string& remote_directory) const
  {
    auto target_urn = Path(this->webdav_root, true) + remote_directory;
    target_urn = Path(target_urn.path(), true);

    Header header =
    {
      "Accept: */*",
      "Depth: 1"
    };

    Data data = { nullptr, 0, 0 };

    Request request(this->options());

    auto url = this->webdav_hostname + target_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "PROPFIND");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<curl_slist*>(header.handle));
    request.set(CURLOPT_HEADER, 0);
    request.set(CURLOPT_WRITEDATA, reinterpret_cast<size_t>(&data));
    request.set(CURLOPT_WRITEFUNCTION, reinterpret_cast<size_t>(Callback::Append::buffer));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif

    bool is_performed = request.perform();

    if (!is_performed) return {};

    resources_t resources;

    pugi::xml_document document;
    document.load_buffer(data.buffer, static_cast<size_t>(data.size));
    auto multistatus = document.select_node("*[local-name()='multistatus']").node();
    auto responses = multistatus.select_nodes("*[local-name()='response']");
    for (auto response : responses)
    {
      auto res = resource_from_xml_node(response.node());
      if (!res.has_value()) continue;
      resources.emplace_back(std::move(res.value()));
    }

    return resources;
  }

  bool Client::download(
    const std::string& remote_file,
    const std::string& local_file,
    progress_t progress
  ) const
  {
    return this->sync_download(remote_file, local_file, nullptr, std::move(progress));
  }

  void
  Client::async_download(
    const std::string& remote_file,
    const std::string& local_file,
    callback_t callback,
    progress_t progress
  ) const
  {
    std::thread downloading([ = ]()
    {
      this->sync_download(remote_file, local_file, callback, std::move(progress));
    });
    downloading.detach();
  }

  bool
  Client::download_to(
    const std::string& remote_file,
    char*& buffer_ptr,
    unsigned long long& buffer_size,
    progress_t progress
  ) const
  {
    return this->sync_download_to(remote_file, buffer_ptr, buffer_size, nullptr, std::move(progress));
  }

  bool
  Client::download_to(
    const std::string& remote_file,
    std::ostream& stream,
    progress_t progress
  ) const
  {
    return this->sync_download_to(remote_file, stream, nullptr, std::move(progress));
  }

  bool
  Client::create_directory(const std::string& remote_directory, bool recursive) const
  {
    bool is_existed = this->check(remote_directory);
    if (is_existed) return true;

    bool resource_is_dir = true;
    Path directory_urn(remote_directory, resource_is_dir);

    if (recursive)
    {
      auto remote_parent_directory = directory_urn.parent().path();
      if (remote_parent_directory == remote_directory) return false;
      bool is_created = this->create_directory(remote_parent_directory, true);
      if (!is_created) return false;
    }

    Header header =
    {
      "Accept: */*",
      "Connection: Keep-Alive"
    };

    auto target_urn = Path(this->webdav_root, true) + remote_directory;
    target_urn = Path(target_urn.path(), true);

    Request request(this->options());

    auto url = this->webdav_hostname + target_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "MKCOL");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<curl_slist*>(header.handle));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif

    return request.perform();
  }

  bool
  Client::move(const std::string& remote_source_resource, const std::string& remote_destination_resource) const
  {
    bool is_existed = this->check(remote_source_resource);
    if (!is_existed) return false;

    Path root_urn(this->webdav_root, true);

    auto source_resource_urn = root_urn + remote_source_resource;
    auto destination_resource_urn = root_urn + remote_destination_resource;

    Header header =
    {
      "Accept: */*",
      "Destination: " + destination_resource_urn.path()
    };

    Request request(this->options());

    auto url = this->webdav_hostname + source_resource_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "MOVE");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<curl_slist*>(header.handle));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif

    return request.perform();
  }

  bool
  Client::copy(const std::string& remote_source_resource, const std::string& remote_destination_resource) const
  {
    bool is_existed = this->check(remote_source_resource);
    if (!is_existed) return false;

    Path root_urn(this->webdav_root, true);

    auto source_resource_urn = root_urn + remote_source_resource;
    auto destination_resource_urn = root_urn + remote_destination_resource;

    Header header =
    {
      "Accept: */*",
      "Destination: " + destination_resource_urn.path()
    };

    Request request(this->options());

    auto url = this->webdav_hostname + source_resource_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "COPY");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<curl_slist*>(header.handle));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif

    return request.perform();
  }

  bool
  Client::upload(
    const std::string& remote_file,
    const std::string& local_file,
    progress_t progress
  ) const
  {
    return this->sync_upload(remote_file, local_file, nullptr, std::move(progress));
  }

  void
  Client::async_upload(
    const std::string& remote_file,
    const std::string& local_file,
    callback_t callback,
    progress_t progress
  ) const
  {
    std::thread uploading([ = ]()
    {
      this->sync_upload(remote_file, local_file, callback, std::move(progress));
    });
    uploading.detach();
  }

  bool
  Client::upload_from(
    const std::string& remote_file,
    std::istream& stream,
    progress_t progress
  ) const
  {
    return this->sync_upload_from(remote_file, stream, nullptr, std::move(progress));
  }

  bool
  Client::upload_from(
    const std::string& remote_file,
    char* buffer_ptr,
    unsigned long long buffer_size,
    progress_t progress
  ) const
  {
    return this->sync_upload_from(remote_file, buffer_ptr, buffer_size, nullptr, std::move(progress));
  }

  bool
  Client::clean(const std::string& remote_resource) const
  {
    bool is_existed = this->check(remote_resource);
    if (!is_existed) return true;

    auto root_urn = Path(this->webdav_root, true);
    auto resource_urn = root_urn + remote_resource;

    Header header =
    {
      "Accept: */*",
      "Connection: Keep-Alive"
    };

    Request request(this->options());

    auto url = this->webdav_hostname + resource_urn.quote(request.handle);

    request.set(CURLOPT_CUSTOMREQUEST, "DELETE");
    request.set(CURLOPT_URL, url.c_str());
    request.set(CURLOPT_HTTPHEADER, reinterpret_cast<curl_slist*>(header.handle));
#ifdef WDC_VERBOSE
    request.set(CURLOPT_VERBOSE, 1);
#endif

    return request.perform();
  }

  class Environment
  {
  public:
    Environment()
    {
      curl_global_init(CURL_GLOBAL_ALL);
    }
    ~Environment()
    {
      curl_global_cleanup();
    }
  };

    std::ostream &operator<<(std::ostream &os, const resource &resource) {
        os << "href: " << resource.href;
        os << " display_name: " << resource.display_name.value_or("(none)");
        os << " size: ";
        if (resource.size.has_value()) os << resource.size.value();
        else os << "(none)";
        os << " modified: ";
        if (resource.modified.has_value()) os << resource.modified.value();
        else os << "(none)";
        os << " created: ";
        if (resource.created.has_value()) os << resource.created.value();
        else os << "(none)";
        os << " type: ";
        if (resource.type.has_value()) os << resource.type.value();
        else os << "(none)";
        os << " etag: ";
        if (resource.etag.has_value()) os << resource.etag.value();
        else os << "(none)";
        return os;
    }
} // namespace WebDAV

static const WebDAV::Environment env;
