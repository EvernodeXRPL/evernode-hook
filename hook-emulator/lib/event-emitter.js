class EventEmitter {
    constructor() {
        this.handlers = {};
    }

    on(event, handler) {
        if (!this.handlers[event])
            this.handlers[event] = [];
        this.handlers[event].push({
            once: false,
            func: handler
        });
    }

    once(event, handler) {
        if (!this.handlers[event])
            this.handlers[event] = [];
        this.handlers[event].push({
            once: true,
            func: handler
        });
    }

    off(event, handler = null) {
        if (this.handlers[event]) {
            if (handler)
                this.handlers[event] = this.handlers[event].filter(h => h !== handler);
            else
                delete this.handlers[event];
        }
    }

    emit(event, value, error = null) {
        if (this.handlers[event]) {
            this.handlers[event].forEach(handler => handler.func(value, error));

            // Rmove all handlers marked as 'once'.
            this.handlers[event] = this.handlers[event].filter(h => !h.once);
        }
    }
}

module.exports = {
    EventEmitter
}