const { app, BrowserWindow, ipcMain, Tray, Menu, nativeImage } = require('electron')
const path = require('node:path')
const fs = require('node:fs')
const { spawn } = require('node:child_process')
const readline = require('node:readline')

let mainWindow = null;
let netmuxProcess = null;
let tray = null;

const CONFIG_FILE_PATH = path.join(__dirname, '..', 'netmux.cfg');

// Helper to parse INI/key-value style netmux.cfg
function readConfigFile() {
  const defaultCfg = {
    server_mode: 'false',
    remote_ip: '127.0.0.1',
    port: '5555',
    boundary_x: '1919',
    boundary_y: '0',
    is_left: 'true',
    driver_type: '2',
    group_id: '0',
    group_name: 'DefaultGroup',
    session_name: 'DefaultSession',
    security_key: '',
    cursor_theme_path: '',
    sel_color_r: '0',
    sel_color_g: '120',
    sel_color_b: '215',
    peer_color_r: '255',
    peer_color_g: '0',
    peer_color_b: '0',
    auto_connect: 'false',
    start_minimized: 'false',
    display_name: 'Workstation',
    use_d3d11: 'true',
    spatial_mode: 'true',
    headless: 'false',
    recent_servers: '127.0.0.1,192.168.1.100'
  };

  try {
    if (fs.existsSync(CONFIG_FILE_PATH)) {
      const data = fs.readFileSync(CONFIG_FILE_PATH, 'utf-8');
      const lines = data.split(/\r?\n/);
      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed || trimmed.startsWith('#')) continue;
        const eqIdx = trimmed.indexOf('=');
        if (eqIdx !== -1) {
          const key = trimmed.slice(0, eqIdx).trim();
          const val = trimmed.slice(eqIdx + 1).trim();
          defaultCfg[key] = val;
        }
      }
    }
  } catch (err) {
    console.error('Failed to read config file:', err);
  }
  return defaultCfg;
}

// Helper to save key-value style netmux.cfg
function writeConfigFile(cfg) {
  try {
    let content = '# NetMux Configuration (Unified Dashboard)\n';
    for (const [key, val] of Object.entries(cfg)) {
      content += `${key}=${val}\n`;
    }
    fs.writeFileSync(CONFIG_FILE_PATH, content, 'utf-8');
    return true;
  } catch (err) {
    console.error('Failed to write config file:', err);
    return false;
  }
}

function createTrayIcon() {
  const icon = nativeImage.createFromDataURL(
    'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAUklEQVR4nGNgGHRgNBg+g4o5B0oY0AF5hv9kGvAfyRikBiSg2A34j0czugH4NGAzEptR2AzFaQDe9BgNl1EAMYECN8kYqE5fQ/g3Gqhh0AMA7+s8j+0XlR4AAAAASUVORK5CYII='
  );

  tray = new Tray(icon);
  const contextMenu = Menu.buildFromTemplate([
    {
      label: 'Show Dashboard',
      click: () => {
        if (mainWindow) {
          mainWindow.show();
          mainWindow.focus();
        }
      }
    },
    {
      label: 'Stop NetMux Backend',
      click: () => {
        if (netmuxProcess) {
          netmuxProcess.kill('SIGINT');
          netmuxProcess = null;
        }
      }
    },
    { type: 'separator' },
    {
      label: 'Quit MultiMousergy',
      click: () => {
        if (netmuxProcess) {
          netmuxProcess.kill('SIGINT');
          netmuxProcess = null;
        }
        app.quit();
      }
    }
  ]);

  tray.setToolTip('MultiMousergy - Spatial Multi-Cursor System');
  tray.setContextMenu(contextMenu);
  tray.on('double-click', () => {
    if (mainWindow) {
      mainWindow.isVisible() ? mainWindow.hide() : mainWindow.show();
    }
  });
}

function createWindow () {
  mainWindow = new BrowserWindow({
    width: 1120,
    height: 820,
    minWidth: 900,
    minHeight: 650,
    title: 'MultiMousergy Command Dashboard',
    backgroundColor: '#121418',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true
    }
  })

  mainWindow.loadFile('index.html')

  mainWindow.on('closed', () => {
    mainWindow = null;
    if (netmuxProcess) {
      netmuxProcess.kill('SIGINT');
      netmuxProcess = null;
    }
  })
}

app.whenReady().then(() => {
  createWindow();
  try {
    createTrayIcon();
  } catch (e) {
    console.warn('System tray setup warning (headless or minimal environment):', e.message);
  }

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})

ipcMain.handle('load-config', async () => {
  return readConfigFile();
});

ipcMain.handle('save-config', async (event, config) => {
  return writeConfigFile(config);
});

ipcMain.on('quit-app', () => {
  if (netmuxProcess) {
    netmuxProcess.kill('SIGINT');
    netmuxProcess = null;
  }
  app.quit();
});

ipcMain.on('initiate-file-transfer', (event, filePath) => {
  if (mainWindow) {
    mainWindow.webContents.send('log-data', {
      level: 'info',
      message: `[FileTransfer] Initiated staged transfer for: ${filePath}`
    });
  }
});

