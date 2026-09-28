#include <gtest/gtest.h>
#include "common/FrameReader.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

std::vector<std::uint8_t> encodeLength(std::uint32_t length) {
    return {
        static_cast<std::uint8_t>((length >> 24) & 0xFF),
        static_cast<std::uint8_t>((length >> 16) & 0xFF),
        static_cast<std::uint8_t>((length >> 8) & 0xFF),
        static_cast<std::uint8_t>(length & 0xFF)
    };
}

std::vector<std::uint8_t> buildFrame(const std::vector<std::uint8_t>& payload) {
    std::vector<std::uint8_t> frame = encodeLength(static_cast<std::uint32_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

}

TEST(FrameReader, ExtractsCompleteMessageArrivingInOneFeed) {
    FrameReader reader;
    std::vector<std::uint8_t> payload = {'H', 'E', 'L', 'L', 'O'};
    reader.feed(buildFrame(payload));

    std::optional<std::vector<std::uint8_t>> frame = reader.tryExtractFrame();

    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(*frame, payload);
}

TEST(FrameReader, ReturnsNulloptUntilMessageFullyArrives) {
    FrameReader reader;
    std::vector<std::uint8_t> payload = {'H', 'E', 'L', 'L', 'O'};
    std::vector<std::uint8_t> fullFrame = buildFrame(payload);

    reader.feed(std::vector<std::uint8_t>(fullFrame.begin(), fullFrame.begin() + 2));
    EXPECT_FALSE(reader.tryExtractFrame().has_value());

    reader.feed(std::vector<std::uint8_t>(fullFrame.begin() + 2, fullFrame.begin() + 5));
    EXPECT_FALSE(reader.tryExtractFrame().has_value());

    reader.feed(std::vector<std::uint8_t>(fullFrame.begin() + 5, fullFrame.end()));
    std::optional<std::vector<std::uint8_t>> frame = reader.tryExtractFrame();

    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(*frame, payload);
}

TEST(FrameReader, ExtractsTwoMessagesFedTogether) {
    FrameReader reader;
    std::vector<std::uint8_t> first = {'A', 'B'};
    std::vector<std::uint8_t> second = {'C', 'D', 'E'};

    std::vector<std::uint8_t> combined = buildFrame(first);
    std::vector<std::uint8_t> secondFrame = buildFrame(second);
    combined.insert(combined.end(), secondFrame.begin(), secondFrame.end());

    reader.feed(combined);

    std::optional<std::vector<std::uint8_t>> frame1 = reader.tryExtractFrame();
    std::optional<std::vector<std::uint8_t>> frame2 = reader.tryExtractFrame();
    std::optional<std::vector<std::uint8_t>> frame3 = reader.tryExtractFrame();

    ASSERT_TRUE(frame1.has_value());
    ASSERT_TRUE(frame2.has_value());
    EXPECT_EQ(*frame1, first);
    EXPECT_EQ(*frame2, second);
    EXPECT_FALSE(frame3.has_value());
}

TEST(FrameReader, ThrowsOnHostileLengthField) {
    FrameReader reader;
    reader.feed(encodeLength(1000000));

    EXPECT_THROW(reader.tryExtractFrame(), std::runtime_error);
}

TEST(FrameReader, EncodeFrameProducesBigEndianLengthPrefix) {
    std::vector<std::uint8_t> payload = {'A', 'B', 'C'};

    std::vector<std::uint8_t> frame = encodeFrame(payload);

    ASSERT_EQ(frame.size(), 4 + payload.size());
    EXPECT_EQ(std::vector<std::uint8_t>(frame.begin(), frame.begin() + 4), encodeLength(3));
    EXPECT_EQ(std::vector<std::uint8_t>(frame.begin() + 4, frame.end()), payload);
}

TEST(FrameReader, EncodeFrameRoundTripsThroughReader) {
    FrameReader reader;
    std::vector<std::uint8_t> payload = {'H', 'E', 'L', 'L', 'O'};

    reader.feed(encodeFrame(payload));
    std::optional<std::vector<std::uint8_t>> frame = reader.tryExtractFrame();

    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(*frame, payload);
}

TEST(FrameReader, EncodeFrameHandlesEmptyPayload) {
    FrameReader reader;
    std::vector<std::uint8_t> payload;

    reader.feed(encodeFrame(payload));
    std::optional<std::vector<std::uint8_t>> frame = reader.tryExtractFrame();

    ASSERT_TRUE(frame.has_value());
    EXPECT_TRUE(frame->empty());
}
