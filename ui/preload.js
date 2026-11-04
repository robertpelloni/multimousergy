const { contextBridge, ipcRenderer } = require('electron')

// Expose safe, isolated API to renderer process
contextBridge.exposeInMainWorld('electronAPI', {
  // Lifecycle controls
  startNetMux: (config) => ipcRenderer.send('start-netmux', config),
  stopNetMux: () => ipcRenderer.send('stop-netmux'),
  quitApp: () => ipcRenderer.send('quit-app'),

  // Configuration persistence
  loadConfig: () => ipcRenderer.invoke('load-config'),
  saveConfig: (config) => ipcRenderer.invoke('save-config', config),

  // File drop transfer simulation/action
  initiateFileTransfer: (filePath) => ipcRenderer.send('initiate-file-transfer', filePath),

  // Event streams
  onTelemetry: (callback) => ipcRenderer.on('telemetry-data', (_event, value) => callback(value)),
  onLog: (callback) => ipcRenderer.on('log-data', (_event, value) => callback(value)),
  onNetMuxExit: (callback) => ipcRenderer.on('netmux-exit', (_event, code) => callback(code))
})
