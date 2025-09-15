#pragma once

// #include <memory>

#include <memory>
#include <string>
#include <vector>

#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/XformComponent.h"
#include "GCore/GOP.h"
#include "nodes/web_server/api.h"
#include "nodes/web_server/geom_dto.hpp"
#include "oatpp/Types.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

/**
 * 需求：一个几何信息转换器，提供由几何信息到DTO的转换功能
 */

class WEB_SERVER_API GeometryUtils {
   public:
    // 将几何体DTO转换为几何消息
    static oatpp::Object<GeometryMessageDto> convertGeometryMessageToDto(
        const std::string& type,
        const std::string& scene_id,
        const std::vector<Geometry>& geometries,
        const std::vector<std::string>& geom_ids);

   private:
    // 将单个几何体转换为对应的 DTO
    static oatpp::Object<GeometryDataDto> convertGeometryToDto(
        const Geometry& geometry,
        const std::string& geom_id);

    static oatpp::Object<MeshDataDto> convertMeshToDto(
        const std::shared_ptr<MeshComponent>& mesh);
    static oatpp::Object<PointsDataDto> convertPointsToDto(
        const std::shared_ptr<PointsComponent>& points);
    static oatpp::Object<CurveDataDto> convertCurveToDto(
        const std::shared_ptr<CurveComponent>& curve);

    static oatpp::Vector<oatpp::Float32> convertMatrixToDto(
        const std::shared_ptr<XformComponent>& xform);
};

USTC_CG_NAMESPACE_CLOSE_SCOPE