#ifdef GEOM_EXTENSION
#include "nodes/web_server/geom_utils.hpp"

#include <iostream>
#include <string>

#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/XformComponent.h"
#include "GCore/GOP.h"
#include "nodes/core/api.h"
#include "nodes/web_server/geom_dto.hpp"
#include "oatpp/Types.hpp"
#include "spdlog/spdlog.h"

USTC_CG_NAMESPACE_OPEN_SCOPE

oatpp::Object<GeometryMessageDto> GeometryUtils::convertGeometryMessageToDto(
    const std::string& type,
    const std::string& scene_id,
    const std::vector<Geometry>& geometries,
    const std::vector<std::string>& geom_ids)
{
    if (geometries.size() != geom_ids.size()) {
        spdlog::error(
            "GeometryUtils::convertGeometryMessageToDto: Geometries size ("
            "{} ) does not match geom_ids size ( {} )",
            geometries.size(),
            geom_ids.size());
        return nullptr;
    }

    auto message_dto = GeometryMessageDto::createShared();
    message_dto->type = type;
    message_dto->scene_id = scene_id;
    message_dto->geometries =
        oatpp::Vector<oatpp::Object<GeometryDataDto>>::createShared();
    message_dto->geometries->reserve(geometries.size());
    for (int i = 0; i < geometries.size(); ++i) {
        auto& geom = geometries[i];
        auto& geom_id = geom_ids[i];
        auto geom_dto = convertGeometryToDto(geom, geom_id);
        if (geom_dto) {
            message_dto->geometries->push_back(geom_dto);
        }
        else {
            spdlog::warn(
                "GeometryUtils::convertGeometryMessageToDto: Failed to convert "
                "geometry with geom_id '{}'",
                geom_id);
        }
    }
    message_dto->timestamp = static_cast<v_int64>(time(nullptr));

    return message_dto;
}

oatpp::Object<GeometryDataDto> GeometryUtils::convertGeometryToDto(
    const Geometry& geometry,
    const std::string& geom_id)
{
    auto geom_dto = GeometryDataDto::createShared();
    geom_dto->id = geom_id;

    auto xform = geometry.get_component<XformComponent>();
    if (xform) {
        geom_dto->transform = convertMatrixToDto(xform);
    }
    else {
        // 默认单位矩阵
        // clang-format off
        geom_dto->transform = { 1.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 1.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 1.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f };
        // clang-format on
    }

    auto mesh = geometry.get_component<MeshComponent>();
    auto points = geometry.get_component<PointsComponent>();
    auto curve = geometry.get_component<CurveComponent>();

    if (mesh) {
        geom_dto->type = "mesh";
        geom_dto->geometry_data = convertMeshToDto(mesh);
    }
    else if (points) {
        geom_dto->type = "points";
        geom_dto->geometry_data = convertPointsToDto(points);
    }
    else if (curve) {
        geom_dto->type = "curve";
        geom_dto->geometry_data = convertCurveToDto(curve);
    }
    else {
        spdlog::warn(
            "GeometryUtils::convertGeometryToDto: Geometry with geom_id '{}' "
            "has no supported component",
            geom_id);
        return nullptr;
    }
    return geom_dto;
}

