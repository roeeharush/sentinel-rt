#include <gtest/gtest.h>
#include "common/core/Status.h"

TEST(Status, OkIsFirstValue) {
    Status s = Status::Ok;
    EXPECT_EQ(s, Status::Ok);
}
