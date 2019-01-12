#include <csignal>
#include <iostream>
#include <pqrs/osx/input_source_monitor.hpp>

namespace {
auto global_wait = pqrs::make_thread_wait();
}

int main(void) {
  std::signal(SIGINT, [](int) {
    global_wait->notify();
  });

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  auto monitor = std::make_shared<pqrs::osx::input_source_monitor>(dispatcher);

  monitor->input_source_changed.connect([](auto&& input_source_ptr) {
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

  monitor->async_start();

  std::thread thread([&monitor] {
    global_wait->wait_notice();

    monitor = nullptr;

    CFRunLoopStop(CFRunLoopGetMain());
  });

  // ============================================================

  CFRunLoopRun();

  // ============================================================

  thread.join();

  dispatcher->terminate();
  dispatcher = nullptr;

  std::cout << "finished" << std::endl;

  return 0;
}
