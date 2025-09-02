#pragma once

#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/GOP.h"
#include "nodes/web_server_oatpp/api.h"
#include "nodes/web_server_oatpp/geom_dto.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

/**
 * 需求：一个几何信息转换器，提供由几何信息到DTO的转换功能
 */

class WEB_SERVER_OATPP_API GeometryUtils {
   public:
    // 将几何体列表转换为 DTO 列表
    static oatpp::Vector<oatpp::Object<GeometryDataDto>> convertGeometriesToDto(
        const std::vector<std::shared_ptr<Geometry>>& geometries,
        const std::vector<std::string>& geom_ids);

    // 将单个几何体转换为对应的 DTO
    static oatpp::Object<GeometryDataDto> convertGeometryToDto(
        const std::shared_ptr<Geometry>& geometry,
        const std::string& geom_id);

   private:
    static oatpp::Object<MeshDataDto> convertMeshToDto(
        const std::shared_ptr<MeshComponent>& mesh);
    static oatpp::Object<PointsDataDto> convertPointsToDto(
        const std::shared_ptr<PointsComponent>& points);
    static oatpp::Object<CurveDataDto> convertCurveToDto(
        const std::shared_ptr<CurveComponent>& curve);
};

USTC_CG_NAMESPACE_CLOSE_SCOPE