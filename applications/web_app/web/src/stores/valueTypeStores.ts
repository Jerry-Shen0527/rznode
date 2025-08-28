import { defineStore } from "pinia"
import { NodeInterfaceType } from "baklavajs"

export const useValueTypeStore = defineStore('valueType', {
    state: () => ({
        // 存储值类型的状态
        interfaceTypeMap: new Map<string, NodeInterfaceType<any>>(),
    }),
    getters: {
        // 计算属性（如果需要）
    },
    actions: {

    }
})