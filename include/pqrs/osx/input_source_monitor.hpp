#pragma once

// pqrs::osx::input_source_monitor v0.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::input_source_monitor` can be used safely in a multi-threaded environment.

// Destroy `pqrs::osx::input_source_monitor` before you call `CFRunLoopStop(CFRunLoopGetMain())`.

#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source.hpp>

namespace pqrs {
namespace osx {
class input_source_monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(cf::cf_ptr<TISInputSourceRef>)> input_source_changed;

  // Methods

  input_source_monitor(const input_source_monitor&) = delete;

  input_source_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                                started_(false) {
  }

  virtual ~input_source_monitor(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (started_) {
        return;
      }

      started_ = true;

      CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                      this,
                                      static_input_source_changed_callback,
                                      kTISNotifySelectedKeyboardInputSourceChanged,
                                      nullptr,
                                      CFNotificationSuspensionBehaviorDeliverImmediately);

      // Call `input_source_changed` slots for the current input source.

      input_source_changed_callback();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop(void) {
    if (!started_) {
      return;
    }

    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                       this,
                                       kTISNotifySelectedKeyboardInputSourceChanged,
                                       nullptr);

    started_ = false;
  }

  static void static_input_source_changed_callback(CFNotificationCenterRef center,
                                                   void* observer,
                                                   CFStringRef notification_name,
                                                   const void* observed_object,
                                                   CFDictionaryRef user_info) {
    auto self = static_cast<input_source_monitor*>(observer);
    if (self) {
      self->input_source_changed_callback();
    }
  }

  void input_source_changed_callback(void) {
    enqueue_to_dispatcher([this] {
      dispatch_sync(
          dispatch_get_main_queue(),
          ^{
            if (auto input_source = osx::input_source::make_current_keyboard_input_source()) {
              enqueue_to_dispatcher([this, input_source] {
                input_source_changed(input_source);
              });
            }
          });
    });
  }

  bool started_;
};
} // namespace osx
} // namespace pqrs