ipcMain.on('start-netmux', (event, config) => {
  if (netmuxProcess) {
    netmuxProcess.kill('SIGINT');
  }

  const fileCfg = {
    server_mode: config.isServer ? 'true' : 'false',
    remote_ip: config.remoteIp || '127.0.0.1',
    port: String(config.port || 5555),
    boundary_x: String(config.boundaryX ?? 1919),
    boundary_y: String(config.boundaryY ?? 0),
    is_left: config.isLeft ? 'true' : 'false',
    driver_type: String(config.driverType ?? 2),
    group_id: String(config.groupId ?? 0),
    group_name: config.groupName || 'DefaultGroup',
    session_name: config.sessionName || 'DefaultSession',
    security_key: config.securityKey || '',
    cursor_theme_path: config.cursorThemePath || '',
    sel_color_r: String(config.selColorR ?? 0),
    sel_color_g: String(config.selColorG ?? 120),
    sel_color_b: String(config.selColorB ?? 215),
    peer_color_r: String(config.peerColorR ?? 255),
    peer_color_g: String(config.peerColorG ?? 0),
    peer_color_b: String(config.peerColorB ?? 0),
    auto_connect: config.autoConnect ? 'true' : 'false',
    start_minimized: config.startMinimized ? 'true' : 'false',
    display_name: config.displayName || 'Workstation',
    use_d3d11: config.useD3D11 ? 'true' : 'false',
    spatial_mode: config.spatialMode ? 'true' : 'false',
    enable_audio: config.enableAudio ? 'true' : 'false',
    enable_webcam: config.enableWebcam ? 'true' : 'false',
    headless: 'true',
    recent_servers: config.recentServers || '127.0.0.1'
  };
  writeConfigFile(fileCfg);

  const winReleaseExe = path.join(__dirname, '..', 'build', 'Release', 'NetMux.exe');
  const winNinjaExe = path.join(__dirname, '..', 'build', 'NetMux.exe');
  const exePath = process.platform === 'win32'
    ? (fs.existsSync(winReleaseExe) ? winReleaseExe : winNinjaExe)
    : path.join(__dirname, '..', 'build', 'NetMux');

  let args = [];

  if (config.isServer) {
    args.push('--server');
  } else {
    args.push('--client');
    args.push(config.remoteIp || '127.0.0.1');
  }

  args.push('--port');
  args.push(String(config.port || 5555));

  args.push('--boundary-x');
  args.push(String(config.boundaryX ?? (config.isLeft ? 0 : 1919)));

  args.push('--boundary-y');
  args.push(String(config.boundaryY ?? 0));

  if (config.isLeft) {
    args.push('--left');
  } else {
    args.push('--right');
  }

  if (config.securityKey) {
    args.push('--key');
    args.push(config.securityKey);
  }

  if (config.useD3D11) {
    args.push('--d3d11');
  }

  args.push('--headless');

  if (config.autoConnect) {
    args.push('--auto-connect');
  }

  if (config.driverType !== undefined) {
    args.push('--driver-type');
    args.push(String(config.driverType));
  }

  if (config.displayName) {
    args.push('--display-name');
    args.push(config.displayName);
  }

  if (config.sessionName) {
    args.push('--session-name');
    args.push(config.sessionName);
  }

  if (config.groupName) {
    args.push('--group-name');
    args.push(config.groupName);
  }

  if (config.groupId !== undefined) {
    args.push('--group-id');
    args.push(String(config.groupId));
  }

  if (config.peerColorR !== undefined) {
    args.push('--peer-color');
    args.push(`${config.peerColorR},${config.peerColorG},${config.peerColorB}`);
  }

  if (mainWindow) {
    mainWindow.webContents.send('log-data', {
      level: 'info',
      message: `[Launcher] Spawning backend: ${exePath} ${args.join(' ')}`
    });
  }

  netmuxProcess = spawn(exePath, args, { stdio: ['pipe', 'pipe', 'pipe'] });

  const rlOut = readline.createInterface({ input: netmuxProcess.stdout });
  rlOut.on('line', (line) => {
    try {
      if (line.startsWith('{') && line.includes('"type":"telemetry"')) {
        const data = JSON.parse(line);
        if (mainWindow) {
            mainWindow.webContents.send('telemetry-data', data);
        }
      } else {
        if (mainWindow) {
            mainWindow.webContents.send('log-data', { level: 'info', message: line });
        }
      }
    } catch (e) {
      if (mainWindow) {
          mainWindow.webContents.send('log-data', { level: 'info', message: line });
      }
    }
  });

  const rlErr = readline.createInterface({ input: netmuxProcess.stderr });
  rlErr.on('line', (line) => {
    if (mainWindow) {
        mainWindow.webContents.send('log-data', { level: 'error', message: line });
    }
  });

  netmuxProcess.on('close', (code) => {
    netmuxProcess = null;
    if (mainWindow) {
        mainWindow.webContents.send('netmux-exit', code);
    }
  });
});

ipcMain.on('stop-netmux', () => {
    if (netmuxProcess) {
        netmuxProcess.kill('SIGINT');
        netmuxProcess = null;
    }
});
