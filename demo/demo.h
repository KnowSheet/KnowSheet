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
#include <iostream>

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
using bricks::net::HTTPHeaders;
using bricks::net::HTTPResponseCode;
using bricks::net::GetFileMimeType;
using namespace bricks::gnuplot;
using namespace bricks::cerealize;


struct LayoutCell {
  // The `meta_url` is relative to the `layout_url`.
  std::string meta_url = "/meta";

  template <typename A>
  void save(A& ar) const {
    ar(CEREAL_NVP(meta_url));
  }
};

struct LayoutItem {
  std::vector<LayoutItem> row;
  std::vector<LayoutItem> col;
  LayoutCell cell;

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

struct ExampleMeta {
  struct Options {
    std::string header_text = "Real-time Data Made Easy";
    std::string color = "blue";
    double min = -1;
    double max = 1;
    double time_interval = 10000;
    template <typename A>
    void save(A& ar) const {
      // TODO(sompylasar): Make a meta that tells the frontend to use auto-min and max.
      ar(CEREAL_NVP(header_text),
         CEREAL_NVP(color),
         CEREAL_NVP(min),
         CEREAL_NVP(max),
         CEREAL_NVP(time_interval));
    }
  };

  // The `data_url` is relative to the `layout_url`.
  std::string data_url = "/data";
  std::string visualizer_name = "plot-visualizer";
  Options visualizer_options;

  template <typename A>
  void save(A& ar) const {
    ar(CEREAL_NVP(data_url), CEREAL_NVP(visualizer_name), CEREAL_NVP(visualizer_options));
  }
};

struct ExampleConfig {
  std::string layout_url = "/layout";

  // For the sake of the demo we put an empty array of `data_hostnames`
  // that results in the option being ignored by the frontend.
  // In production, this array should be filled with a set of alternative
  // hostnames that all resolve to the same backend. This technique is used 
  // to overcome the browser domain-based connection limit. The frontend selects
  // a domain from this array for every new connection via a simple round-robin.
  std::vector<std::string> data_hostnames;

  template <typename A>
  void save(A& ar) const {
    ar(CEREAL_NVP(layout_url), CEREAL_NVP(data_hostnames));
  }
};


class DemoServer {
 public:
  DemoServer(int port = FLAGS_demo_port) : port_(port) {
    std::cout << Printf("Preparing to listen on port %d...\n", port_);
    HTTP(port).Register("/ok", [](Request r) { r.connection.SendHTTPResponse("OK\n"); });
    HTTP(port).Register("/uptime", UptimeTracker());
    HTTP(port).Register("/yinyang.svg", State::ClassBoundaries);
    HTTP(port).Register("/config.json", [](Request r) {
      r.connection.SendHTTPResponse(ExampleConfig(),
                                    "config",
                                    HTTPResponseCode::OK,
                                    "application/json; charset=utf-8",
                                    HTTPHeaders({{"Access-Control-Allow-Origin", "*"}}));
    });
    HTTP(port).Register("/layout/data", [](Request r) {
      std::thread([](Request r) {
                    // Since we are in another thread, need to catch exceptions ourselves.
                    try {
                      auto response = r.connection.SendChunkedHTTPResponse(
                          HTTPResponseCode::OK,
                          "application/json; charset=utf-8",
                          {{"Access-Control-Allow-Origin", "*"}});
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
                                    HTTPHeaders({{"Access-Control-Allow-Origin", "*"}}));
    });
    HTTP(port).Register("/layout", [](Request r) {
      LayoutItem layout;
      LayoutItem row;
      layout.col.push_back(row);
      r.connection.SendHTTPResponse(layout,
                                    "layout",
                                    HTTPResponseCode::OK,
                                    "application/json; charset=utf-8",
                                    HTTPHeaders({{"Access-Control-Allow-Origin", "*"}}));
    });
    // The "./static/" directory should be a symlink to "Web/build" or "Web/build-dev".
    FileSystem::ScanDir("./static/", [&port](const std::string& filename) {
      const std::string filepath = "./static/" + filename;
      std::string fileurl = "/static/" + filename;
      // The default route points to the frontend entry point file.
      if (filename == "index.html") {
        fileurl = "/";
      }
      // Hack with string ownership.
      // TODO(dkorolev): Make this cleaner.
      const std::string* contentType = new std::string(GetFileMimeType(filename));
      std::string* content = new std::string();
      *content = FileSystem::ReadFileAsString(filepath);
      HTTP(port).Register(fileurl, [content, contentType](Request r) {
        r.connection.SendHTTPResponse(*content, HTTPResponseCode::OK, *contentType);
      });
    });
    HTTP(port).Register("/demo_id", [this](Request r) { state_.DemoRequest(std::move(r)); });
  }

  void Join() {
    std::cout << Printf("Listening on port %d\n", port_);
    HTTP(port_).Join();
    std::cout << Printf("Done.\n");
  }

 private:
  State state_;
  const int port_;
};

}  // namespace demo

#endif  // DEMO_DEMO_H