oatpp::Object<MeshDataDto> GeometryUtils::convertMeshToDto(
    const std::shared_ptr<MeshComponent>& mesh)
{
    if (!mesh) {
        return nullptr;
    }

    auto mesh_dto = MeshDataDto::createShared();

    auto vertices = mesh->get_vertices();
    auto face_vertex_counts = mesh->get_face_vertex_counts();
    auto face_vertex_indices = mesh->get_face_vertex_indices();
    auto normals = mesh->get_normals();
    auto colors = mesh->get_display_color();
    auto uvs = mesh->get_texcoords_array();

    bool has_normals = !normals.empty();
    bool normals_available = has_normals && (normals.size() == vertices.size());
    bool has_colors = !colors.empty();
    bool colors_available = has_colors && (colors.size() == vertices.size());
    bool has_uvs = !uvs.empty();
    bool uvs_available = has_uvs && (uvs.size() == vertices.size());

    // 填充顶点数据
    mesh_dto->vertices = oatpp::Vector<oatpp::Float32>::createShared();
    mesh_dto->vertices->reserve(vertices.size());
    for (const auto& v : vertices) {
        mesh_dto->vertices->push_back(v.x);
        mesh_dto->vertices->push_back(v.y);
        mesh_dto->vertices->push_back(v.z);
    }

    // 填充面数据
    mesh_dto->face_vertex_counts = oatpp::Vector<oatpp::Int32>::createShared();
    mesh_dto->face_vertex_counts->reserve(face_vertex_counts.size());
    for (const auto& count : face_vertex_counts) {
        mesh_dto->face_vertex_counts->push_back(count);
    }
    mesh_dto->face_vertex_indices = oatpp::Vector<oatpp::Int32>::createShared();
    mesh_dto->face_vertex_indices->reserve(face_vertex_indices.size());
    for (const auto& index : face_vertex_indices) {
        mesh_dto->face_vertex_indices->push_back(index);
    }

    // 填充法线数据
    if (normals_available) {
        mesh_dto->normals = oatpp::Vector<oatpp::Float32>::createShared();
        mesh_dto->normals->reserve(normals.size() * 3);
        for (const auto& n : normals) {
            mesh_dto->normals->push_back(n.x);
            mesh_dto->normals->push_back(n.y);
            mesh_dto->normals->push_back(n.z);
        }
    }

    // 填充颜色数据
    if (colors_available) {
        mesh_dto->colors = oatpp::Vector<oatpp::Float32>::createShared();
        mesh_dto->colors->reserve(colors.size() * 3);
        for (const auto& c : colors) {
            mesh_dto->colors->push_back(c.r);
            mesh_dto->colors->push_back(c.g);
            mesh_dto->colors->push_back(c.b);
        }
    }

    // 填充 UV 数据
    if (uvs_available) {
        mesh_dto->uvs = oatpp::Vector<oatpp::Float32>::createShared();
        mesh_dto->uvs->reserve(uvs.size() * 2);
        for (const auto& uv : uvs) {
            mesh_dto->uvs->push_back(uv.x);
            mesh_dto->uvs->push_back(uv.y);
        }
    }

    return mesh_dto;
}

oatpp::Object<PointsDataDto> GeometryUtils::convertPointsToDto(
    const std::shared_ptr<PointsComponent>& points)
{
    if (!points) {
        return nullptr;
    }

    auto points_dto = PointsDataDto::createShared();

    auto vertices = points->get_vertices();
    auto normals = points->get_normals();
    auto colors = points->get_display_color();
    auto widths = points->get_width();

    bool has_normals = !normals.empty();
    bool normals_available = has_normals && (normals.size() == vertices.size());
    bool has_colors = !colors.empty();
    bool colors_available = has_colors && (colors.size() == vertices.size());
    bool has_widths = !widths.empty();
    bool widths_available = has_widths && (widths.size() == vertices.size());

    // 填充顶点数据
    points_dto->vertices = oatpp::Vector<oatpp::Float32>::createShared();
    points_dto->vertices->reserve(vertices.size() * 3);
    for (const auto& v : vertices) {
        points_dto->vertices->push_back(v.x);
        points_dto->vertices->push_back(v.y);
        points_dto->vertices->push_back(v.z);
    }

    // 填充法线数据
    if (normals_available) {
        points_dto->normals = oatpp::Vector<oatpp::Float32>::createShared();
        points_dto->normals->reserve(normals.size() * 3);
        for (const auto& n : normals) {
            points_dto->normals->push_back(n.x);
            points_dto->normals->push_back(n.y);
            points_dto->normals->push_back(n.z);
        }
    }

    // 填充颜色数据
    if (colors_available) {
        points_dto->colors = oatpp::Vector<oatpp::Float32>::createShared();
        points_dto->colors->reserve(colors.size() * 3);
        for (const auto& c : colors) {
            points_dto->colors->push_back(c.r);
            points_dto->colors->push_back(c.g);
            points_dto->colors->push_back(c.b);
        }
    }

    // 填充宽度数据
    if (widths_available) {
        points_dto->widths = oatpp::Vector<oatpp::Float32>::createShared();
        points_dto->widths->reserve(widths.size());
        for (const auto& w : widths) {
            points_dto->widths->push_back(w);
        }
    }

    return points_dto;
}

