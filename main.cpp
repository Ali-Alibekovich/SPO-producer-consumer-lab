#include <cstring>
#include <iostream>
#include "producer_consumer.h"

int main(int argc, char *argv[]) {
  int size_consumer, time_sleep;
  bool debug = false;
  //аргументы программы
  if (argc > 2) {
    size_consumer = std::stoi(argv[1]);
    time_sleep = std::stoi(argv[2]);
    if (argc > 3) {
      debug = !std::strcmp(argv[3], "-debug");
    }
  } else {
    std::cout << "Недостаточно агрументов или они некорректны";
    return 1;
  }

  std::cout << run_threads(size_consumer, time_sleep, debug) << std::endl;
  return 0;
}
