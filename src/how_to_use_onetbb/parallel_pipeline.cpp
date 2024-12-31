#include <algorithm>
#include <iostream>
#include <random>

#include <oneapi/tbb/parallel_pipeline.h>

int main() {
  using Seed  = std::random_device::result_type;
  using Seeds = std::array<Seed, 2>;
  using Coord = std::array<double, 2>;

  std::random_device seed_gen;

  std::size_t counter = 0;

  std::size_t i = 0;
  constexpr std::size_t N = 1e6;

  oneapi::tbb::parallel_pipeline(
    N,
    oneapi::tbb::make_filter<void, Seeds>(
      oneapi::tbb::filter_mode::serial_out_of_order,
      [&](oneapi::tbb::flow_control& fc) -> Seeds {
        if (i < N) {
          ++i;
          return {seed_gen(), seed_gen()};
        } else {
          fc.stop();
          return {0, 0};
        }
      }
    ) &
    oneapi::tbb::make_filter<Seeds, std::size_t>(
      oneapi::tbb::filter_mode::parallel,
      [&](const Seeds& seeds) -> std::size_t {
        std::uniform_real_distribution<double> dist(0, 1);
        Coord coord;
        for (std::size_t i = 0; i < 2; ++i) {
          std::ranlux48 engine(seeds[i]);
          coord[i] = dist(engine);
        }
        if (coord[0] * coord[0] + coord[1] * coord[1] < 1) {
          return 1;
        }
        return 0;
      }
    ) &
    oneapi::tbb::make_filter<std::size_t, void>(
      oneapi::tbb::filter_mode::serial_out_of_order,
      [&](std::size_t i) -> void {
        counter += i;
      }
    )
  );

  std::cout << static_cast<double>(4 * counter) / N << std::endl;

  return 0;
}