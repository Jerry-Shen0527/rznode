import './styles/global.css'
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import App from './App.vue'

// 创建 Pinia 实例
const pinia = createPinia()

// 创建Vue应用
const app = createApp(App)

// 使用 Pinia
app.use(pinia)

// 使用路由
app.use(router)

// 挂载应用
app.mount('#app')
