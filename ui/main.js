const { app, BrowserWindow, ipcMain } = require('electron')
const path = require('node:path')
const { spawn } = require('node:child_process')
const readline = require('node:readline')

let mainWindow = null;
let netmuxProcess = null;

function createWindow () {
  mainWindow = new BrowserWindow({
    width: 900,
    height: 700,
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
  createWindow()

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})

ipcMain.on('start-netmux', (event, config) => {
  if (netmuxProcess) {
    netmuxProcess.kill('SIGINT');
  }

  const exePath = path.join(__dirname, '..', 'build', 'NetMux');
  let args = [];

  if (config.isServer) {
    args.push('--server');
  } else {
    args.push('--client');
    args.push(config.remoteIp);
  }

  args.push('--port');
  args.push(config.port);

  if (config.securityKey) {
    args.push('--key');
    args.push(config.securityKey);
  }

  if (config.isLeft) {
    args.push('--left');
  } else {
    args.push('--right');
  }

  args.push('--auto-connect');

  console.log("Starting NetMux:", exePath, args.join(' '));

  netmuxProcess = spawn(exePath, args, { stdio: ['pipe', 'pipe', 'pipe'] });

  const rlOut = readline.createInterface({ input: netmuxProcess.stdout });
  rlOut.on('line', (line) => {
    try {
      if (line.startsWith('{')) {
        const data = JSON.parse(line);
        if (data.type === 'telemetry' && mainWindow) {
            mainWindow.webContents.send('telemetry-data', data);
        } else if (data.type === 'transfer_progress' && mainWindow) {
            mainWindow.webContents.send('transfer-progress', data);
        } else if (mainWindow) {
            mainWindow.webContents.send('log-data', { level: 'info', message: line });
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
    console.log(`child process exited with code ${code}`);
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

ipcMain.on('send-file', (event, fileData) => {
    if (netmuxProcess && netmuxProcess.stdin) {
        const command = {
            command: "send_file",
            path: fileData.path,
            target: fileData.target
        };
        netmuxProcess.stdin.write(JSON.stringify(command) + "\n");
        console.log("Sent file transfer command to NetMux:", command);
    } else {
        console.error("Failed to send file: NetMux process not running or stdin not available.");
        if (mainWindow) {
            mainWindow.webContents.send('log-data', { level: 'error', message: 'Failed to send file: NetMux is not running.' });
        }
    }
});
