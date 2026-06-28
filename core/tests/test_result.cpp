#include <cassert>

#include "dante/domain/Ids.hpp"
#include "dante/util/Result.hpp"

namespace {

dante::Result<int> divide(int a, int b) {
    if (b == 0) {
        return dante::fail("division by zero");
    }
    return a / b;
}

}  // namespace

// ponytail: one main, no framework. Proves the two things F0 core promises —
// Result ok/err round-trips, and ids are unique. Grow into ctest cases only
// when there's logic worth more than asserts.
int main() {
    const auto ok = divide(10, 2);
    assert(ok.has_value() && *ok == 5);

    const auto err = divide(1, 0);
    assert(!err.has_value() && err.error().message == "division by zero");

    assert(dante::newSessionId() != dante::newSessionId());
    assert(dante::newTabId() != dante::newTabId());

    return 0;
}
