import { createRouter, createWebHistory } from 'vue-router'
import NodeEditor from '../pages/NodeEditor.vue'
import GeometryVisualizer from '../pages/GeometryVisualizer.vue'

const routes = [
    {
        path: '/',
        redirect: '/node-editor'
    },
    {
        path: '/node-editor',
        name: 'NodeEditor',
        component: NodeEditor,
        meta: {
            title: '节点编辑器'
        }
    },
    {
        path: '/geometry-visualizer',
        name: 'GeometryVisualizer',
        component: GeometryVisualizer,
        meta: {
            title: '几何可视化器'
        }
    }
]

const router = createRouter({
    history: createWebHistory(),
    routes
})

// 路由守卫，用于设置页面标题
router.beforeEach((to, from, next) => {
    if (to.meta?.title) {
        document.title = `RzNode - ${to.meta.title}`
    }
    next()
})

export default router
