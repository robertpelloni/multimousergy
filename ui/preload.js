const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('electronAPI', {
  startNetMux: (config) => ipcRenderer.send('start-netmux', config),
  stopNetMux: () => ipcRenderer.send('stop-netmux'),
  onTelemetry: (callback) => ipcRenderer.on('telemetry-data', (_event, value) => callback(value)),
  onLog: (callback) => ipcRenderer.on('log-data', (_event, value) => callback(value)),
  onNetMuxExit: (callback) => ipcRenderer.on('netmux-exit', (_event, code) => callback(code))
})
