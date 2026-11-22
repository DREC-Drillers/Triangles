#pragma once

#include <vector>

import geometry;
//FIXME подумать над созданием общего файла конфиг/settings
namespace triangles {

struct InitPolysData {
    using PolygonPoints = std::vector<geometry::point_t<3>>;

    std::vector<PolygonPoints> data;
};

} // namespace triangles
