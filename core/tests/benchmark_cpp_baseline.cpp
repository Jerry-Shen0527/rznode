/**
 * @file benchmark_cpp_baseline.cpp
 * @brief C++ baseline benchmark for node graph execution performance
 * 
 * This benchmark measures the native C++ performance without Python wrapper overhead.
 * Results from this benchmark should be compared with benchmark_wrapper_overhead.py
 * to quantify the Python wrapper overhead.
 * 
 * Measurements include:
 * - Graph construction time
 * - Graph execution time
 * - Full cycle time (construct + execute + read)
 * - Per-operation timing breakdowns
 */

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <iostream>

#include <entt/meta/meta.hpp>
#include "nodes/core/api.hpp"
#include "nodes/core/node.hpp"
#include "nodes/core/node_tree.hpp"
#include "nodes/core/node_exec_eager.hpp"
#include "nodes/core/node_link.hpp"

using namespace USTC_CG;
using namespace std::chrono;

/**
 * @brief Simple benchmark result structure
 */
struct BenchmarkResult {
    std::string name;
    double mean_ms;
    double median_ms;
    double std_dev_ms;
    double min_ms;
    double max_ms;
    int iterations;

    void print() const {
        std::cout << name << ":\n";
        std::cout << "  Mean:    " << std::fixed << std::setprecision(4) << mean_ms << " ms\n";
        std::cout << "  Median:  " << std::fixed << std::setprecision(4) << median_ms << " ms\n";
        std::cout << "  Std Dev: " << std::fixed << std::setprecision(4) << std_dev_ms << " ms\n";
        std::cout << "  Min:     " << std::fixed << std::setprecision(4) << min_ms << " ms\n";
        std::cout << "  Max:     " << std::fixed << std::setprecision(4) << max_ms << " ms\n";
        std::cout << "  Iterations: " << iterations << "\n";
    }
};

/**
 * @brief Benchmark a function with statistics
 */
template<typename Func>
BenchmarkResult benchmark_function(const std::string& name, Func&& func, int iterations = 100) {
    std::vector<double> times;
    times.reserve(iterations);

    // Warmup runs (not counted)
    for (int i = 0; i < 5; ++i) {
        func();
    }

    // Actual benchmark runs
    for (int i = 0; i < iterations; ++i) {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<nanoseconds>(end - start);
        times.push_back(duration.count() / 1e6);  // Convert to milliseconds
    }

    // Calculate statistics
    BenchmarkResult result;
    result.name = name;
    result.iterations = iterations;
    
    result.mean_ms = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    
    std::sort(times.begin(), times.end());
    result.median_ms = times[times.size() / 2];
    result.min_ms = times.front();
    result.max_ms = times.back();
    
    // Standard deviation
    double variance = 0.0;
    for (const auto& t : times) {
        variance += (t - result.mean_ms) * (t - result.mean_ms);
    }
    result.std_dev_ms = std::sqrt(variance / times.size());
    
    return result;
}

/**
 * @brief Test fixture for C++ baseline benchmarks
 */
class CppBaselineBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        // Register types
        register_cpp_type<int>();
        register_cpp_type<float>();
        register_cpp_type<double>();
        register_cpp_type<std::string>();

        // Create descriptor and register add node
        descriptor = std::make_shared<NodeTreeDescriptor>();

        NodeTypeInfo add_node;
        add_node.id_name = "add";
        add_node.ui_name = "Add";
        add_node.ALWAYS_REQUIRED = true;
        add_node.set_declare_function([](NodeDeclarationBuilder& b) {
            b.add_input<int>("value");
            b.add_input<int>("value2").default_val(1);
            b.add_output<int>("value");
        });

        add_node.set_execution_function([](ExeParams params) {
            auto a = params.get_input<int>("value");
            auto b = params.get_input<int>("value2");
            params.set_output("value", a + b);
            return true;
        });

        descriptor->register_node(add_node);

        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "C++ BASELINE BENCHMARK\n";
        std::cout << std::string(60, '=') << "\n";
        std::cout << "Node types registered: add\n";
    }

    void TearDown() override {
        entt::meta_reset();
    }

    std::shared_ptr<NodeTreeDescriptor> descriptor;
};

/**
 * @brief Create a simple linear graph: node0 -> node1 -> node2
 */
std::unique_ptr<NodeTree> create_simple_graph(const std::shared_ptr<NodeTreeDescriptor>& desc) {
    auto tree = create_node_tree(desc);
    
    auto node0 = tree->add_node("add");
    auto node1 = tree->add_node("add");
    auto node2 = tree->add_node("add");
    
    tree->add_link(
        node0->get_output_socket("value"),
        node1->get_input_socket("value"));
    tree->add_link(
        node1->get_output_socket("value"),
        node2->get_input_socket("value"));
    
    return tree;
}

