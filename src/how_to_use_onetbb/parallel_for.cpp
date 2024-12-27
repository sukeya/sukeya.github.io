#include <vector>

#include <oneapi/tbb/parallel_for.h>

int main() {
    using Range = oneapi::tbb::blocked_range<std::size_t>;

    std::vector<double> v;
    v.resize(1e6);

    oneapi::tbb::parallel_for(Range(1, v.size() + 1), [&v](const Range& r){
        for (auto i = r.begin(); i != r.end(); ++i) {
            v[i - 1] = 1.0 / (i * i);
        }
    });

    for (std::size_t i = 1; i < v.size() + 1; ++i) {
        if (v[i - 1] != 1.0 / (i * i)) {
            return 1;
        }
    }
    return 0;
}
