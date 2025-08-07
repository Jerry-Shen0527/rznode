// RzNode API 客户端
class RzNodeAPI {
    constructor(baseURL = 'http://localhost:8080') {
        this.baseURL = baseURL;
    }

    async getStatus() {
        const response = await fetch(`${this.baseURL}/api/status`);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        return await response.json();
    }

    async getNodeTypes() {
        const response = await fetch(`${this.baseURL}/api/node-types`);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        return await response.json();
    }

    async executeNodeTree(nodeTree) {
        const response = await fetch(`${this.baseURL}/api/execute`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(nodeTree)
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        return await response.json();
    }

    async validateNodeTree(nodeTree) {
        const response = await fetch(`${this.baseURL}/api/validate`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(nodeTree)
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        return await response.json();
    }
}

export default RzNodeAPI;
