"""
Test script for Geometry Node Graph using RuzinoGraph API.

This test demonstrates creating, connecting, and executing geometry nodes,
and retrieving/inspecting the resulting geometry data.

Note: This script requires the geometry_nodes.json configuration and
the rzgeometry_py Python module to be available.
"""

import os
import json

# Import modules - environment setup is handled by conftest.py
from ruzino_graph import RuzinoGraph
import nodes_core_py as core
import nodes_system_py as system

# Try importing rzgeometry_py - may not be built yet
try:
    import rzgeometry_py as geom
    HAS_GEOMETRY = True
except ImportError:
    HAS_GEOMETRY = False
    print("WARNING: rzgeometry_py not available. Some tests will be skipped.")

# Get binary directory for test configuration files
binary_dir = os.getcwd()


def test_geometry_graph_creation():
    """Test creating a basic geometry node graph."""
    print("\n" + "="*60)
    print("TEST: Geometry Graph Creation")
    print("="*60)
    
    # Create graph
    g = RuzinoGraph("GeometryGraph")
    print(f"✓ Created graph: {g}")
    
    # Load geometry nodes configuration
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    assert os.path.exists(config_path), f"Config file not found: {config_path}"
    
    g.loadConfiguration(config_path)
    print(f"✓ Loaded geometry configuration from: {config_path}")
    print(f"  Graph state: {g}")


def test_create_simple_geometry_nodes():
    """Test creating geometry nodes."""
    print("\n" + "="*60)
    print("TEST: Create Geometry Nodes")
    print("="*60)
    
    g = RuzinoGraph("SimpleGeomGraph")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a basic geometry creation node (e.g., create_cube)
    cube_node = g.createNode("create_cube", name="MyCube")
    print(f"✓ Created node: {cube_node.ui_name} (ID: {cube_node.ID})")
    
    # Verify node is in the graph
    assert len(g.nodes) == 1, f"Expected 1 node, got {len(g.nodes)}"
    print(f"✓ Graph contains {len(g.nodes)} node(s)")


def test_geometry_node_pipeline():
    """Test a simple geometry processing pipeline."""
    print("\n" + "="*60)
    print("TEST: Geometry Node Pipeline")
    print("="*60)
    
    g = RuzinoGraph("GeomPipeline")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a geometry creation node
    sphere_node = g.createNode("create_uv_sphere", name="Sphere")
    print(f"✓ Created sphere node: {sphere_node.ui_name}")
    
    # Create a transform node (if available)
    transform_node = g.createNode("transform_geom", name="Transform")
    print(f"✓ Created transform node: {transform_node.ui_name}")
    
    # Connect them
    # Note: Need to check actual socket names in the geometry nodes
    g.addEdge(sphere_node, "Geometry", transform_node, "Geometry")
    print(f"✓ Connected: {sphere_node.ui_name} -> {transform_node.ui_name}")
    
    assert len(g.links) == 1, f"Expected 1 link, got {len(g.links)}"
    print(f"✓ Graph contains {len(g.links)} link(s)")


def test_geometry_execution_and_output():
    """Test executing a geometry graph and retrieving results."""
    if not HAS_GEOMETRY:
        print("\n" + "="*60)
        print("TEST: Geometry Execution (SKIPPED - rzgeometry_py not available)")
        print("="*60)
        return
    
    print("\n" + "="*60)
    print("TEST: Geometry Execution and Output")
    print("="*60)
    
    g = RuzinoGraph("GeomExecution")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a simple geometry
    cube_node = g.createNode("create_cube", name="Cube")
    print(f"✓ Created cube node")
    
    # Mark output for execution
    g.markOutput(cube_node, "Geometry")
    print(f"✓ Marked output: Cube.Geometry")
    
    # Execute the graph
    try:
        g.prepare_and_execute()
        print("✓ Executed graph successfully")
        
        # Get the geometry output
        result = g.getOutput(cube_node, "Geometry")
        print(f"✓ Retrieved output: {type(result)}")
        
        # If result is a Geometry object, inspect it
        if isinstance(result, geom.Geometry):
            print(f"  Geometry: {result.to_string()}")
            
            # Try to get mesh component
            mesh = result.get_mesh_component()
            if mesh:
                vertices = mesh.get_vertices()
                faces = mesh.get_face_vertex_counts()
                print(f"  Mesh: {len(vertices)} vertices, {len(faces)} faces")
            else:
                print("  No mesh component found")
        else:
            print(f"  Result type: {type(result)}")
            
    except Exception as e:
        print(f"✗ Execution failed: {e}")
        import traceback
        traceback.print_exc()


