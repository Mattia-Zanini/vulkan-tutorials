#include "first_app.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <spdlog/spdlog.h>

int main() {
  spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
  spdlog::set_level(spdlog::level::debug);

  lve::FirstApp app{};

  try
  {
    app.run();
  }
  catch (const std::exception& e)
  {
    spdlog::error(e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}