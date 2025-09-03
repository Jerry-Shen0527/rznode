#pragma once

// #include <memory>

#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/XformComponent.h"
#include "GCore/GOP.h"
#include "nodes/web_server_oatpp/api.h"
#include "nodes/web_server_oatpp/geom_dto.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

/**
 * 需求：一个几何信息转换器，提供由几何信息到DTO的转换功能
 */

class WEB_SERVER_OATPP_API GeometryUtils {
   public:
    // 将单个几何体转换为对应的 DTO
    template<typename T>
    static oatpp::Object<T> convertGeometryToDto(
        const std::shared_ptr<Geometry>& geometry,
        const std::string& geom_id);

   public:
    static oatpp::Object<MeshDto> convertMeshToDto(
        const std::shared_ptr<MeshComponent>& mesh);
    static oatpp::Object<PointsDto> convertPointsToDto(
        const std::shared_ptr<PointsComponent>& points);
    static oatpp::Object<CurveDto> convertCurveToDto(
        const std::shared_ptr<CurveComponent>& curve);
    static oatpp::Vector<oatpp::Float32> convertMatrixToDto(
        const std::shared_ptr<XformComponent>& xform);
};

template<>
oatpp::Object<MeshDataDto> GeometryUtils::convertGeometryToDto<MeshDataDto>(
    const std::shared_ptr<Geometry>& geometry,
    const std::string& geom_id)
{
    if (!geometry)
        return nullptr;

    auto mesh = geometry->get_component<MeshComponent>();
    if (!mesh)
        return nullptr;  // 类型不匹配返回空

    auto dto = MeshDataDto::createShared();
    dto->id = geom_id;
    dto->type = "mesh";
    dto->mesh_data = convertMeshToDto(mesh);
    dto->transform =
        convertMatrixToDto(geometry->get_component<XformComponent>());
    return dto;
}

template<>
oatpp::Object<PointsDataDto> GeometryUtils::convertGeometryToDto<PointsDataDto>(
    const std::shared_ptr<Geometry>& geometry,
    const std::string& geom_id)
{
    if (!geometry)
        return nullptr;

    auto points = geometry->get_component<PointsComponent>();
    if (!points)
        return nullptr;  // 类型不匹配返回空

    auto dto = PointsDataDto::createShared();
    dto->id = geom_id;
    dto->type = "points";
    dto->points_data = convertPointsToDto(points);
    dto->transform =
        convertMatrixToDto(geometry->get_component<XformComponent>());
    return dto;
}

template<>
oatpp::Object<CurveDataDto> GeometryUtils::convertGeometryToDto<CurveDataDto>(
    const std::shared_ptr<Geometry>& geometry,
    const std::string& geom_id)
{
    if (!geometry)
        return nullptr;

    auto curve = geometry->get_component<CurveComponent>();
    if (!curve)
        return nullptr;  // 类型不匹配返回空

    auto dto = CurveDataDto::createShared();
    dto->id = geom_id;
    dto->type = "curve";
    dto->curve_data = convertCurveToDto(curve);
    dto->transform =
        convertMatrixToDto(geometry->get_component<XformComponent>());
    return dto;
}

USTC_CG_NAMESPACE_CLOSE_SCOPE