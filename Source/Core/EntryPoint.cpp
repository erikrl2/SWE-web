#include "Application.hpp"

int main(int argc, char** argv) {
  auto app = Core::createApplication(argc, argv);
  app->run();
  delete app;

  return 0;
}
