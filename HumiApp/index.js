// index.js

const { app, BrowserWindow } = require('electron');
const path = require('path');

function createWindow() {
    // Create the browser window
    const win = new BrowserWindow({
        width: 800, // Set the width of the window
        height: 600, // Set the height of the window
        webPreferences: {
            contextIsolation: true, // For security
            preload: path.join(__dirname, 'preload.js') // Optional: Use a preload script if needed
        }
    });

    // Load the index.html file
    win.loadFile('index.html');

    // Open the DevTools (optional)
    win.webContents.openDevTools();
}

// This method will be called when Electron has finished initialization
app.whenReady().then(createWindow);

// Quit the app when all windows are closed, except on macOS
app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

// Create a new window when the app is activated (macOS)
app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
        createWindow();
    }
});
