import * as THREE from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js'
import type { GeometryData } from '../api/geometry-websocket'

export class ThreeJSRenderer {
    private scene!: THREE.Scene
    private camera!: THREE.PerspectiveCamera | THREE.OrthographicCamera
    private renderer!: THREE.WebGLRenderer
    private controls!: OrbitControls
    private container: HTMLElement

    private geometryObjects: Map<string, THREE.Object3D> = new Map()
    private selectedObjects: Set<string> = new Set()

    // 材质
    private defaultMaterial!: THREE.MeshStandardMaterial
    private selectedMaterial!: THREE.MeshStandardMaterial
    private wireframeMaterial!: THREE.MeshBasicMaterial

    // 光源
    private ambientLight!: THREE.AmbientLight
    private directionalLight!: THREE.DirectionalLight

    constructor(container: HTMLElement) {
        this.container = container
        this.initScene()
        this.initCamera()
        this.initRenderer()
        this.initControls()
        this.initLights()
        this.initMaterials()

        // 开始渲染循环
        this.animate()

        // 监听窗口大小变化
        window.addEventListener('resize', this.onWindowResize.bind(this))
    }

    private initScene(): void {
        this.scene = new THREE.Scene()
        this.scene.background = new THREE.Color(0x2a2a2a)

        // 添加网格
        const gridHelper = new THREE.GridHelper(20, 20, 0x444444, 0x444444)
        this.scene.add(gridHelper)

        // 添加坐标轴
        const axesHelper = new THREE.AxesHelper(5)
        this.scene.add(axesHelper)
    }

    private initCamera(): void {
        const aspect = this.container.clientWidth / this.container.clientHeight
        this.camera = new THREE.PerspectiveCamera(75, aspect, 0.1, 1000)
        this.camera.position.set(5, 5, 5)
        this.camera.lookAt(0, 0, 0)
    }

    private initRenderer(): void {
        this.renderer = new THREE.WebGLRenderer({ antialias: true })
        this.renderer.setSize(this.container.clientWidth, this.container.clientHeight)
        this.renderer.setPixelRatio(window.devicePixelRatio)
        this.renderer.shadowMap.enabled = true
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap
        this.container.appendChild(this.renderer.domElement)
    }

    private initControls(): void {
        this.controls = new OrbitControls(this.camera, this.renderer.domElement)
        this.controls.enableDamping = true
        this.controls.dampingFactor = 0.05
        this.controls.screenSpacePanning = false
        this.controls.maxPolarAngle = Math.PI

        // 监听相机变化
        this.controls.addEventListener('change', () => {
            // 可以在这里发送相机变化事件
        })
    }

    private initLights(): void {
        // 环境光
        this.ambientLight = new THREE.AmbientLight(0x404040, 0.6)
        this.scene.add(this.ambientLight)

        // 方向光
        this.directionalLight = new THREE.DirectionalLight(0xffffff, 0.8)
        this.directionalLight.position.set(5, 10, 5)
        this.directionalLight.castShadow = true
        this.directionalLight.shadow.mapSize.width = 2048
        this.directionalLight.shadow.mapSize.height = 2048
        this.scene.add(this.directionalLight)
    }

    private initMaterials(): void {
        this.defaultMaterial = new THREE.MeshStandardMaterial({
            color: 0x888888,
            metalness: 0.1,
            roughness: 0.7
        })

        this.selectedMaterial = new THREE.MeshStandardMaterial({
            color: 0xff6b35,
            metalness: 0.1,
            roughness: 0.7
        })

        this.wireframeMaterial = new THREE.MeshBasicMaterial({
            color: 0x00ff00,
            wireframe: true
        })
    }

    private animate(): void {
        requestAnimationFrame(this.animate.bind(this))
        this.controls.update()
        this.renderer.render(this.scene, this.camera)
    }

    private onWindowResize(): void {
        const width = this.container.clientWidth
        const height = this.container.clientHeight

        if (this.camera instanceof THREE.PerspectiveCamera) {
            this.camera.aspect = width / height
        } else if (this.camera instanceof THREE.OrthographicCamera) {
            const aspect = width / height
            this.camera.left = -10 * aspect
            this.camera.right = 10 * aspect
            this.camera.top = 10
            this.camera.bottom = -10
        }

        this.camera.updateProjectionMatrix()
        this.renderer.setSize(width, height)
    }

    // 几何数据更新方法
    updateGeometries(geometries: GeometryData[]): void {
        // 清除现有几何体
        this.clearGeometries()

        // 添加新几何体
        geometries.forEach(geomData => {
            const object = this.createGeometryObject(geomData)
            if (object) {
                this.scene.add(object)
                this.geometryObjects.set(geomData.id, object)
            }
        })
    }

