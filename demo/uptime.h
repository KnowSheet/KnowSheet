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

// A simple module to respond back with uptime formatted as a JSON object.

#ifndef DEMO_UPTIME_H
#define DEMO_UPTIME_H

#include "../Bricks/time/chrono.h"
#include "../Bricks/cerealize/cerealize.h"
#include "../Bricks/net/api/api.h"

struct UptimeTracker {
  bricks::time::EPOCH_MILLISECONDS start_ms_;
  UptimeTracker() : start_ms_(bricks::time::Now()) {}

  // An object to serialize uptime as JSON.
  struct ResponseJSON {
    uint64_t uptime_total_s, uptime_total_m, uptime_total_h, uptime_total_d;
    ResponseJSON(bricks::time::MILLISECONDS_INTERVAL uptime_ms)
        : uptime_total_s(static_cast<uint64_t>(uptime_ms) / 1000),
          uptime_total_m(uptime_total_s / 60),
          uptime_total_h(uptime_total_m / 60),
          uptime_total_d(uptime_total_h / 24) {}
    template <typename A>
    void serialize(A& ar) {
      ar(CEREAL_NVP(uptime_total_s),
         CEREAL_NVP(uptime_total_m),
         CEREAL_NVP(uptime_total_h),
         CEREAL_NVP(uptime_total_d));
    }
  };

  // A functor to be able to use the `UptimeTracker` object directly as a handler.
  void operator()(bricks::net::api::Request r) {
    r.connection.SendHTTPResponse(ResponseJSON(bricks::time::Now() - start_ms_), "uptime");
  };
};

#endif  // DEMO_UPTIME_H
