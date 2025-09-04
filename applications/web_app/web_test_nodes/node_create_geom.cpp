#include "GCore/create_geom.h"
#include "nodes/core/def/node_def.hpp"

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_UI(create_circle_face)
{
    return "Create Circle Face";
}

NODE_DECLARATION_FUNCTION(create_circle_face)
{
    b.add_input<int>("resolution").min(1).max(100).default_val(10);
    b.add_input<float>("radius").min(0.1f).max(100.0f).default_val(1.0f);
    b.add_output<Geometry>("geometry");
}

NODE_EXECUTION_FUNCTION(create_circle_face)
{
    auto resolution = params.get_input<int>("resolution");
    auto radius = params.get_input<float>("radius");

    auto geom = create_circle_face(resolution, radius);
    params.set_output("geometry", geom);
    return true;
}

NODE_DEF_CLOSE_SCOPE