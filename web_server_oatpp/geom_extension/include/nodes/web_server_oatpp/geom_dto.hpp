#pragma once

#include <cstdint>

#include "nodes/web_server_oatpp/api.h"
#include "oatpp/Types.hpp"
#include "oatpp/macro/codegen.hpp"

/**
 * 参考文档：
 * https://oatpp.io/docs/components/dto/
 */

#include OATPP_CODEGEN_BEGIN(DTO)  //<- Begin Codegen

USTC_CG_NAMESPACE_OPEN_SCOPE

class WEB_SERVER_OATPP_API GeometryDataDto : public oatpp::DTO {
    DTO_INIT(GeometryDataDto, DTO)

    // 几何体ID
    DTO_FIELD(String, id) = "default";
    // 几何体类型，如 "mesh", "points", "curve"
    DTO_FIELD(String, type) = "mesh";
    // clang-format off
    DTO_FIELD(Vector<Float32>, transform) =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };
    // clang-format on
};

class WEB_SERVER_OATPP_API MeshDto : public oatpp::DTO {
    DTO_INIT(MeshDto, DTO)

    DTO_FIELD(Vector<Float32>, vertices) = { };
    DTO_FIELD(Vector<Int32>, face_vertex_counts) = { };
    DTO_FIELD(Vector<Int32>, face_vertex_indices) = { };
    DTO_FIELD(Vector<Float32>, normals) = { };
    DTO_FIELD(Vector<Float32>, colors) = { };
    DTO_FIELD(Vector<Float32>, uvs) = { };
};

class WEB_SERVER_OATPP_API MeshDataDto : public GeometryDataDto {
    DTO_INIT(MeshDataDto, GeometryDataDto)

    DTO_FIELD(Object<MeshDto>, mesh_data) = MeshDto::createShared();
};

class WEB_SERVER_OATPP_API PointsDto : public oatpp::DTO {
    DTO_INIT(PointsDto, DTO)

    DTO_FIELD(Vector<Float32>, vertices) = { };
    DTO_FIELD(Vector<Float32>, normals) = { };
    DTO_FIELD(Vector<Float32>, colors) = { };
    DTO_FIELD(Vector<Float32>, widths) = { };
};

class WEB_SERVER_OATPP_API PointsDataDto : public GeometryDataDto {
    DTO_INIT(PointsDataDto, GeometryDataDto)

    DTO_FIELD(Object<PointsDto>, points_data) = PointsDto::createShared();
};

class WEB_SERVER_OATPP_API CurveDto : public oatpp::DTO {
    DTO_INIT(CurveDto, DTO)

    DTO_FIELD(Vector<Float32>, vertices) = { };
    DTO_FIELD(Vector<Int32>, vert_count) = { };
    DTO_FIELD(Vector<Float32>, colors) = { };
    DTO_FIELD(Vector<Float32>, widths) = { };
    DTO_FIELD(Vector<Float32>, normals) = { };
    DTO_FIELD(Boolean, periodic) = false;
};

class WEB_SERVER_OATPP_API CurveDataDto : public GeometryDataDto {
    DTO_INIT(CurveDataDto, GeometryDataDto)

    DTO_FIELD(Object<CurveDto>, curve_data) = CurveDto::createShared();
};

class WEB_SERVER_OATPP_API GeometryMessageDto : public oatpp::DTO {
    DTO_INIT(GeometryMessageDto, DTO)

    // 'geometry_update' | 'geometry_clear' | 'scene_update'
    DTO_FIELD(String, type);
    // 当前由于只有一棵节点树，因此只有一个场景，scene_id
    // 可固定为 "default"
    DTO_FIELD(String, scene_id) = "default";
    DTO_FIELD(Vector<Object<GeometryDataDto>>, geometries) = { };
    DTO_FIELD(Int64, timestamp) = (int64_t)0;  // 时间戳，毫秒
};

USTC_CG_NAMESPACE_CLOSE_SCOPE

#include OATPP_CODEGEN_END(DTO)  //<- End Codegen