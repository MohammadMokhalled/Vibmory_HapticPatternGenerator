#include <gtest/gtest.h>
#include "animation.h"

class AnimationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// 1. Test Default Constructor
TEST_F(AnimationTest, DefaultConstructor) {
    QSize size(200, 200);
    Animation anim(size);
    EXPECT_EQ(anim.getSize(), size);
    EXPECT_EQ(anim.getCurrentFrameIndex(), 0);
}

// // 2. Test File Constructor with Valid File
// TEST_F(AnimationTest, FileConstructor_ValidFile) {
//     QString filePath = "path_to_valid_file";
//     Animation anim(filePath);
//     EXPECT_FALSE(anim.getError());
// }

// // 3. Test File Constructor with Invalid File
// TEST_F(AnimationTest, FileConstructor_InvalidFile) {
//     QString filePath = "path_to_invalid_file";
//     ASSERT_THROW(Animation anim(filePath), std::runtime_error);
// }

// 4. Test Add Frame
TEST_F(AnimationTest, AddFrame) {
    QSize size(200, 200);
    Animation anim(size);
    anim.addFrame();
    EXPECT_EQ(anim.getLen(), 2);
}

// 5. Test Duplicate Current Frame
TEST_F(AnimationTest, DuplicateCurrentFrame) {
    QSize size(200, 200);
    Animation anim(size);
    anim.duplicateCurrentFrame();
    EXPECT_EQ(anim.getLen(), 2);
}

// 6. Test Frame Selection
TEST_F(AnimationTest, SelectFrame) {
    QSize size(100, 100);
    Animation anim(size);
    anim.addFrame();
    anim.selectFrame(1);
    EXPECT_EQ(anim.getCurrentFrameIndex(), 1);
}

// 7. Test Position Setting
TEST_F(AnimationTest, SetPosition) {
    QSize size(100, 100);
    Animation anim(size);
    QPoint point(50, 50);
    anim.setPos(point);
    // You might need some method to verify the set position.
}

// 8. Test Next Frame Logic
TEST_F(AnimationTest, NextFrame) {
    QSize size(100, 100);
    Animation anim(size);
    anim.addFrame();
    anim.nextFrame();
    EXPECT_EQ(anim.getCurrentFrameIndex(), 1);
}

// 9. Test Next Frame Looping Logic
TEST_F(AnimationTest, NextFrameLooping) {
    QSize size(100, 100);
    Animation anim(size);
    anim.nextFrame();
    EXPECT_EQ(anim.getCurrentFrameIndex(), 0);
}

// 10. Test Removing Current Frame
TEST_F(AnimationTest, RemoveCurrentFrame) {
    QSize size(100, 100);
    Animation anim(size);
    anim.addFrame();
    anim.removeCurrentFrame();
    EXPECT_EQ(anim.getLen(), 1);
}

// 11. Test Getting Frame String
TEST_F(AnimationTest, GetFrameString) {
    QSize size(100, 100);
    Animation anim(size);
    QString frameStr = anim.getFrameString();
    EXPECT_FALSE(frameStr.isEmpty());
}

// 12. Test Writing To File
// TEST_F(AnimationTest, WriteToFile) {
//     QSize size(100, 100);
//     Animation anim(size);
//     QString outputPath = "path_to_output_file";
//     EXPECT_TRUE(anim.writeInFile(outputPath));
// }

// 13. Test Error Flag On Default Constructor
TEST_F(AnimationTest, NoErrorOnDefaultConstructor) {
    QSize size(100, 100);
    Animation anim(size);
    EXPECT_FALSE(anim.getError());
}

// 14. Test Getting Amplitude
TEST_F(AnimationTest, GetAmplitude) {
    QSize size(100, 100);
    Animation anim(size);
    QPoint point(50, 50);
    // Assuming default amplitude value is 0 for a new frame
    EXPECT_EQ(anim.getAmplitude(point), 0);
}

// 15. Test Getting Frequency
TEST_F(AnimationTest, GetFrequency) {
    QSize size(100, 100);
    Animation anim(size);
    QPoint point(50, 50);
    // Assuming default frequency value is 0 for a new frame
    EXPECT_EQ(anim.getFrequency(point), 0);
}
