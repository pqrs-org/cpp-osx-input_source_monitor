#include <csignal>
#include <iostream>
#include <pqrs/osx/input_source_monitor.hpp>

namespace {
std::shared_ptr<pqrs::osx::input_source_monitor> global_monitor;
}

int main(void) {
  std::signal(SIGINT, [](int) {
    // Destroy monitor before `CFRunLoopStop(CFRunLoopGetMain())`.
    global_monitor = nullptr;

    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  global_monitor = std::make_shared<pqrs::osx::input_source_monitor>(dispatcher);

  global_monitor->input_source_changed.connect([](auto&& input_source_ptr) {
    if (input_source_ptr) {
      std::cout << "input_source_changed: ";
      dispatch_sync(
          dispatch_get_main_queue(),
          ^{
            if (auto input_source_id = pqrs::osx::input_source::make_input_source_id(*input_source_ptr)) {
              std::cout << *input_source_id;
            }
          });
      std::cout << std::endl;
    }
  });

  global_monitor->async_start();

  // ============================================================

  CFRunLoopRun();

  // ============================================================

  dispatcher->terminate();
  dispatcher = nullptr;

  std::cout << "finished" << std::endl;

  return 0;
}
