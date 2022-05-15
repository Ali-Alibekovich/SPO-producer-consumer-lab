#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <producer_consumer.h>
#include <iostream>

TEST_CASE("1 consumer 0 timesleep" * doctest::timeout(1)) {
  std::istringstream in("15 6 7 3 2");
  std::cin.rdbuf(in.rdbuf());
  CHECK(33 == run_threads(1, 0, false));
}

TEST_CASE("5 consumer 0 timesleep" * doctest::timeout(1)) {
  std::istringstream in("15 6 7 3 2");
  std::cin.rdbuf(in.rdbuf());
  CHECK(33 == run_threads(5, 0, false));
}

TEST_CASE("3 consumer 3 timesleep" * doctest::timeout(1)) {
  std::istringstream in("15 6 7 3 2");
  std::cin.rdbuf(in.rdbuf());
  CHECK(33 == run_threads(3, 3, false));
}
