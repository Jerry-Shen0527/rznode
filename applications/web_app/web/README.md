# RzNode Web应用程序 - 几何可视化架构

## 项目概述

这是RzNode项目的Web前端，包含节点编辑器和几何可视化器两个主要功能页面。采用Vue 3 + TypeScript + Pinia的现代前端架构。

## 文件结构设计

```
src/
├── api/                          # API层
│   ├── rznode-api.ts            # 现有的RESTful API客户端
│   └── geometry-websocket.ts    # 新增：几何可视化WebSocket客户端
├── config/                      # 配置层
│   └── index.ts                 # 新增：应用配置文件
├── pages/                       # 页面层
│   ├── NodeEditor.vue           # 现有：节点编辑器页面(重命名自App.vue)
│   └── GeometryVisualizer.vue   # 新增：几何可视化器页面
├── router/                      # 路由层
│   └── index.ts                 # 新增：Vue Router配置
├── stores/                      # 状态管理层
│   ├── globalStores.ts          # 现有：全局状态
│   ├── valueTypeStores.ts       # 现有：值类型状态
│   └── geometryStore.ts         # 新增：几何可视化状态
├── styles/                      # 样式层
│   └── global.css               # 现有：全局样式
├── utils/                       # 工具层
│   ├── logFormatter.ts          # 现有：日志格式化
│   ├── nodeConverter.ts         # 现有：节点转换器
│   ├── nodeTreeSerializer.ts    # 现有：节点树序列化
│   ├── valueTypeRegistrant.ts   # 现有：值类型注册器
│   └── threeJSRenderer.ts       # 新增：Three.js渲染器类
├── App.vue                      # 新增：应用根组件(临时导航)
├── main.ts                      # 更新：应用入口
└── vite-env.d.ts               # 现有：Vite类型定义
```

## 架构特点

### 1. 双页面架构
- **节点编辑器页面**: 基于Baklava.js的节点编辑界面，使用RESTful API
- **几何可视化器页面**: 基于Three.js的3D可视化界面，使用WebSocket通信

### 2. 混合通信模式
- **RESTful API**: 节点编辑器的数据交互(现有)
- **WebSocket**: 几何可视化器的实时数据流(新增)

### 3. 状态管理分离
- **globalStores**: 节点编辑器全局状态
- **valueTypeStores**: 值类型管理状态  
- **geometryStore**: 几何可视化专用状态

### 4. 组件化设计
- **ThreeJSRenderer**: 封装Three.js渲染逻辑
- **页面组件**: 专注于布局和用户交互

## 技术栈

### 现有技术栈
- **Vue 3**: 响应式框架
- **TypeScript**: 类型安全
- **Pinia**: 状态管理
- **Baklava.js**: 节点编辑器

### 新增技术栈
- **Vue Router**: 页面路由管理
- **Three.js**: 3D渲染引擎
- **WebSocket**: 实时通信

## 开发阶段

### 当前阶段（Phase 1）
- ✅ 文件结构设计完成
- ✅ 基础组件和状态管理就位
- ✅ WebSocket客户端实现
- ⏳ 需要安装依赖项: `npm install vue-router three @types/three`
- ⏳ Three.js渲染器需要启用
- ⏳ WebSocket服务端支持需要实现

### 下一阶段（Phase 2）
- [ ] 完整的Three.js集成
- [ ] WebSocket服务端实现
- [ ] 用户交互功能
- [ ] 性能优化

## 使用说明

### 1. 安装依赖
```bash
cd applications/web_app/web
npm install vue-router three @types/three
```

### 2. 启动开发服务器
```bash
npm run dev
```

### 3. 构建生产版本
```bash
npm run build
```

## API接口

### RESTful API (现有)
- `GET /api/node-types` - 获取节点类型
- `GET /api/value-types` - 获取值类型
- `POST /api/node-tree` - 节点树操作
- `POST /api/execute` - 执行节点树
- `POST /api/validate` - 验证节点树

### WebSocket API (新增)
```typescript
// 几何数据更新消息
interface GeometryMessage {
  type: 'geometry_update' | 'geometry_clear' | 'scene_update'
  geometries: GeometryData[]
  scene_id: string
  timestamp: number
}

// 用户交互消息
interface VisualizerMessage {
  type: 'user_interaction' | 'geometry_selection' | 'camera_change'
  data: any
  target_node?: string
}
```

## 配置选项

配置文件位于 `src/config/index.ts`，包含：
- WebSocket连接配置
- REST API配置
- 3D渲染配置
- 相机和材质默认设置
- UI主题配置

## 注意事项

1. **依赖项**: 需要安装vue-router和three.js才能完整运行
2. **临时导航**: 当前使用简单的按钮切换页面，待vue-router安装后启用
3. **Three.js集成**: ThreeJSRenderer类已准备就绪，但在页面中暂时注释
4. **WebSocket服务端**: 需要后端实现几何数据的WebSocket推送
5. **性能考虑**: 大型几何数据需要考虑分块传输和渲染优化

## 未来扩展

1. **全WebSocket架构**: 将节点编辑器也迁移到WebSocket通信
2. **高级交互**: 实现几何对象的选择、变换、测量等功能
3. **材质系统**: 支持复杂材质和纹理
4. **动画系统**: 支持几何体动画和相机动画
5. **协作功能**: 多用户实时协作编辑
