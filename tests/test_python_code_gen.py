"""
Test Python code generation from NodeTree

This test demonstrates how to generate Python code from a node graph.
"""

import os
from ruzino_graph import RuzinoGraph

# Get binary directory
binary_dir = os.getcwd()


def test_python_code_generation_simple():
    """Test generating Python code from a simple graph."""
    print("\n" + "="*60)
    print("TEST: Python Code Generation - Simple Graph")
    print("="*60)
    
    # Create a simple graph
    g = RuzinoGraph("SimpleTest")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create simple computation: (5 + 3) = 8
    add1 = g.createNode("add", name="add_node")
    g.markOutput(add1, "value")
    
    # Generate Python code from the graph
    python_code = g.to_python_code()
    print("Generated Python code:")
    print("-" * 60)
    print(python_code)
    print("-" * 60)
    
    print("✓ Python code generated successfully")


def test_python_code_generation_complex():
    """Test generating Python code from a complex graph."""
    print("\n" + "="*60)
    print("TEST: Python Code Generation - Complex Graph")
    print("="*60)
    
    g = RuzinoGraph("ComplexTest")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create graph: (1+2) + (3+4) 
    add1 = g.createNode("add", name="left_branch")
    add2 = g.createNode("add", name="right_branch")
    add3 = g.createNode("add", name="combiner")
    
    g.addEdge(add1, "value", add3, "value")
    g.addEdge(add2, "value", add3, "value2")
    g.markOutput(add3, "value")
    
    print("✓ Complex graph created")
    print("  Graph structure: (left_branch) + (right_branch) -> combiner")
    
    # Generate code
    python_code = g.to_python_code()
    print("\nGenerated Python code:")
    print("-" * 60)
    print(python_code)
    print("-" * 60)
    
    # Optionally save to file
    output_file = os.path.join(binary_dir, "generated_complex_graph.py")
    g.save_python_code(output_file)
    print(f"\n✓ Code saved to: {output_file}")


# Example of what the generated code should look like:
EXPECTED_GENERATED_CODE = '''
from ruzino_graph import RuzinoGraph
import os

# Auto-generated Python code from NodeTree
# This script recreates the node graph and executes it

# Create graph
g = RuzinoGraph("GeneratedGraph")
binary_dir = os.getcwd()
config_path = os.path.join(binary_dir, "test_nodes.json")
g.loadConfiguration(config_path)

# Create nodes
add_node = g.createNode("add", name="add_node")

# Create connections
# No connections in this graph

# Set input values and mark outputs
inputs = {
    # (add_node, "value"): value,
    # (add_node, "value2"): value,
}

# Mark output sockets
g.markOutput(add_node, "value")

# Execute graph
g.prepare_and_execute(inputs)

# Get outputs
result_value = g.getOutput(add_node, "value")
print(f"value = {result_value}")
'''


if __name__ == "__main__":
    print("Expected generated code example:")
    print(EXPECTED_GENERATED_CODE)
    print("\nTo enable actual code generation, the C++ to_python_code")
    print("function needs to be exposed through Python bindings.")
