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

#include "../Bricks/net/api/api.h"
#include "../Bricks/net/http/codes.h"
#include "../Bricks/graph/gnuplot.h"

namespace demo {

using bricks::net::api::Request;
using bricks::net::HTTPResponseCode;
using namespace bricks::gnuplot;

struct State {
  // TODO(dkorolev): Add the actual data points here.

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
};

}  // namespace demo

#endif  // DEMO_STATE_H
