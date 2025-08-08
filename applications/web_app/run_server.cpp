#include <spdlog/spdlog.h>

#include <csignal>
#include <iostream>
#include <memory>

#include "nodes/system/node_system_dl.hpp"
#include "nodes/web_server/web_server.hpp"


// 全局变量用于信号处理（放在全局命名空间）
std::unique_ptr<USTC_CG::WebServer> g_web_server;

// 信号处理函数，用于优雅地关闭服务器
void signal_handler(int signal)
{
    std::cout << "\n收到停止信号 (" << signal << ")，正在关闭服务器..."
              << std::endl;
    if (g_web_server && g_web_server->is_running()) {
        g_web_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[])
{
    // 设置日志级别
    spdlog::set_level(spdlog::level::info);

    std::cout << "========================================" << std::endl;
    std::cout << "    RzNode Web 服务器启动程序" << std::endl;
    std::cout << "========================================" << std::endl;

    // 解析命令行参数
    int port = 8080;  // 默认端口
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
            if (port < 1024 || port > 65535) {
                std::cerr << "错误：端口号必须在 1024-65535 范围内"
                          << std::endl;
                return 1;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "错误：无效的端口号 '" << argv[1] << "'" << std::endl;
            std::cerr << "用法: " << argv[0] << " [端口号]" << std::endl;
            return 1;
        }
    }

    try {
        // 创建web服务器
        std::cout << "正在初始化 Web 服务器..." << std::endl;
        g_web_server = USTC_CG::create_web_server();

        // 初始化服务器
        if (!g_web_server->initialize(port)) {
            std::cerr << "错误：服务器初始化失败" << std::endl;
            return 1;
        }

        // 创建并设置节点系统
        std::cout << "正在加载节点系统..." << std::endl;
        auto node_system =
            std::make_shared<USTC_CG::NodeDynamicLoadingSystem>();

        // 尝试加载节点配置
        try {
            node_system->load_configuration("test_nodes.json");
            std::cout << "节点配置加载成功" << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "警告：无法加载节点配置 (test_nodes.json): "
                      << e.what() << std::endl;
            std::cout << "服务器将以基本模式运行" << std::endl;
        }

        g_web_server->set_node_system(node_system);

        // 设置信号处理器
        std::signal(SIGINT, signal_handler);   // Ctrl+C
        std::signal(SIGTERM, signal_handler);  // 终止信号

        // 显示启动信息
        std::cout << std::endl;
        std::cout << "服务器配置：" << std::endl;
        std::cout << "  端口: " << port << std::endl;
        std::cout << "  前端界面: http://localhost:" << port << std::endl;
        std::cout << "  API状态: http://localhost:" << port << "/api/status"
                  << std::endl;
        std::cout << "  节点类型: http://localhost:" << port
                  << "/api/node-types" << std::endl;
        std::cout << std::endl;
        std::cout << "按 Ctrl+C 停止服务器" << std::endl;
        std::cout << "========================================" << std::endl;

        // 启动服务器（这是阻塞调用）
        std::cout << "正在启动服务器..." << std::endl;
        g_web_server->start();
    }
    catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }

    std::cout << "服务器已关闭" << std::endl;
    return 0;
}
