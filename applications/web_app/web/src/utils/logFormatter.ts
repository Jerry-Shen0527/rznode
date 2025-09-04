// 统一的调试日志格式化工具
import chalk from 'chalk'

/**
 * 日志类型
 */
export type LogType = 'DEBUG' | 'INFO' | 'WARNING' | 'ERROR'

/**
 * 获取时间戳字符串 (HH:MM:SS 格式)
 */
const getTimestamp = (): string => {
    const now = new Date()
    return now.toLocaleTimeString('en-GB', {
        hour12: false,
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    })
}

/**
 * 获取带颜色的日志标签
 * @param type - 日志类型
 * @returns 格式化的日志标签字符串
 */
export const logTag = (type: LogType): string => {
    const timestamp = chalk.gray(`[${getTimestamp()}]`)

    let level: string
    switch (type) {
        case 'DEBUG':
            level = chalk.green('[DEBUG]')
            break
        case 'INFO':
            level = chalk.blue('[INFO]')
            break
        case 'WARNING':
            level = chalk.yellow('[WARNING]')
            break
        case 'ERROR':
            level = chalk.red('[ERROR]')
            break
        default:
            level = chalk.blue('[INFO]')
    }

    return `${timestamp} ${level}`
}

/**
 * 使用示例:
 * 
 * import { logTag } from './utils/logFormatter'
 * 
 * // 使用原生console获得完美调用栈信息
 * console.log(logTag('INFO'), '这是一条信息', data)
 * console.warn(logTag('WARNING'), '这是一条警告', data)
 * console.error(logTag('ERROR'), '这是一条错误', data)
 * 
 * // 输出格式:
 * // [14:30:45] [INFO] 这是一条信息 {data}
 * // [14:30:45] [WARNING] 这是一条警告 {data}
 * // [14:30:45] [ERROR] 这是一条错误 {data}
 */