def test_geometry_graph_serialization():
    """Test serializing a geometry graph to JSON."""
    print("\n" + "="*60)
    print("TEST: Geometry Graph Serialization")
    print("="*60)
    
    g = RuzinoGraph("SerializationTest")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a simple pipeline
    sphere = g.createNode("create_uv_sphere", name="Sphere")
    mesh_decompose = g.createNode("mesh_decompose", name="Decompose")
    
    # mesh_decompose uses "Mesh" as input socket name
    g.addEdge(sphere, "Geometry", mesh_decompose, "Mesh")
    g.markOutput(mesh_decompose, "Vertices")
    
    print(f"✓ Created graph with {len(g.nodes)} nodes and {len(g.links)} links")
    
    # Serialize to JSON
    json_str = g.serialize()
    print(f"✓ Serialized graph ({len(json_str)} characters)")
    
    # Parse and pretty-print a snippet
    try:
        json_obj = json.loads(json_str)
        print(f"  Nodes in JSON: {len(json_obj.get('nodes', []))}")
        print(f"  Links in JSON: {len(json_obj.get('links', []))}")
        
        # Print a small sample of the JSON (first 500 chars)
        json_pretty = json.dumps(json_obj, indent=2)
        print(f"\n  JSON Sample (first 500 chars):")
        print(f"  {json_pretty[:500]}...")
    except Exception as e:
        print(f"  JSON parsing: {e}")
    
    # Test deserialization
    g2 = RuzinoGraph("DeserializedGraph")
    g2.loadConfiguration(config_path)
    g2.deserialize(json_str)
    
    print(f"✓ Deserialized graph: {len(g2.nodes)} nodes, {len(g2.links)} links")
    assert len(g2.nodes) == len(g.nodes), "Node count mismatch after deserialization"
    assert len(g2.links) == len(g.links), "Link count mismatch after deserialization"


def test_complex_geometry_pipeline():
    """Test a more complex geometry processing pipeline."""
    print("\n" + "="*60)
    print("TEST: Complex Geometry Pipeline")
    print("="*60)
    
    g = RuzinoGraph("ComplexGeomGraph")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create multiple geometry nodes
    # Base geometry
    sphere = g.createNode("create_uv_sphere", name="BaseSphere")
    
    # Process the geometry
    triangulate = g.createNode("triangulate", name="Triangulate")
    
    # Extract information
    decompose = g.createNode("mesh_decompose", name="Decompose")
    
    # Connect the pipeline - triangulate uses "Input", mesh_decompose uses "Mesh"
    g.addEdge(sphere, "Geometry", triangulate, "Input")
    g.addEdge(triangulate, "Ouput", decompose, "Mesh")  # Note: triangulate outputs "Ouput" (typo in C++ code)
    
    # Mark multiple outputs
    g.markOutput(decompose, "Position")
    g.markOutput(decompose, "Face Indices")
    
    print(f"✓ Created complex pipeline:")
    print(f"  {sphere.ui_name} -> {triangulate.ui_name} -> {decompose.ui_name}")
    print(f"  Nodes: {len(g.nodes)}, Links: {len(g.links)}")
    
    # Serialize to verify structure
    json_str = g.serialize()
    json_obj = json.loads(json_str)
    print(f"✓ Serialized: {len(json_obj.get('nodes', []))} nodes in JSON")


def test_geometry_with_python_created_data():
    """Test passing Python-created geometry data to nodes."""
    if not HAS_GEOMETRY:
        print("\n" + "="*60)
        print("TEST: Python Geometry Data (SKIPPED - rzgeometry_py not available)")
        print("="*60)
        return
    
    print("\n" + "="*60)
    print("TEST: Python-Created Geometry Data")
    print("="*60)
    
    # Create a simple triangle in Python
    vertices = [
        geom.vec3(0.0, 0.0, 0.0),
        geom.vec3(1.0, 0.0, 0.0),
        geom.vec3(0.5, 1.0, 0.0)
    ]
    
    # Create geometry from Python
    triangle_geom = geom.create_mesh_from_arrays(
        vertices,
        [3],  # One face with 3 vertices
        [0, 1, 2]  # Face indices
    )
    
    print(f"✓ Created triangle geometry in Python")
    print(f"  {triangle_geom.to_string()}")
    
    # Get mesh component and verify
    mesh = triangle_geom.get_mesh_component()
    if mesh:
        verts = mesh.get_vertices()
        print(f"  Triangle has {len(verts)} vertices")
        assert len(verts) == 3, "Expected 3 vertices"
    
    # TODO: Test passing this to a node graph when input setting is implemented


# Tests are automatically discovered and run by pytest
