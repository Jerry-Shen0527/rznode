#pragma once

#include "nodes/web_server/api.h"
#include "oatpp/Types.hpp"
#include "oatpp/macro/codegen.hpp"

/**
 * 参考文档：
 * https://oatpp.io/docs/components/dto/
 */

#include OATPP_CODEGEN_BEGIN(DTO)  //<- Begin Codegen

USTC_CG_NAMESPACE_OPEN_SCOPE

class WEB_SERVER_API StatusDto : public oatpp::DTO {
    DTO_INIT(StatusDto, DTO)

    DTO_FIELD(String, status) = "running";
    DTO_FIELD(String, message);
    DTO_FIELD(Boolean, has_node_system);
};

class WEB_SERVER_API ValueTypeDto : public oatpp::DTO {
    DTO_INIT(ValueTypeDto, DTO)

    DTO_FIELD(String, type_name);
};

class WEB_SERVER_API ValueTypesDto : public oatpp::DTO {
    DTO_INIT(ValueTypesDto, DTO)

    DTO_FIELD(Vector<Object<ValueTypeDto>>, value_types) = { };
};

class WEB_SERVER_API NodeTypeDto : public oatpp::DTO {
    DTO_INIT(NodeTypeDto, DTO)

    DTO_FIELD(String, id_name);
    DTO_FIELD(String, ui_name);

    class WEB_SERVER_API SocketDto : public oatpp::DTO {
        DTO_INIT(SocketDto, DTO)

        DTO_FIELD(String, name);
        DTO_FIELD(String, identifier);
        DTO_FIELD(String, type);
        DTO_FIELD(Boolean, optional) = false;

        // 默认值、最小值、最大值等（JSON格式）
        DTO_FIELD(String, default_value);
        DTO_FIELD(String, min_value);
        DTO_FIELD(String, max_value);
    };

    class WEB_SERVER_API SocketGroupDto : public oatpp::DTO {
        DTO_INIT(SocketGroupDto, DTO)

        DTO_FIELD(String, identifier);
        DTO_FIELD(String, type);  // "input" or "output"
        DTO_FIELD(String, element_type);
        DTO_FIELD(Boolean, runtime_dynamic) = false;
    };

    DTO_FIELD(Vector<Object<SocketDto>>, inputs) = { };
    DTO_FIELD(Vector<Object<SocketDto>>, outputs) = { };
    DTO_FIELD(Vector<Object<SocketGroupDto>>, groups) = { };

    DTO_FIELD(Vector<Float32>, color) = { 0.3f, 0.5f, 0.7f, 1.0f };
};

class WEB_SERVER_API NodeTypesDto : public oatpp::DTO {
    DTO_INIT(NodeTypesDto, DTO)

    DTO_FIELD(Vector<Object<NodeTypeDto>>, node_types) = { };
};

class WEB_SERVER_API NodeInstanceDto : public oatpp::DTO {
    DTO_INIT(NodeInstanceDto, DTO)

    DTO_FIELD(String, id);
    DTO_FIELD(String, type);
    // JSON格式的输入值
    // Fields<Any>表示一个String到Any的映射
    DTO_FIELD(Fields<Any>, input_values) = { };
};

class WEB_SERVER_API NodeLinkDto : public oatpp::DTO {
    DTO_INIT(NodeLinkDto, DTO)

    DTO_FIELD(String, id);
    DTO_FIELD(String, from_node);
    DTO_FIELD(String, from_socket);
    DTO_FIELD(String, to_node);
    DTO_FIELD(String, to_socket);
};

class WEB_SERVER_API NodeTreeDto : public oatpp::DTO {
    DTO_INIT(NodeTreeDto, DTO)

    DTO_FIELD(Vector<Object<NodeInstanceDto>>, nodes) = { };
    DTO_FIELD(Vector<Object<NodeLinkDto>>, links) = { };
};

class WEB_SERVER_API ExecutionResultDto : public oatpp::DTO {
    DTO_INIT(ExecutionResultDto, DTO)

    DTO_FIELD(Boolean, success) = false;       // 执行是否成功
    DTO_FIELD(String, error) = "";             // 错误信息（如果有的话）
    DTO_FIELD(Float64, execution_time) = 0.0;  // 执行时间（秒）
    // TODO: 添加执行完成后，各节点输出值、执行状态、颜色等
};

class WEB_SERVER_API ValidationResultDto : public oatpp::DTO {
    DTO_INIT(ValidationResultDto, DTO)

    DTO_FIELD(Boolean, valid) = false;  // 是否有效
    DTO_FIELD(String, error) = "";      // 错误信息（如果有的话）
};

class WEB_SERVER_API MessageDto : public oatpp::DTO {
    DTO_INIT(MessageDto, DTO)

    DTO_FIELD(Int32, code);
    DTO_FIELD(String, message);
    DTO_FIELD(Any, data);
};

USTC_CG_NAMESPACE_CLOSE_SCOPE

#include OATPP_CODEGEN_END(DTO)  //<- End Codegen