#include "nodes/web_server/web_server.hpp"

#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "nodes/core/io/json.hpp"
#include "nodes/system/node_system.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

class WebServerTest : public ::testing::Test {
   protected:
    void SetUp() override
    {
        web_server = create_web_server();
    }

    void TearDown() override
    {
        if (web_server && web_server->is_running()) {
            web_server->stop();
        }
    }

    std::unique_ptr<WebServer> web_server;
};

TEST_F(WebServerTest, BasicInitialization)
{
    ASSERT_TRUE(web_server != nullptr);
    EXPECT_FALSE(web_server->is_running());
    EXPECT_EQ(web_server->get_port(), 8080);  // 默认端口
}

TEST_F(WebServerTest, InitializeWithCustomPort)
{
    const int custom_port = 9000;
    EXPECT_TRUE(web_server->initialize(custom_port));
    EXPECT_EQ(web_server->get_port(), custom_port);
}

TEST_F(WebServerTest, StartStopServer)
{
    web_server->initialize(8081);

    // 创建一个测试用的NodeSystem
    auto node_system = create_dynamic_loading_system();
    node_system->init();

    // 将NodeSystem设置到WebServer
    web_server->set_node_system(node_system);

    // 在后台线程启动服务器
    std::thread server_thread([this]() { web_server->start(); });

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(web_server->is_running());

    // 停止服务器
    web_server->stop();
    EXPECT_FALSE(web_server->is_running());

    server_thread.join();
}

TEST_F(WebServerTest, NodeSystemExecution)
{
    web_server->initialize(8082);

    // 创建一个测试用的NodeSystem，并导入测试节点配置
    auto node_system = create_dynamic_loading_system();
    auto loaded = node_system->load_configuration("test_nodes.json");
    ASSERT_TRUE(loaded);
    node_system->init();

    // 将NodeSystem设置到WebServer
    web_server->set_node_system(node_system);

    // 在后台线程启动服务器
    std::thread server_thread([this]() { web_server->start(); });

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(web_server->is_running());

    // 测试HTTP客户端
    httplib::Client client("127.0.0.1", 8082);
    client.set_connection_timeout(5, 0);  // 5秒超时

    // 1. 测试服务器状态API
    {
        auto response = client.Get("/api/status");
        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 200);

        auto status_json = nlohmann::json::parse(response->body);
        EXPECT_EQ(status_json["status"], "running");
        EXPECT_EQ(status_json["port"], 8082);
        EXPECT_EQ(status_json["has_node_system"], true);

        std::cout << "Server status: " << response->body << std::endl;
    }

    // 2. 测试节点类型查询API
    {
        auto response = client.Get("/api/node-types");
        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 200);

        auto node_types_json = nlohmann::json::parse(response->body);
        EXPECT_TRUE(node_types_json.is_array());
        EXPECT_GT(node_types_json.size(), 0);  // 应该有至少一个节点类型

        std::cout << "Available node types (" << node_types_json.size()
                  << "):" << std::endl;
        for (const auto& node_type : node_types_json) {
            EXPECT_TRUE(node_type.contains("id_name"));
            EXPECT_TRUE(node_type.contains("ui_name"));
            EXPECT_TRUE(node_type.contains("inputs"));
            EXPECT_TRUE(node_type.contains("outputs"));

            std::cout << "  - " << node_type["ui_name"] << " ("
                      << node_type["id_name"] << ")" << std::endl;
        }
    }

    // 3. 测试节点树验证API
    {
        nlohmann::json test_tree = { { "nodes", nlohmann::json::array() },
                                     { "links", nlohmann::json::array() } };

        auto response =
            client.Post("/api/validate", test_tree.dump(), "application/json");
        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 200);

        auto validation_result = nlohmann::json::parse(response->body);
        EXPECT_TRUE(validation_result.contains("valid"));

        std::cout << "Empty tree validation: " << response->body << std::endl;
    }

    // 4. 测试简单节点树执行API（如果test_nodes.json中有合适的节点）
    {
        // 先获取可用的节点类型
        auto node_types_response = client.Get("/api/node-types");
        if (node_types_response && node_types_response->status == 200) {
            auto node_types = nlohmann::json::parse(node_types_response->body);

            if (!node_types.empty()) {
                // 创建一个简单的测试节点树
                nlohmann::json test_execution = {
                    { "nodes",
                      nlohmann::json::array(
                          { { { "id", 1 },
                              { "type", node_types[0]["id_name"] },
                              { "input_values", nlohmann::json::object() },
                              { "position_x", 100.0 },
                              { "position_y", 100.0 } } }) },
                    { "links", nlohmann::json::array() }
                };

                auto response = client.Post(
                    "/api/execute", test_execution.dump(), "application/json");
                ASSERT_TRUE(response);

                auto execution_result = nlohmann::json::parse(response->body);
                EXPECT_TRUE(execution_result.contains("success"));

                std::cout << "Simple execution test: " << response->body
                          << std::endl;
            }
        }
    }

    // 停止服务器
    web_server->stop();
    EXPECT_FALSE(web_server->is_running());

    server_thread.join();
}

USTC_CG_NAMESPACE_CLOSE_SCOPE
