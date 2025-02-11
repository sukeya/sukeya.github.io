#include <chrono>
#include <list>
#include <thread>

#include <oneapi/tbb/parallel_for_each.h>

struct Timer {
  std::chrono::milliseconds ms;
};

int main() {
  using Range = oneapi::tbb::blocked_range<std::size_t>;
  using MilliSeconds = std::chrono::milliseconds;

  std::list<Timer> v;
  v.emplace_back(MilliSeconds{1});

  const auto sleep_start = std::chrono::system_clock::now();
  const MilliSeconds max_sleep = std::chrono::seconds(1);
  oneapi::tbb::parallel_for_each(
      v.begin(), v.end(), [&](Timer& sleep_time, auto& feeder) {
        std::this_thread::sleep_for(sleep_time.ms);
        const auto now = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<MilliSeconds>(now - sleep_start) < max_sleep) {
          feeder.add(Timer{sleep_time.ms});
        }
      });

  const auto now = std::chrono::system_clock::now();
  if (std::chrono::duration_cast<MilliSeconds>(now - sleep_start) < max_sleep) {
    return 1;
  }
  return 0;
}
