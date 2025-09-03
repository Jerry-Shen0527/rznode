#include "GCore/create_geom.h"
#include "nodes/core/def/node_def.hpp"

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_UI(create_grid)
{
    return "Create Grid";
}

NODE_DECLARATION_FUNCTION(create_grid)
{
    b.add_input<int>("resolution").min(1).max(100).default_val(10);
    b.add_input<float>("size").min(0.1f).max(100.0f).default_val(1.0f);
    b.add_output<Geometry>("geometry");
}

NODE_EXECUTION_FUNCTION(create_grid)
{
    auto resolution = params.get_input<int>("resolution");
    auto size = params.get_input<float>("size");

    auto geom = create_grid(resolution, size);
    params.set_output("geometry", geom);
    return true;
}

NODE_DEF_CLOSE_SCOPE