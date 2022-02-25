const WebSocket = require('ws');

/**
 * Hook api launches a api which sends states data to requesters.
 */
class ApiManager {
    #stateManager = null;
    #wss = null;

    constructor(apiPort, stateManager) {
        this.#stateManager = stateManager;
        this.#wss = new WebSocket.Server({ port: apiPort })
    }

    init() {
        this.#wss.on('connection', ws => {
            this.#wss.on('message', message => {
                console.log(`Received message => ${message}`)
                this.#wss.send('Hello! Message From Server!!')
            });
        });
    }
}

module.exports = {
    ApiManager
}