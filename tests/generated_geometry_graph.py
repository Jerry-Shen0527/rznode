from ruzino_graph import RuzinoGraph
import os

# Auto-generated Python code from NodeTree
# This script recreates the node graph and executes it

# Create graph
g = RuzinoGraph("GeneratedGraph")
binary_dir = os.getcwd()
config_path = os.path.join(binary_dir, "geometry_nodes.json")
g.loadConfiguration(config_path)

# Create nodes
create_uv_sphere = g.createNode("create_uv_sphere", name="create_uv_sphere")
triangulate = g.createNode("triangulate", name="triangulate")
mesh_decompose = g.createNode("mesh_decompose", name="mesh_decompose")

# Create connections
g.addEdge(create_uv_sphere, "Geometry", triangulate, "Input")
g.addEdge(triangulate, "Ouput", mesh_decompose, "Mesh")

# Set input values and mark outputs
inputs = {
    (create_uv_sphere, "segments"): 32,
    (create_uv_sphere, "rings"): 16,
    (create_uv_sphere, "radius"): 1.000000,
}

# Mark output sockets
g.markOutput(mesh_decompose, "Vertices")
g.markOutput(mesh_decompose, "FaceVertexCounts")
g.markOutput(mesh_decompose, "FaceVertexIndices")
g.markOutput(mesh_decompose, "Normals")
g.markOutput(mesh_decompose, "Texcoords")

# Execute graph
g.prepare_and_execute(inputs)

# Get outputs
mesh_decompose_Vertices = g.getOutput(mesh_decompose, "Vertices")
print(f"mesh_decompose.Vertices = {mesh_decompose_Vertices}")
mesh_decompose_FaceVertexCounts = g.getOutput(mesh_decompose, "FaceVertexCounts")
print(f"mesh_decompose.FaceVertexCounts = {mesh_decompose_FaceVertexCounts}")
mesh_decompose_FaceVertexIndices = g.getOutput(mesh_decompose, "FaceVertexIndices")
print(f"mesh_decompose.FaceVertexIndices = {mesh_decompose_FaceVertexIndices}")
mesh_decompose_Normals = g.getOutput(mesh_decompose, "Normals")
print(f"mesh_decompose.Normals = {mesh_decompose_Normals}")
mesh_decompose_Texcoords = g.getOutput(mesh_decompose, "Texcoords")
print(f"mesh_decompose.Texcoords = {mesh_decompose_Texcoords}")
