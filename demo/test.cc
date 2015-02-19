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

#include "demo.h"

DEFINE_int32(demo_port, 2015, "The local port to spawn the demo on.");

#include "../Bricks/util/singleton.h"
#include "../Bricks/file/file.h"

// TODO(dkorolev): Should we swap the arguments to `WriteStringToFile()` to actually read "string to file"? :)

#include "../Bricks/3party/gtest/gtest-main-with-dflags.h"

using namespace demo;
using bricks::Singleton;
using namespace bricks::net::api;
using namespace bricks::cerealize;
using bricks::FileSystem;

TEST(Demo, OK) {
  Singleton<demo::DemoServer>();
  const auto response = HTTP(GET("localhost:2015/ok"));
  EXPECT_EQ(200, static_cast<int>(response.code));
  EXPECT_EQ("OK\n", response.body);
}

TEST(Demo, NoPointsYet) {
  Singleton<DemoServer>();
  const auto response = HTTP(GET("localhost:2015/demo_id"));
  EXPECT_EQ(200, static_cast<int>(response.code));
  EXPECT_EQ("{\"state\":{\"points\":[]}}\n", response.body);
}

TEST(Demo, AddTwoPoints) {
  Singleton<DemoServer>();
  EXPECT_EQ("ADDED\n", HTTP(POST("localhost:2015/demo_id?x=+0.25&y=-0.25&label=1")).body);
  EXPECT_EQ("ADDED\n", HTTP(POST("localhost:2015/demo_id", State::Point(-0.25, +0.25, false))).body);
}

TEST(Demo, AddInvalidPointReturns500) {
  Singleton<DemoServer>();
  const auto response = HTTP(POST("localhost:2015/demo_id", "fffuuuu", "application/json"));
  EXPECT_EQ(500, static_cast<int>(response.code));
  EXPECT_EQ("<h1>INTERNAL SERVER ERROR</h1>\n", response.body);
}

TEST(Demo, HasTwoPoints) {
  Singleton<DemoServer>();
  EXPECT_EQ(
      "{\"state\":{\"points\":["
      "{\"x\":0.25,\"y\":-0.25,\"label\":true},"
      "{\"x\":-0.25,\"y\":0.25,\"label\":false}"
      "]}}\n",
      HTTP(GET("localhost:2015/demo_id")).body);
}

TEST(Demo, VisualizesPoints) {
  Singleton<DemoServer>();
  EXPECT_EQ(FileSystem::ReadFileAsString("golden/two_points.svg"),
            HTTP(GET("localhost:2015/demo_id?format=svg")).body);
}
