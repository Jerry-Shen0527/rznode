import { defineStore } from 'pinia'
import { IEditorState, Editor } from 'baklavajs'

export const useGlobalStore = defineStore('global', {
    state: () => ({
        // Baklava.js 编辑器状态
        editorState: {} as IEditorState,
        // 页面状态
        isConnected: false,
        connectionStatus: '未连接',
        isExecuting: false,
        lastExecutionResult: null as any
    }),
    getters: {
        // 计算属性（如果需要）
    },
    actions: {
        // 存储 Baklava.js 编辑器状态
        saveEditorState(editor: Editor) {
            this.editorState = editor.save()
        },
        // 恢复 Baklava.js 编辑器状态
        loadEditorState(editor: Editor) {
            if (this.editorState && Object.keys(this.editorState).length > 0) {
                editor.load(this.editorState)
            }
        }
    }
})