oatpp::Object<CurveDataDto> GeometryUtils::convertCurveToDto(
    const std::shared_ptr<CurveComponent>& curve)
{
    if (!curve) {
        return nullptr;
    }

    auto curve_dto = CurveDataDto::createShared();

    auto vertices = curve->get_vertices();
    auto vert_counts = curve->get_vert_count();
    auto normals = curve->get_curve_normals();
    auto colors = curve->get_display_color();
    auto widths = curve->get_width();
    bool periodic = curve->get_periodic();

    bool has_normals = !normals.empty();
    bool normals_available = has_normals && (normals.size() == vertices.size());
    bool has_colors = !colors.empty();
    bool colors_available = has_colors && (colors.size() == vertices.size());
    bool has_widths = !widths.empty();
    bool widths_available = has_widths && (widths.size() == vertices.size());

    // 填充顶点数据
    curve_dto->vertices = oatpp::Vector<oatpp::Float32>::createShared();
    curve_dto->vertices->reserve(vertices.size() * 3);
    for (const auto& v : vertices) {
        curve_dto->vertices->push_back(v.x);
        curve_dto->vertices->push_back(v.y);
        curve_dto->vertices->push_back(v.z);
    }

    // 填充顶点计数数据
    curve_dto->vertex_counts = oatpp::Vector<oatpp::Int32>::createShared();
    curve_dto->vertex_counts->reserve(vert_counts.size());
    for (const auto& count : vert_counts) {
        curve_dto->vertex_counts->push_back(count);
    }

    // 填充法线数据
    if (normals_available) {
        curve_dto->normals = oatpp::Vector<oatpp::Float32>::createShared();
        curve_dto->normals->reserve(normals.size() * 3);
        for (const auto& n : normals) {
            curve_dto->normals->push_back(n.x);
            curve_dto->normals->push_back(n.y);
            curve_dto->normals->push_back(n.z);
        }
    }

    // 填充颜色数据
    if (colors_available) {
        curve_dto->colors = oatpp::Vector<oatpp::Float32>::createShared();
        curve_dto->colors->reserve(colors.size() * 3);
        for (const auto& c : colors) {
            curve_dto->colors->push_back(c.r);
            curve_dto->colors->push_back(c.g);
            curve_dto->colors->push_back(c.b);
        }
    }

    // 填充宽度数据
    if (widths_available) {
        curve_dto->widths = oatpp::Vector<oatpp::Float32>::createShared();
        curve_dto->widths->reserve(widths.size());
        for (const auto& w : widths) {
            curve_dto->widths->push_back(w);
        }
    }

    curve_dto->periodic = periodic;

    return curve_dto;
}

oatpp::Vector<oatpp::Float32> GeometryUtils::convertMatrixToDto(
    const std::shared_ptr<XformComponent>& xform)
{
    auto matrix_dto = oatpp::Vector<oatpp::Float32>::createShared();
    matrix_dto->reserve(16);

    if (xform) {
        auto transform = xform->get_transform();
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                matrix_dto->push_back(transform[i][j]);
            }
        }
    }
    else {
        // 如果没有变换组件，使用单位矩阵
        // clang-format off
            matrix_dto =
                { 1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f };
        // clang-format on
    }

    return matrix_dto;
}

USTC_CG_NAMESPACE_CLOSE_SCOPE

#endif