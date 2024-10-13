// preload.js

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electron', {
    ipcRenderer: {
        on: (channel, func) => ipcRenderer.on(channel, (event, ...args) => func(event, ...args)),
        send: (channel, data) => ipcRenderer.send(channel, data)
    },
    document: window.document // Expose document
});
