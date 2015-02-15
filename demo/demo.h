/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

// Defines class `DemoServer`, which defines the endpoints and keeps the `State` of the demo.

#ifndef DEMO_DEMO_H
#define DEMO_DEMO_H

#include <algorithm>
#include <string>

#include "../Bricks/dflags/dflags.h"

#include "uptime.h"
#include "state.h"

DECLARE_int32(demo_port);

namespace demo {

using bricks::FileSystem;
using bricks::net::api::HTTP;
using bricks::net::api::Request;
using bricks::net::HTTPResponseCode;
using namespace bricks::gnuplot;

class DemoServer {
 public:
  DemoServer(int port = FLAGS_demo_port) : port_(port) {
    printf("Preparing to listen on port %d...\n", port_);
    HTTP(port).Register("/ok", [](Request r) { r.connection.SendHTTPResponse("OK\n"); });
    HTTP(port).Register("/uptime", UptimeTracker());
    HTTP(port).Register("/yingyang.svg", State::ClassBoundaries);
    HTTP(port).Register("/config.json", [](Request r) {
      // TODO(sompylasar): Use cerealize.
      r.connection.SendHTTPResponse(
          "{}\n",
          HTTPResponseCode::OK, "application/json");
    });
    HTTP(port).Register("/layout/data", [](Request r) {
      // TODO(sompylasar): Use cerealize.
      // TODO(sompylasar): Use chunked response.
      r.connection.SendHTTPResponse(
          "{\"x\":0.0,\"y\":1.0}\n",
          HTTPResponseCode::OK, "application/json");
    });
    HTTP(port).Register("/layout/meta", [](Request r) {
      // TODO(sompylasar): Use cerealize.
      r.connection.SendHTTPResponse(
        "\
{\n\
  \"meta\":{\n\
    \"data_url\":\"/data\",\n\
    \"visualizer_name\":\"value-visualizer\",\n\
    \"visualizer_options\":{\n\
      \"header_text\":\"Demo Value\",\n\
      \"higher_is_better\":true\n\
    }\n\
  }\n\
}\n",
          HTTPResponseCode::OK,
          "application/json");
    });
    HTTP(port).Register("/layout", [](Request r) {
      // TODO(sompylasar): Use cerealize.
      r.connection.SendHTTPResponse(
          "\
{\n\
  \"layout\":{\n\
    \"col\":[\n\
      {\n\
        \"cell\":{\n\
          \"meta_url\":\"/meta\"\n\
        }\n\
      }\n\
    ]\n\
  }\n\
}\n",
          HTTPResponseCode::OK, "application/json");
    });
    // The "./static/" directory should be a symlink to "Web/build" or "Web/build-dev".
    FileSystem::ScanDir("./static/", [&port](const std::string& filename) {
      std::string filepath = "./static/" + filename;
      std::string fileurl = "/static/" + filename;
      // The default route points to the frontend entry point file.
      if (filename == "index.html") {
        fileurl.assign("/");
      }
      // Hack with string ownership.
      // TODO(dkorolev): Make this cleaner.
      std::string* contentType = new std::string(DemoServer::GetFileMimeType(filename));
      std::string* content = new std::string();
      *content = FileSystem::ReadFileAsString(filepath);
      HTTP(port).Register(fileurl, [content, contentType](Request r) {
        r.connection.SendHTTPResponse(*content, HTTPResponseCode::OK, *contentType);
      });
    });
  }

  void Join() {
    printf("Listening on port %d\n", port_);
    HTTP(port_).Join();
    printf("Done.\n");
  }

 private:
  State state_;
  const int port_;

  /**
   * Gets the file extension from the file name or path.
   */
  static const std::string GetFileExtension(const std::string& filename) {
    size_t fileextIndex = filename.find_last_of("/\\.");

    if (fileextIndex == std::string::npos || filename[fileextIndex] != '.') {
      return "";
    }

    // Add 1 to skip the dot.
    std::string fileext = filename.substr(fileextIndex + 1);

    return fileext;
  }

  /**
   * Detects MIME type by the file extension.
   */
  static const std::string GetFileMimeType(const std::string& filename) {
    std::string fileext = DemoServer::GetFileExtension(filename);

    std::transform(fileext.begin(), fileext.end(), fileext.begin(), ::tolower);

    std::map<std::string, std::string> mimeTypesByExtension = {
      {"js", "application/javascript"},
      {"json", "application/json"},
      {"css", "text/css"},
      {"html", "text/html"},
      {"htm", "text/html"},
      {"txt", "text/plain"},
      {"png", "image/png"},
      {"jpg", "image/jpeg"},
      {"jpeg", "image/jpeg"},
      {"gif", "image/gif"},
      {"svg", "image/svg"}
    };

    if (mimeTypesByExtension.count(fileext)) {
      return mimeTypesByExtension.at(fileext);
    }

    return "text/plain";
  }
};

}  // namespace demo

#endif  // DEMO_DEMO_H
