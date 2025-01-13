#include <cmath>
#include <iostream>
#include <set>
#include <vector>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_reduce.h>

int main() {
    using Range = oneapi::tbb::blocked_range<std::size_t>;
    using Set = std::set<double>;

    std::vector<Set> v;
    v.resize(1e4);

    for (std::size_t i = 1; i < v.size() + 1; ++i) {
      Set s;
      for (std::size_t j = 1; j < 1e2 + 1; ++j) {
        auto k = (i - 1) * 1e2 + j;
        s.insert(1.0 / (k + k));
      }
      v[i - 1] = std::move(s);
    }

    Set merged = oneapi::tbb::parallel_reduce(
      Range(0, v.size()),
      Set{},
      [&v](const Range& r, Set&& local_sum) {
        for (auto i = r.begin(); i != r.end(); ++i) {
            if (local_sum.size() < v[i].size()) {
              local_sum.swap(v[i]);
            }
            local_sum.merge(v[i]);
            if (not v[i].empty()) {
              std::cout << "something is wrong!\n";
            }
        }
        return std::move(local_sum);
      },
      [](Set&& x, Set&& y) {
        if (x.size() < y.size()) {
          x.swap(y);
        }
        x.merge(y);
        if (not y.empty()) {
          std::cout << "something is wrong!\n";
        }
        return std::move(x);
      }
    );

    if (merged.size() != 1e6) {
      return 1;
    }
    return 0;
}
