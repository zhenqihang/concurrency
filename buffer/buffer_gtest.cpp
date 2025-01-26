#include <gtest/gtest.h>
#include "buffer.h"

TEST(BufferTest, Initialization) {
    Buffer buffer;
    EXPECT_EQ(buffer.ReadableBytes(), 0);
    EXPECT_EQ(buffer.WritableBytes(), 1024);
    EXPECT_EQ(buffer.PrependableBytes(), 0);
}

TEST(BufferTest, AppendAndRetrieve) {
    Buffer buffer;
    std::string str = "Hello World!\n";
    buffer.Append(str);
    EXPECT_EQ(buffer.ReadableBytes(), str.size());
    EXPECT_EQ(buffer.RetrieveAllToStr(), str);
    EXPECT_EQ(buffer.ReadableBytes(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}