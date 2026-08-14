#ifndef DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#endif


#include <doctest/doctest.h>
#include <Monoworks.hh>
#include <common/SafeQueue.hh>

#include <queue>
#include <type_traits>

namespace
{
    static_assert(std::is_same_v<Monoworks::Vector, glm::vec3>);
    static_assert(std::is_same_v<Monoworks::Vector2, glm::vec2>);
    static_assert(std::is_same_v<Monoworks::Vector4, glm::vec4>);
    static_assert(std::is_same_v<Monoworks::Matrix, glm::mat4>);
    static_assert(std::is_same_v<Monoworks::Quaternion, glm::quat>);

    struct SMemoryManagerScope
    {
        SMemoryManagerScope()
        {
            Monoworks::CMemoryManager::Init();
        }

        ~SMemoryManagerScope()
        {
            Monoworks::CMemoryManager::Shutdown();
        }
    };

    struct STestBase
    {
        virtual ~STestBase() = default;
        int BaseValue = 0;
    };

    struct STestDerived : STestBase
    {
        int DerivedValue = 0;

        STestDerived(int base, int derived)
        {
            BaseValue = base;
            DerivedValue = derived;
        }
    };
}

TEST_CASE("Common - aggregate defaults")
{
    Monoworks::SVersion version{};
    CHECK(version.Major == 0);
    CHECK(version.Minor == 0);
    CHECK(version.Patch == 0);

    Monoworks::SExtent2D extent2{};
    CHECK(extent2.Width == 0);
    CHECK(extent2.Height == 0);

    Monoworks::SExtent3D extent3{};
    CHECK(extent3.Width == 0);
    CHECK(extent3.Height == 0);
    CHECK(extent3.Depth == 0);

    CHECK(Monoworks::MW_NULL_MEMORY.Index == 0);
    CHECK(Monoworks::MW_NULL_MEMORY.Generation == 0);
}

TEST_CASE("CSafeQueue - FIFO behavior and size tracking")
{
    Monoworks::CSafeQueue<int> queue{};

    CHECK(queue.IsEmpty());
    CHECK(queue.GetSize() == 0);

    queue.Push(10);
    queue.Push(20);
    queue.Push(30);

    CHECK_FALSE(queue.IsEmpty());
    CHECK(queue.GetSize() == 3);
    CHECK(queue.Front() == 10);
    CHECK(queue.GetSize() == 2);
    CHECK(queue.Front() == 20);
    CHECK(queue.Front() == 30);
    CHECK(queue.IsEmpty());
    CHECK(queue.GetSize() == 0);
}

TEST_CASE("CSafeQueue - supports top-based containers")
{
    Monoworks::CSafeQueue<int, std::priority_queue<int>> queue{};

    queue.Push(3);
    queue.Push(1);
    queue.Push(5);

    CHECK(queue.GetSize() == 3);
    CHECK(queue.Front() == 5);
    CHECK(queue.Front() == 3);
    CHECK(queue.Front() == 1);
    CHECK(queue.IsEmpty());
}

TEST_CASE_FIXTURE(SMemoryManagerScope, "CMemoryManager - allocate, validate and delete handles")
{
    CHECK(Monoworks::CMemoryManager::Allocate(0) == Monoworks::MW_NULL_MEMORY);

    auto handle = Monoworks::CMemoryManager::Allocate(static_cast<u32>(sizeof(int)));
    REQUIRE(handle != Monoworks::MW_NULL_MEMORY);

    auto* value = static_cast<int*>(Monoworks::CMemoryManager::Get(handle));
    REQUIRE(value != nullptr);
    *value = 1234;

    CHECK(Monoworks::CMemoryManager::IsValid(handle));
    CHECK(*static_cast<int*>(Monoworks::CMemoryManager::Get(handle)) == 1234);

    Monoworks::CMemoryManager::Delete(handle);

    CHECK_FALSE(Monoworks::CMemoryManager::IsValid(handle));
    CHECK(Monoworks::CMemoryManager::Get(handle) == nullptr);
}

TEST_CASE_FIXTURE(SMemoryManagerScope, "CRef - create, copy, move, upcast and downcast")
{
    auto derived = Monoworks::CRef<STestDerived>::Create(7, 9);
    REQUIRE(derived);

    const auto handle = derived.GetHandle();
    CHECK(Monoworks::CMemoryManager::IsValid(handle));

    Monoworks::CRef<STestBase> base = derived;
    CHECK(base);
    CHECK(base == derived);
    CHECK(base->BaseValue == 7);

    auto moved = std::move(base);
    CHECK_FALSE(base);
    CHECK(moved);
    CHECK(moved->BaseValue == 7);

    auto downcast = moved.As<STestDerived>();
    REQUIRE(downcast);
    CHECK(downcast->BaseValue == 7);
    CHECK(downcast->DerivedValue == 9);

    moved = nullptr;
    derived = nullptr;
    downcast = nullptr;

    CHECK_FALSE(Monoworks::CMemoryManager::IsValid(handle));
}

TEST_CASE_FIXTURE(SMemoryManagerScope, "CMemoryManager - deleted handle invalidates and reused slot bumps generation")
{
    const auto a = Monoworks::CMemoryManager::Allocate(static_cast<u32>(sizeof(int)));
    REQUIRE(a != Monoworks::MW_NULL_MEMORY);

    const auto oldIndex = a.Index;
    const auto oldGeneration = a.Generation;

    Monoworks::CMemoryManager::Delete(a);
    CHECK_FALSE(Monoworks::CMemoryManager::IsValid(a));

    const auto b = Monoworks::CMemoryManager::Allocate(static_cast<u32>(sizeof(int)));
    REQUIRE(b != Monoworks::MW_NULL_MEMORY);

    CHECK(b.Index == oldIndex);
    CHECK(b.Generation == oldGeneration + 1);
}
