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

// Defines class `State`, which stores the information of all the points added so far.

#ifndef DEMO_STATE_H
#define DEMO_STATE_H

#include <vector>
#include <string>

#include "../Bricks/net/api/api.h"
#include "../Bricks/net/http/codes.h"
#include "../Bricks/cerealize/cerealize.h"
#include "../Bricks/graph/gnuplot.h"

namespace demo {

using bricks::net::api::Request;
using bricks::net::HTTPResponseCode;
using bricks::JSONParseException;
using namespace bricks::cerealize;
using namespace bricks::gnuplot;

struct State {
  struct Point {
    double x;
    double y;
    bool label;
    Point(double x = 0, double y = 0, bool label = false) : x(x), y(y), label(label) {}
    template <typename A>
    void serialize(A& ar) {
      ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(label));
    }
  };

  std::vector<Point> points;
  template <typename A>
  void serialize(A& ar) {
    ar(CEREAL_NVP(points));
  }

  State() {}

  static void ClassBoundaries(Request r) {
    const size_t N = 60;
    r.connection.SendHTTPResponse(GNUPlot()
                                      .Title("Class boundaries")
                                      .NoKey()
                                      .XRange(-1.2, +1.2)
                                      .YRange(-1.2, +1.2)
                                      .Grid("back")
                                      .Plot(WithMeta([](Plotter& p) {
                                                       for (size_t i = 0; i < N; ++i) {
                                                         const double t = M_PI * 2 * i / (N - 1);
                                                         p(sin(t), cos(t));
                                                       }
                                                     })
                                                .LineWidth(5)
                                                .Color("rgb 'black'"))
                                      .Plot(WithMeta([](Plotter& p) {
                                                       for (size_t i = 0; i < N / 2; ++i) {
                                                         const double t = M_PI * i / (N / 2 - 1);
                                                         p(+sin(t) * 0.5, cos(t) * 0.5 + 0.5);
                                                       }
                                                     })
                                                .LineWidth(5)
                                                .Color("rgb 'black'"))
                                      .Plot(WithMeta([](Plotter& p) {
                                                       for (size_t i = 0; i < N / 2; ++i) {
                                                         const double t = M_PI * i / (N / 2 - 1);
                                                         p(-sin(t) * 0.5, cos(t) * 0.5 - 0.5);
                                                       }
                                                     })
                                                .LineWidth(5)
                                                .Color("rgb 'black'"))
                                      .OutputFormat("svg"),
                                  HTTPResponseCode::OK,
                                  "image/svg+xml");
  }

  void DemoRequest(Request r) {
    if (r.http.Method() == "POST") {
      // TODO(dkorolev): This should get simpler once Bricks 1.0 is out, the `.http.` will go away.
      if (!r.http.HasBody()) {
        points.emplace_back(atof(r.url.query["x"].c_str()),
                            atof(r.url.query["y"].c_str()),
                            !!atoi(r.url.query["label"].c_str()));
        r.connection.SendHTTPResponse("ADDED\n");
      } else {
        // TODO(dkorolev): Move this logic of returning 500 on JSONParseException (any exception, really) to Bricks.
        try {
          points.push_back(std::move(JSONParse<Point>(r.http.Body())));
          r.connection.SendHTTPResponse("ADDED\n");
        } catch (const JSONParseException& e) {
          // TODO(dkorolev): Make sure just `e.what()` compiles w/o casting to `std::string`.
          r.connection.SendHTTPResponse(std::string(e.what()), HTTPResponseCode::InternalServerError);
        }
      }
    } else if (r.url.query["format"] == "svg") {
      // TODO(dkorolev): Change colors, make it red vs. blue.
      r.connection.SendHTTPResponse(GNUPlot()
                                        .Title("State.")
                                        .KeyTitle("Legend")
                                        .XRange(-1.1, +1.1)
                                        .YRange(-1.1, +1.1)
                                        .Grid("back")
                                        .Plot(WithMeta([this](Plotter& plotter) {
                                                         for (const auto& p : points) {
                                                           if (!p.label) {
                                                             plotter(p.x, p.y);
                                                           }
                                                         }
                                                       })
                                                  .Name("Label \"false\"")
                                                  .AsPoints())
                                        .Plot(WithMeta([this](Plotter& plotter) {
                                                         for (const auto& p : points) {
                                                           if (p.label) {
                                                             plotter(p.x, p.y);
                                                           }
                                                         }
                                                       })
                                                  .Name("Label \"true\"")
                                                  .AsPoints())
                                        .OutputFormat("svg"),
                                    HTTPResponseCode::OK,
                                    "image/svg+xml");
    } else {
      r.connection.SendHTTPResponse(*this, "state", HTTPResponseCode::OK, "application/json");
    }
  }
};

}  // namespace demo

#endif  // DEMO_STATE_H
