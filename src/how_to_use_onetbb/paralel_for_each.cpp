#include <vector>

#include <oneapi/tbb/parallel_for_each.h>

int main() {
    using Range = oneapi::tbb::blocked_range<std::size_t>;

    std::vector<double> v;
    v.resize(1e6);
    for (std::size_t i = 0; i < v.size(); ++i) {
      v[i] = i;
    }

    oneapi::tbb::parallel_for_each(v.begin(), v.end(), [&v](double& d){
      d = d * d;
    });

    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i] != i * i) {
            return 1;
        }
    }
    return 0;
}
