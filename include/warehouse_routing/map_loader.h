#pragma once
#include <string>
#include "warehouse_routing/grid.h"

namespace warehouse_routing {

Grid loadMap(const std::string& filePath);

}