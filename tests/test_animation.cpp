#include "animation.h"
#include <QFile>
#include <gtest/gtest.h>

class AnimationTest : public ::testing::Test
{
        protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(AnimationTest, DefaultConstructor)
{
    QSize size(200, 200);
    Animation anim(size);
    EXPECT_EQ(anim.getSize(), size);
    EXPECT_EQ(anim.getCurrentFrameIndex(), 0);
}

TEST_F(AnimationTest, FileConstructor_ValidFile)
{

    QString filePath = "resources/valid.csv";
    Animation anim(filePath);
    EXPECT_FALSE(anim.getError());
}

TEST_F(AnimationTest, FileConstructor_InvalidFile)
{
    QString filePath = "resources/invalid.csv";
    ASSERT_THROW(Animation anim(filePath), std::runtime_error);
}

TEST_F(AnimationTest, AddFrame)
{
    QSize size(200, 200);
    Animation anim(size);
    anim.addFrame();
    EXPECT_EQ(anim.getLen(), 2);
}

TEST_F(AnimationTest, DuplicateCurrentFrame)
{
    QSize size(200, 200);
    Animation anim(size);
    anim.duplicateCurrentFrame();
    EXPECT_EQ(anim.getLen(), 2);
}

TEST_F(AnimationTest, SelectFrame)
{
    QSize size(100, 100);
    Animation anim(size);
    anim.addFrame();
    anim.selectFrame(1);
    EXPECT_EQ(anim.getCurrentFrameIndex(), 1);
}

TEST_F(AnimationTest, NextFrame)
{
    QSize size(100, 100);
    Animation anim(size);
    anim.addFrame();
    anim.nextFrame();
    EXPECT_EQ(anim.getCurrentFrameIndex(), 1);
}

TEST_F(AnimationTest, NextFrameLooping)
{
    QSize size(100, 100);
    Animation anim(size);
    anim.nextFrame();
    EXPECT_EQ(anim.getCurrentFrameIndex(), 0);
}

TEST_F(AnimationTest, RemoveCurrentFrame)
{
    QSize size(100, 100);
    Animation anim(size);
    anim.addFrame();
    anim.removeCurrentFrame();
    EXPECT_EQ(anim.getLen(), 1);
}

TEST_F(AnimationTest, GetFrameString)
{
    QSize size(100, 100);
    Animation anim(size);
    QString frameStr = anim.getFrameString();
    EXPECT_FALSE(frameStr.isEmpty());
}

TEST_F(AnimationTest, WriteToFile)
{
    QSize size(100, 100);
    Animation anim(size);
    QString outputPath = "test.csv";
    EXPECT_TRUE(anim.writeInFile(outputPath));
    EXPECT_FALSE(anim.getError());
    EXPECT_TRUE(QFile::exists(outputPath));

    Animation anim2(outputPath);
    EXPECT_FALSE(anim2.getError());
    EXPECT_EQ(anim2.getLen(), anim.getLen());

    QFile::remove(outputPath);
}

TEST_F(AnimationTest, NoErrorOnDefaultConstructor)
{
    QSize size(100, 100);
    Animation anim(size);
    EXPECT_FALSE(anim.getError());
}

TEST_F(AnimationTest, GetAmplitude)
{
    QSize size(100, 100);
    Animation anim(size);
    QPoint point(50, 50);
    EXPECT_EQ(anim.getAmplitude(point), 0);
}

TEST_F(AnimationTest, GetFrequency)
{
    QSize size(100, 100);
    Animation anim(size);
    QPoint point(50, 50);
    EXPECT_EQ(anim.getFrequency(point), 0);
}
