#ifndef DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#endif

#include <Monoworks.hh>

#include <doctest/doctest.h>


#include <string_view>

namespace
{
    static_assert(sizeof(Monoworks::SEvent) == 64);
    static_assert(alignof(Monoworks::SEvent) == 64);
    static_assert(Monoworks::impl::ValidEvent<Monoworks::Events::SWindowClose>);
    static_assert(Monoworks::impl::ValidEvent<Monoworks::Events::SWindowResize>);
}

TEST_CASE("SEvent - stores and retrieves typed payload")
{
    Monoworks::SEvent event{};
    Monoworks::Events::SWindowResize resize{};
    resize.NewExtent = { 1920, 1080 };

    event.SetType(Monoworks::MW_EVENT_WINDOW_RESIZE);
    event.SetPayload(resize);

    const auto restored = event.GetPayload<Monoworks::Events::SWindowResize>();
    CHECK(event.GetType() == Monoworks::MW_EVENT_WINDOW_RESIZE);
    CHECK(restored.NewExtent.Width == 1920);
    CHECK(restored.NewExtent.Height == 1080);
}

TEST_CASE("EventTypeToString - returns readable names")
{
    CHECK(std::string_view(Monoworks::EventTypeToString(Monoworks::MW_EVENT_APP_UPDATE)) == "MW_EVENT_APP_UPDATE");
    CHECK(std::string_view(Monoworks::EventTypeToString(Monoworks::MW_EVENT_MOUSE_SCROLLED)) == "MW_EVENT_MOUSE_SCROLLED");
    CHECK(std::string_view(Monoworks::EventTypeToString(static_cast<Monoworks::EEventType>(255))) == "Unknown Event");
}

TEST_CASE("CEventManager - deferred events are queued then dispatched")
{
    Monoworks::CEventManager::Init();

    int callbackCount = 0;
    Monoworks::CEventManager::Subscribe(Monoworks::MW_EVENT_MOUSE_MOVED, [&callbackCount](Monoworks::SEvent& e)
    {
        ++callbackCount;
        const auto payload = e.GetPayload<Monoworks::Events::SMouseMoved>();
        CHECK(payload.MouseX == doctest::Approx(4.0f));
        CHECK(payload.MouseY == doctest::Approx(8.0f));
        return false;
    });

    Monoworks::Events::SMouseMoved moved{};
    moved.MouseX = 4.0f;
    moved.MouseY = 8.0f;

    Monoworks::CEventManager::EmitEvent(moved, Monoworks::MW_EVENT_MOUSE_MOVED);
    CHECK(callbackCount == 0);

    Monoworks::CEventManager::ProcessEvents();
    CHECK(callbackCount == 1);

    Monoworks::CEventManager::Shutdown();
}

TEST_CASE("CEventManager - callback chain stops when handled")
{
    Monoworks::CEventManager::Init();

    int first = 0;
    int second = 0;

    Monoworks::CEventManager::Subscribe(Monoworks::MW_EVENT_KEY_RELEASED, [&first](Monoworks::SEvent&)
    {
        ++first;
        return true;
    });

    Monoworks::CEventManager::Subscribe(Monoworks::MW_EVENT_KEY_RELEASED, [&second](Monoworks::SEvent&)
    {
        ++second;
        return false;
    });

    Monoworks::Events::SKeyReleased released{};
    released.Code = Monoworks::MW_SCANCODE_A;

    Monoworks::CEventManager::EmitEventNonDeffered(released, Monoworks::MW_EVENT_KEY_RELEASED);

    CHECK(first == 1);
    CHECK(second == 0);

    Monoworks::CEventManager::Shutdown();
}
