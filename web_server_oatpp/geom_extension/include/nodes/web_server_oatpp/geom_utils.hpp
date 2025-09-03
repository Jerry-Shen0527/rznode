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

// 模板偏特化声明
template<>
WEB_SERVER_OATPP_API oatpp::Object<MeshDataDto> GeometryUtils::convertGeometryToDto<MeshDataDto>(
    const std::shared_ptr<Geometry>& geometry,
    const std::string& geom_id);

template<>
WEB_SERVER_OATPP_API oatpp::Object<PointsDataDto> GeometryUtils::convertGeometryToDto<PointsDataDto>(
    const std::shared_ptr<Geometry>& geometry,
    const std::string& geom_id);

template<>
WEB_SERVER_OATPP_API oatpp::Object<CurveDataDto> GeometryUtils::convertGeometryToDto<CurveDataDto>(
    const std::shared_ptr<Geometry>& geometry,
    const std::string& geom_id);

USTC_CG_NAMESPACE_CLOSE_SCOPE