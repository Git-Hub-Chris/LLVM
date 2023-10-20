//===-- Unittests for gettimeofday ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <time.h>

#include "src/time/gettimeofday.h"
#include "src/time/nanosleep.h"
#include "test/UnitTest/ErrnoSetterMatcher.h"
#include "test/UnitTest/Test.h"

namespace cpp = LIBC_NAMESPACE::cpp;

using LIBC_NAMESPACE::testing::tlog;

TEST(LlvmLibcGettimeofday, SmokeTest) {
  using LIBC_NAMESPACE::testing::ErrnoSetterMatcher::Succeeds;
  void *tz = nullptr;

  bool retry = false;

  suseconds_t sleep_times[2] = {200, 1000};
  for (int i = 0; i < 2; i++) {
    timeval tv;
    int ret = LIBC_NAMESPACE::gettimeofday(&tv, tz);
    ASSERT_EQ(ret, 0);

    suseconds_t sleep_time = sleep_times[i];
    // Sleep for {sleep_time} microsceconds.
    timespec tim = {0, sleep_time * 1000};
    timespec tim2 = {0, 0};
    ret = LIBC_NAMESPACE::nanosleep(&tim, &tim2);

    if (ret < 0) {
      tlog << "nanosleep call failed, skip the remaining of the test.";
      continue;
    }

    // Call gettimeofday again and verify that it is more {sleep_time}
    // microseconds.
    timeval tv1;
    ret = LIBC_NAMESPACE::gettimeofday(&tv1, tz);
    ASSERT_EQ(ret, 0);

    auto diff = tv1.tv_usec - tv.tv_usec;
    if (diff < 0) {
      tlog << "Time goes backward from tv: " << tv.tv_usec
           << " to tv1: " << tv1.tv_usec;

      if (!retry) {
        tlog << "Retry the test.";
        retry = true;
        --i;
      } else {
        tlog << "Skip the remaining of the test.";
      }

      continue;
    }

    ASSERT_GE(diff, sleep_time);
  }
}
