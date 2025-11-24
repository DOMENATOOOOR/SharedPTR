#include <fstream>
#include <gtest/gtest.h>
#include "Shared.h"


struct Counter {
    static int destructor;
    int value;
    Counter(int v = 0) : value(v) {
    }
    ~Counter() {
        destructor++;
    }

};

int Counter::destructor = 0;

struct MyDeleter {
    static int calls;
    void operator()(Counter *p) const {
        ++calls;
        delete p;
    }
};

int MyDeleter::calls = 0;

TEST(SharedPTR, ConstructorAndDestructor) {
    Counter::destructor = 0;
    {
        SharedPTR<Counter> ptr(new Counter(123));
        EXPECT_EQ(123, ptr->value);
        EXPECT_EQ(ptr.use_count(), 1);
    }
    EXPECT_EQ(1, Counter::destructor);
}



TEST(SharedPtrTests, CopyIncreasesCount) {
    Counter::destructor = 0;
    auto nn = Counter::destructor;
    SharedPTR<Counter> p1(new Counter(5));

    {
        SharedPTR<Counter> p2(p1);
        EXPECT_EQ(p1.use_count(), 2);
        EXPECT_EQ(p2.use_count(), 2);
    }

    EXPECT_EQ(p1.use_count(), 1);
    EXPECT_EQ(Counter::destructor, 0);
}

TEST(SharedPtrTests, MoveConstructorTransfersOwnership) {
    Counter::destructor = 0;

    SharedPTR<Counter> p1(new Counter(10));
    EXPECT_EQ(p1.use_count(), 1);

    SharedPTR<Counter> p2(std::move(p1));
    EXPECT_EQ(p2.use_count(), 1);
    EXPECT_EQ(p1.use_count(), 0);
}

TEST(SharedPtrTests, ResetReleasesObject) {
    Counter::destructor = 0;

    SharedPTR<Counter> p(new Counter(20));
    EXPECT_EQ(p.use_count(), 1);

    p.reset();
    EXPECT_EQ(p.use_count(), 0);
    EXPECT_EQ(Counter::destructor, 1);
}

TEST(SharedPtrTests, MakeSharedCreatesObject) {
    auto p = make_shared<Counter>(40);
    EXPECT_EQ(p->value, 40);
    EXPECT_EQ(p.use_count(), 1);
}

TEST(WeakPtrTests, BasicWeakPtrFunctionality) {
    Counter::destructor = 0;

    SharedPTR<Counter> shared(new Counter(50));
    EXPECT_EQ(shared.use_count(), 1);

    WeakPtr<Counter> weak(shared);
    EXPECT_FALSE(weak.expired());
    EXPECT_EQ(weak.use_count(), 1);

    {
        auto locked = weak.lock();
        EXPECT_TRUE(locked.get() != nullptr);
        EXPECT_EQ(locked.use_count(), 2);
    }

    EXPECT_EQ(shared.use_count(), 1);
}

TEST(WeakPtrTests, WeakPtrExpiresAfterSharedDeleted) {
    Counter::destructor = 0;
    WeakPtr<Counter> weak;

    {
        SharedPTR<Counter> shared(new Counter(99));
        weak = shared;

        EXPECT_FALSE(weak.expired());
        EXPECT_EQ(weak.use_count(), 1);
    }

    EXPECT_TRUE(weak.expired());
    EXPECT_EQ(weak.use_count(), 0);
}

TEST(WeakPtrTests, LockOnExpiredReturnsEmptySharedPtr) {
    WeakPtr<Counter> weak;

    {
        SharedPTR<Counter> shared(new Counter(123));
        weak = shared;
    }

    auto locked = weak.lock();
    EXPECT_EQ(locked.get(), nullptr);
    EXPECT_EQ(locked.use_count(), 0);
}

TEST(WeakPtrTests, ManyWeakPtrsTrackCorrectly) {
    Counter::destructor = 0;

    SharedPTR<Counter> shared(new Counter(10));

    WeakPtr<Counter> w1(shared);
    WeakPtr<Counter> w2(shared);
    WeakPtr<Counter> w3(w1);

    EXPECT_EQ(shared.use_count(), 1);
    EXPECT_FALSE(w1.expired());
    EXPECT_FALSE(w2.expired());
    EXPECT_FALSE(w3.expired());

    shared.reset();

    EXPECT_TRUE(w1.expired());
    EXPECT_TRUE(w2.expired());
    EXPECT_TRUE(w3.expired());
}

TEST(SharedPtrTests, CustomDelter) {
    Counter::destructor = 0;
    MyDeleter::calls = 0;
    {
        SharedPTR<Counter, MyDeleter> shared(new Counter(180), MyDeleter());
        EXPECT_EQ(shared->value, 180);
        EXPECT_EQ(shared->destructor, 0);
        EXPECT_EQ(MyDeleter::calls, 0);
    }
    EXPECT_EQ(Counter::destructor, 1);
    EXPECT_EQ(MyDeleter::calls, 1);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}