#include "frame.h"
#include <gtest/gtest.h>

class FrameTest : public ::testing::Test
{
        protected:
    void SetUp() override
    {
        // Set up logic, if needed.
    }
};

TEST_F(FrameTest, ConstructorWithQSize)
{
    QSize size(10, 10);
    Frame frame(size);
    EXPECT_EQ(frame.toString().count('\n'), size.height());
    EXPECT_EQ(frame.toString().count(','), size.width() * size.height() * 2);
}

TEST_F(FrameTest, ConstructorWithCSVAndSize)
{
    QSize size(2, 2);
    QString csv = "0, 0, 0, 0,\n0, 0, 0, 0,\n";
    Frame frame(csv, size);
    EXPECT_EQ(frame.toString().count('\n'), size.height());
    EXPECT_EQ(frame.toString().count(','), size.width() * size.height() * 2);
}

TEST_F(FrameTest, SetAndGetAmplitude)
{
    QSize size(10, 10);
    Frame frame(size);
    QPoint pos(5, 5);
    frame.setAmplitude(pos, 100);
    EXPECT_EQ(frame.getAmplitude(pos), 100);
}

TEST_F(FrameTest, SetAndGetFrequency)
{
    QSize size(10, 10);
    Frame frame(size);
    QPoint pos(5, 5);
    frame.setFrequency(pos, 50);
    EXPECT_EQ(frame.getFrequency(pos), 50);
}

TEST_F(FrameTest, SetAndGetColor)
{
    QSize size(10, 10);
    Frame frame(size);
    QPoint pos(5, 5);
    frame.setFrequency(pos, 11000);
    frame.setAmplitude(pos, 16383);
    QColor color = frame.getColor(pos);
    EXPECT_EQ(color.hue(), 100);
    EXPECT_EQ(color.saturation(), 127);
}

TEST_F(FrameTest, InvalidIndexCheck)
{
    QSize size(10, 10);
    Frame frame(size);
    EXPECT_EQ(frame.getAmplitude(QPoint(11, 5)), -1);
    EXPECT_EQ(frame.getFrequency(QPoint(5, 11)), -1);
}

TEST_F(FrameTest, ErrorCheck)
{
    QSize size(2, 2);
    QString invalidCsv = "0, 0, 0,\n0, 0, 0, 0,\n";
    // Expecting that constructor of Frame will throw an exception of any type
    EXPECT_THROW([&]() { Frame frame(invalidCsv, size); }(), std::exception);
}
