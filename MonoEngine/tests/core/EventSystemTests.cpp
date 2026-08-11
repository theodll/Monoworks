#ifndef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#endif
#include <doctest/doctest.h>

#include <Monoworks.hh>

TEST_CASE("CEventSystem - Emit deffered events")
{
	Monoworks::CEventManager::Init();
}