    private createGeometryObject(geomData: GeometryData): THREE.Object3D | null {
        let geometry: THREE.BufferGeometry | null = null

        switch (geomData.type) {
            case 'mesh':
                geometry = this.createMeshGeometry(geomData)
                break
            case 'line':
                geometry = this.createLineGeometry(geomData)
                break
            case 'point':
                geometry = this.createPointGeometry(geomData)
                break
            default:
                console.warn('未知的几何类型:', geomData.type)
                return null
        }

        if (!geometry) return null

        let object: THREE.Object3D

        if (geomData.type === 'line') {
            const material = new THREE.LineBasicMaterial({ color: 0x0066cc })
            object = new THREE.Line(geometry, material)
        } else if (geomData.type === 'point') {
            const material = new THREE.PointsMaterial({ color: 0xff0000, size: 5 })
            object = new THREE.Points(geometry, material)
        } else {
            object = new THREE.Mesh(geometry, this.defaultMaterial.clone())
        }

        // 应用变换矩阵
        if (geomData.transform && geomData.transform.length === 16) {
            const matrix = new THREE.Matrix4()
            matrix.fromArray(geomData.transform)
            object.applyMatrix4(matrix)
        }

        // 设置用户数据
        object.userData = { id: geomData.id, type: geomData.type }

        return object
    }

    private createMeshGeometry(geomData: GeometryData): THREE.BufferGeometry {
        const geometry = new THREE.BufferGeometry()

        // 顶点数据
        const vertices = new Float32Array(geomData.vertices)
        geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3))

        // 索引数据
        if (geomData.indices && geomData.indices.length > 0) {
            const indices = new Uint32Array(geomData.indices)
            geometry.setIndex(new THREE.BufferAttribute(indices, 1))
        }

        // 法线数据
        if (geomData.normals && geomData.normals.length > 0) {
            const normals = new Float32Array(geomData.normals)
            geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3))
        } else {
            geometry.computeVertexNormals()
        }

        // 颜色数据
        if (geomData.colors && geomData.colors.length > 0) {
            const colors = new Float32Array(geomData.colors)
            geometry.setAttribute('color', new THREE.BufferAttribute(colors, 3))
        }

        return geometry
    }

    private createLineGeometry(geomData: GeometryData): THREE.BufferGeometry {
        const geometry = new THREE.BufferGeometry()
        const vertices = new Float32Array(geomData.vertices)
        geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3))
        return geometry
    }

    private createPointGeometry(geomData: GeometryData): THREE.BufferGeometry {
        const geometry = new THREE.BufferGeometry()
        const vertices = new Float32Array(geomData.vertices)
        geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3))
        return geometry
    }

    clearGeometries(): void {
        this.geometryObjects.forEach(object => {
            this.scene.remove(object)
            // 清理几何体和材质
            if (object instanceof THREE.Mesh || object instanceof THREE.Line || object instanceof THREE.Points) {
                object.geometry.dispose()
                if (Array.isArray(object.material)) {
                    object.material.forEach(mat => mat.dispose())
                } else {
                    object.material.dispose()
                }
            }
        })
        this.geometryObjects.clear()
        this.selectedObjects.clear()
    }

    // 选择方法
    selectObject(id: string): void {
        const object = this.geometryObjects.get(id)
        if (object && object instanceof THREE.Mesh) {
            object.material = this.selectedMaterial
            this.selectedObjects.add(id)
        }
    }

    deselectObject(id: string): void {
        const object = this.geometryObjects.get(id)
        if (object && object instanceof THREE.Mesh) {
            object.material = this.defaultMaterial
            this.selectedObjects.delete(id)
        }
    }

    clearSelection(): void {
        this.selectedObjects.forEach(id => {
            this.deselectObject(id)
        })
    }

    // 视图控制方法
    setViewMode(mode: 'perspective' | 'orthographic'): void {
        const position = this.camera.position.clone()
        const target = this.controls.target.clone()

        if (mode === 'orthographic' && this.camera instanceof THREE.PerspectiveCamera) {
            const aspect = this.container.clientWidth / this.container.clientHeight
            this.camera = new THREE.OrthographicCamera(-10 * aspect, 10 * aspect, 10, -10, 0.1, 1000)
        } else if (mode === 'perspective' && this.camera instanceof THREE.OrthographicCamera) {
            const aspect = this.container.clientWidth / this.container.clientHeight
            this.camera = new THREE.PerspectiveCamera(75, aspect, 0.1, 1000)
        }

        this.camera.position.copy(position)
        this.camera.lookAt(target)
        this.controls.object = this.camera
    }

    setWireframe(enabled: boolean): void {
        this.geometryObjects.forEach(object => {
            if (object instanceof THREE.Mesh) {
                if (enabled) {
                    object.material = this.wireframeMaterial
                } else {
                    const id = object.userData.id
                    object.material = this.selectedObjects.has(id) ? this.selectedMaterial : this.defaultMaterial
                }
            }
        })
    }

    setBackgroundColor(color: string): void {
        this.scene.background = new THREE.Color(color)
    }

    resetCamera(): void {
        this.camera.position.set(5, 5, 5)
        this.controls.target.set(0, 0, 0)
        this.controls.update()
    }

    // 获取相机信息
    getCameraInfo(): { position: THREE.Vector3; target: THREE.Vector3 } {
        return {
            position: this.camera.position.clone(),
            target: this.controls.target.clone()
        }
    }

    // 清理资源
    dispose(): void {
        window.removeEventListener('resize', this.onWindowResize.bind(this))
        this.clearGeometries()
        this.renderer.dispose()
        this.controls.dispose()
    }
}
