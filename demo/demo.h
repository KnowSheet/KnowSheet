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
#include <type_traits>

#include "../Bricks/dflags/dflags.h"
#include "../Bricks/cerealize/cerealize.h"
#include "../Bricks/strings/printf.h"
#include "../Bricks/time/chrono.h"

#include "uptime.h"
#include "state.h"

DECLARE_int32(demo_port);

namespace demo {

using bricks::FileSystem;
using bricks::time::Now;
using bricks::strings::Printf;
using bricks::net::api::HTTP;
using bricks::net::api::Request;
using bricks::net::HTTPResponseCode;
using namespace bricks::gnuplot;
using namespace bricks::cerealize;


struct LayoutCell {
  std::string meta_url = "/meta";

  // Define only output serialization (`JSON.stringify()`), forbid input serialization (`JSON.parse()`).
  template <typename A>
  void save(A& ar) const {
    ar(CEREAL_NVP(meta_url));
  }
};
static_assert(is_write_cerealizable<LayoutCell>::value, "");
static_assert(!is_read_cerealizable<LayoutCell>::value, "");

struct LayoutItem {
  std::vector<LayoutItem> row;
  std::vector<LayoutItem> col;
  LayoutCell cell;

  // Define only output serialization (`JSON.stringify()`), forbid input serialization (`JSON.parse()`).
  template <typename A>
  void save(A& ar) const {
    if (!row.empty()) {
      ar(CEREAL_NVP(row));
    } else if (!col.empty()) {
      ar(CEREAL_NVP(col));
    } else {
      ar(CEREAL_NVP(cell));
    }
  }
};
static_assert(is_write_cerealizable<LayoutItem>::value, "");
static_assert(!is_read_cerealizable<LayoutItem>::value, "");

struct ExampleMeta {
  struct Options {
    std::string header_text = "Real-time Data Made Easy";
    std::string color = "blue";
    double min = 0;
    double max = 1;
    double time_interval = 10000;
    template <typename A>
    void serialize(A& ar) {
      ar(CEREAL_NVP(header_text),
         CEREAL_NVP(color),
         CEREAL_NVP(min),
         CEREAL_NVP(max),
         CEREAL_NVP(time_interval));
    }
  };
  std::string data_url = "/data";
  std::string visualizer_name = "plot-visualizer";
  Options visualizer_options;
  template <typename A>
  void serialize(A& ar) {
    ar(CEREAL_NVP(data_url), CEREAL_NVP(visualizer_name), CEREAL_NVP(visualizer_options));
  }
};


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
      std::thread([](Request&& r) {
                    // Since we are in another thread, need to catch exceptions ourselves.
                    try {
                      auto response = r.connection.SendChunkedHTTPResponse(
                          HTTPResponseCode::OK,
                          "application/json; charset=utf-8",
                          {{"Connection", "keep-alive"}, {"Access-Control-Allow-Origin", "*"}});
                      std::string data;
                      const double begin = static_cast<double>(Now());
                      const double t = atof(r.url.query["t"].c_str());
                      const double end = (t > 0) ? (begin + t * 1e3) : 1e18;
                      double current;
                      while ((current = static_cast<double>(Now())) < end) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100 + 100));
                        const double x = current;
                        const double y = sin(5e-3 * (current - begin));
                        data += Printf("{\"x\":%lf,\"y\":%lf}\n", x, y);
                        const double f = (rand() % 101) * (rand() % 101) * (rand() % 101) * 1e-6;
                        const size_t n = static_cast<size_t>(data.length() * f);
                        if (n) {
                          response.Send(data.substr(0, n));
                          data = data.substr(n);
                        }
                      }
                    } catch (const std::exception& e) {
                      std::cerr << "Exception in data serving thread: " << e.what() << std::endl;
                    }
                  },
                  std::move(r)).detach();
    });
    HTTP(port).Register("/layout/meta", [](Request r) {
      r.connection.SendHTTPResponse(ExampleMeta(),
                                    "meta",
                                    HTTPResponseCode::OK,
                                    "application/json; charset=utf-8",
                                    {{"Connection", "close"}, {"Access-Control-Allow-Origin", "*"}});
    });
    HTTP(port).Register("/layout", [](Request r) {
      LayoutItem layout;
      LayoutItem row;
      layout.col.push_back(row);
      r.connection.SendHTTPResponse(layout,
                                    "layout",
                                    HTTPResponseCode::OK,
                                    "application/json; charset=utf-8",
                                    {{"Connection", "close"}, {"Access-Control-Allow-Origin", "*"}});
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
