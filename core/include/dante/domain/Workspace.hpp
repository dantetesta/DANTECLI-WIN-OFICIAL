#pragma once

#include <string>
#include <vector>

#include "dante/domain/Tab.hpp"

namespace dante {

struct Workspace {
    std::string name;
    std::vector<Tab> tabs;
};

}  // namespace dante
