"""Quick test to check if get_loaded_configs works"""
import os
import pytest

binary_dir = os.getcwd()

def test_get_loaded_configs():
    import nodes_system_py as system
    
    s = system.create_dynamic_loading_system()
    s.init()
    config_path = os.path.join(binary_dir, 'geometry_nodes.json')
    
    try:
        s.load_configuration(config_path)
    except RuntimeError as e:
        if "GPU_sph.dll" in str(e):
            pytest.skip(f"GPU_sph.dll not available: {e}")
        raise
    
    configs = s.get_loaded_configs()
    print(f"Loaded configs: {configs}")
    print(f"Type: {type(configs)}")
    assert 'geometry_nodes.json' in configs
    print("✓ get_loaded_configs works!")
