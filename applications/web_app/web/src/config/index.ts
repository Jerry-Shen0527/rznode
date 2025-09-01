// 开发环境配置
export const config = {
    // WebSocket服务器配置
    websocket: {
        // 几何可视化WebSocket URL
        geometryUrl: 'ws://localhost:8080/geometry',
        // 重连配置
        reconnect: {
            maxAttempts: 5,
            interval: 1000,
            backoff: true
        }
    },

    // REST API配置
    api: {
        baseUrl: 'http://localhost:8080',
        timeout: 5000,
        endpoints: {
            nodeTypes: '/api/node-types',
            valueTypes: '/api/value-types',
            nodeTree: '/api/node-tree',
            execute: '/api/execute',
            validate: '/api/validate'
        }
    },

    // 3D渲染配置
    renderer: {
        antialias: true,
        pixelRatio: window.devicePixelRatio,
        shadows: true,
        shadowMapSize: 2048
    },

    // 默认相机配置
    camera: {
        fov: 75,
        near: 0.1,
        far: 1000,
        position: { x: 5, y: 5, z: 5 },
        target: { x: 0, y: 0, z: 0 }
    },

    // 默认材质配置
    materials: {
        default: {
            color: 0x888888,
            metalness: 0.1,
            roughness: 0.7
        },
        selected: {
            color: 0xff6b35,
            metalness: 0.1,
            roughness: 0.7
        },
        wireframe: {
            color: 0x00ff00
        }
    },

    // 光源配置
    lights: {
        ambient: {
            color: 0x404040,
            intensity: 0.6
        },
        directional: {
            color: 0xffffff,
            intensity: 0.8,
            position: { x: 5, y: 10, z: 5 }
        }
    },

    // UI配置
    ui: {
        theme: 'dark',
        colors: {
            primary: '#0d6efd',
            secondary: '#6c757d',
            success: '#28a745',
            danger: '#dc3545',
            warning: '#ffc107',
            info: '#17a2b8',
            background: '#1e1e1e',
            surface: '#2a2a2a',
            border: '#3d3d3d'
        }
    }
}
