#include <boost/ut.hpp>
#include <pqrs/osx/input_source_monitor.hpp>

#include <memory>

namespace {
void wait_dispatcher(const std::shared_ptr<pqrs::dispatcher::dispatcher>& dispatcher) {
  auto object_id = pqrs::dispatcher::make_new_object_id();
  dispatcher->attach(object_id);

  for (auto i = 0; i < 2; ++i) {
    auto wait = pqrs::make_thread_wait();
    dispatcher->enqueue(object_id, [wait] {
      wait->notify();
    });
    wait->wait_notice();
  }

  dispatcher->detach(object_id);
}
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "monitor"_test = [] {
    auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

    auto monitor = std::make_shared<pqrs::osx::input_source_monitor>(dispatcher);

    expect(monitor != nullptr);

    monitor = nullptr;

    dispatcher->terminate();
    dispatcher = nullptr;
  };

  "stop_without_start"_test = [] {
    auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

    auto monitor = std::make_shared<pqrs::osx::input_source_monitor>(dispatcher);

    monitor->async_stop();
    wait_dispatcher(dispatcher);

    monitor = nullptr;

    dispatcher->terminate();
    dispatcher = nullptr;
  };

  "destroy_after_pending_stop"_test = [] {
    auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

    auto monitor = std::make_shared<pqrs::osx::input_source_monitor>(dispatcher);

    monitor->async_stop();
    monitor = nullptr;

    dispatcher->terminate();
    dispatcher = nullptr;
  };

  return 0;
}