/**
 * @brief Create a complex linear chain graph
 */
std::unique_ptr<NodeTree> create_complex_graph(
    const std::shared_ptr<NodeTreeDescriptor>& desc, 
    int chain_length) 
{
    auto tree = create_node_tree(desc);
    
    std::vector<Node*> nodes;
    for (int i = 0; i < chain_length; ++i) {
        nodes.push_back(tree->add_node("add"));
    }
    
    for (int i = 0; i < chain_length - 1; ++i) {
        tree->add_link(
            nodes[i]->get_output_socket("value"),
            nodes[i + 1]->get_input_socket("value"));
    }
    
    return tree;
}

// ============================================================================
// BENCHMARK TESTS
// ============================================================================

TEST_F(CppBaselineBenchmark, GraphCreation_Simple) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking simple graph creation (3 nodes)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto result = benchmark_function("Graph Creation (simple)", [&]() {
        auto tree = create_simple_graph(descriptor);
        return tree;
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, GraphCreation_Medium) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking medium graph creation (20 nodes)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto result = benchmark_function("Graph Creation (medium)", [&]() {
        auto tree = create_complex_graph(descriptor, 20);
        return tree;
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, GraphCreation_Large) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking large graph creation (50 nodes)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto result = benchmark_function("Graph Creation (large)", [&]() {
        auto tree = create_complex_graph(descriptor, 50);
        return tree;
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, GraphExecution_Simple) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking simple graph execution...\n";
    std::cout << std::string(60, '=') << "\n";

    auto tree = create_simple_graph(descriptor);
    NodeTreeExecutorDesc exec_desc;
    exec_desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(exec_desc);

    auto& nodes = tree->nodes;
    
    auto result = benchmark_function("Graph Execution (simple)", [&]() {
        executor->prepare_tree(tree.get());
        
        // Set inputs
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value"), 1);
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value2"), 2);
        executor->sync_node_from_external_storage(nodes[1]->get_input_socket("value2"), 3);
        executor->sync_node_from_external_storage(nodes[2]->get_input_socket("value2"), 4);
        
        // Execute
        executor->execute_tree(tree.get());
        
        // Read output
        entt::meta_any output;
        executor->sync_node_to_external_storage(nodes[2]->get_output_socket("value"), output);
        return output.cast<int>();
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, GraphExecution_Medium) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking medium graph execution (20 nodes)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto tree = create_complex_graph(descriptor, 20);
    NodeTreeExecutorDesc exec_desc;
    exec_desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(exec_desc);

    auto& nodes = tree->nodes;
    
    auto result = benchmark_function("Graph Execution (medium)", [&]() {
        executor->prepare_tree(tree.get());
        
        // Set inputs
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value"), 1);
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value2"), 2);
        for (size_t i = 1; i < nodes.size(); ++i) {
            executor->sync_node_from_external_storage(
                nodes[i]->get_input_socket("value2"), static_cast<int>(i) + 1);
        }
        
        // Execute
        executor->execute_tree(tree.get());
        
        // Read output
        entt::meta_any output;
        executor->sync_node_to_external_storage(nodes.back()->get_output_socket("value"), output);
        return output.cast<int>();
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, GraphExecution_Large) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking large graph execution (50 nodes)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto tree = create_complex_graph(descriptor, 50);
    NodeTreeExecutorDesc exec_desc;
    exec_desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(exec_desc);

    auto& nodes = tree->nodes;
    
    auto result = benchmark_function("Graph Execution (large)", [&]() {
        executor->prepare_tree(tree.get());
        
        // Set inputs
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value"), 1);
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value2"), 2);
        for (size_t i = 1; i < nodes.size(); ++i) {
            executor->sync_node_from_external_storage(
                nodes[i]->get_input_socket("value2"), static_cast<int>(i) + 1);
        }
        
        // Execute
        executor->execute_tree(tree.get());
        
        // Read output
        entt::meta_any output;
        executor->sync_node_to_external_storage(nodes.back()->get_output_socket("value"), output);
        return output.cast<int>();
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, FullCycle_Simple) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking simple full cycle (create + execute + read)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto result = benchmark_function("Full Cycle (simple)", [&]() {
        // Create graph
        auto tree = create_simple_graph(descriptor);
        
        // Create executor
        NodeTreeExecutorDesc exec_desc;
        exec_desc.policy = NodeTreeExecutorDesc::Policy::Eager;
        auto executor = create_node_tree_executor(exec_desc);
        
        auto& nodes = tree->nodes;
        
        // Prepare and set inputs
        executor->prepare_tree(tree.get());
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value"), 1);
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value2"), 2);
        executor->sync_node_from_external_storage(nodes[1]->get_input_socket("value2"), 3);
        executor->sync_node_from_external_storage(nodes[2]->get_input_socket("value2"), 4);
        
        // Execute
        executor->execute_tree(tree.get());
        
        // Read output
        entt::meta_any output;
        executor->sync_node_to_external_storage(nodes[2]->get_output_socket("value"), output);
        return output.cast<int>();
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, FullCycle_Medium) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking medium full cycle (create + execute + read)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto result = benchmark_function("Full Cycle (medium)", [&]() {
        auto tree = create_complex_graph(descriptor, 20);
        
        NodeTreeExecutorDesc exec_desc;
        exec_desc.policy = NodeTreeExecutorDesc::Policy::Eager;
        auto executor = create_node_tree_executor(exec_desc);
        
        auto& nodes = tree->nodes;
        
        executor->prepare_tree(tree.get());
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value"), 1);
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value2"), 2);
        for (size_t i = 1; i < nodes.size(); ++i) {
            executor->sync_node_from_external_storage(
                nodes[i]->get_input_socket("value2"), static_cast<int>(i) + 1);
        }
        
        executor->execute_tree(tree.get());
        
        entt::meta_any output;
        executor->sync_node_to_external_storage(nodes.back()->get_output_socket("value"), output);
        return output.cast<int>();
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, FullCycle_Large) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking large full cycle (create + execute + read)...\n";
    std::cout << std::string(60, '=') << "\n";

    auto result = benchmark_function("Full Cycle (large)", [&]() {
        auto tree = create_complex_graph(descriptor, 50);
        
        NodeTreeExecutorDesc exec_desc;
        exec_desc.policy = NodeTreeExecutorDesc::Policy::Eager;
        auto executor = create_node_tree_executor(exec_desc);
        
        auto& nodes = tree->nodes;
        
        executor->prepare_tree(tree.get());
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value"), 1);
        executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value2"), 2);
        for (size_t i = 1; i < nodes.size(); ++i) {
            executor->sync_node_from_external_storage(
                nodes[i]->get_input_socket("value2"), static_cast<int>(i) + 1);
        }
        
        executor->execute_tree(tree.get());
        
        entt::meta_any output;
        executor->sync_node_to_external_storage(nodes.back()->get_output_socket("value"), output);
        return output.cast<int>();
    }, 100);
    
    result.print();
}

TEST_F(CppBaselineBenchmark, OperationBreakdown) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Benchmarking individual C++ operations...\n";
    std::cout << std::string(60, '=') << "\n";

    auto tree = create_simple_graph(descriptor);
    auto& nodes = tree->nodes;
    
    NodeTreeExecutorDesc exec_desc;
    exec_desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(exec_desc);
    
    // 1. Socket access
    auto result = benchmark_function("Socket Access", [&]() {
        auto socket = nodes[0]->get_input_socket("value");
        return socket;
    }, 1000);
    result.print();
    
    // 2. meta_any conversion
    result = benchmark_function("meta_any Creation", [&]() {
        entt::meta_any value = 42;
        return value;
    }, 1000);
    result.print();
    
    // 3. Input setting
    auto socket = nodes[0]->get_input_socket("value");
    executor->prepare_tree(tree.get());
    
    result = benchmark_function("Set Input (sync_node_from_external_storage)", [&]() {
        executor->sync_node_from_external_storage(socket, 42);
    }, 1000);
    result.print();
    
    // 4. Output reading
    executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value"), 1);
    executor->sync_node_from_external_storage(nodes[0]->get_input_socket("value2"), 2);
    executor->sync_node_from_external_storage(nodes[1]->get_input_socket("value2"), 3);
    executor->sync_node_from_external_storage(nodes[2]->get_input_socket("value2"), 4);
    executor->execute_tree(tree.get());
    
    auto output_socket = nodes[2]->get_output_socket("value");
    result = benchmark_function("Read Output (sync_node_to_external_storage)", [&]() {
        entt::meta_any output;
        executor->sync_node_to_external_storage(output_socket, output);
        return output.cast<int>();
    }, 1000);
    result.print();
}

TEST_F(CppBaselineBenchmark, Summary) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "C++ BASELINE BENCHMARK COMPLETE\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "\nCompare these results with benchmark_wrapper_overhead.py to calculate\n";
    std::cout << "the Python wrapper overhead.\n\n";
    std::cout << "Expected overhead calculation:\n";
    std::cout << "  Overhead% = ((Python_Time - CPP_Time) / CPP_Time) * 100\n";
    std::cout << "\n";
}
