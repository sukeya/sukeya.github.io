#include <cmath>
#include <iostream>
#include <vector>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_reduce.h>

int main() {
    using Range = oneapi::tbb::blocked_range<std::size_t>;

    std::vector<double> v;
    v.resize(1e6);

    for (std::size_t i = 1; i < v.size() + 1; ++i) {
      v[i] = 1.0 / (i * i);
    }

    double sum = oneapi::tbb::parallel_reduce(
      Range(0, v.size()),
      0.0,
      [&v](const Range& r, double local_sum) {
        for (auto i = r.begin(); i != r.end(); ++i) {
            local_sum += v[i];
        }
        return local_sum;
      },
      [](double x, double y) {
        return x + y;
      }
    );

    std::cout << std::sqrt(6 * sum) << '\n';

    return 0;
}
