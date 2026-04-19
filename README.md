# Keyboard Monitor Plugin for OBS Studio

**Keyboard Monitor** is a high-performance OBS Studio plugin designed to visualize keyboard inputs in real-time. It provides a seamless way to show your viewers exactly what keys you are pressing, making it ideal for tutorials, speedrunning, and software demonstrations.

This project is a streamlined version of the original [StreamUP Hotkey Display](https://github.com/Andilippi/obs-streamup-hotkey-display), focusing exclusively on robust keyboard monitoring with a native OBS user experience.

## 🚀 Key Features

### ⌨️ Advanced Keyboard Monitoring
- **Real-time Visualization**: Instantly displays key combinations and single keystrokes.
- **Modifier Support**: Full detection of Ctrl, Alt, Shift, and Command (macOS) / Windows keys.
- **Custom Filters**: Toggle capture for Numpad keys, numbers, letters, and punctuation separately.
- **Whitelist System**: Manually specify exactly which keys should be displayed when pressed individually.

### 🌐 Direct Browser Integration
- **Zero-Latency Events**: Emits events directly into the OBS Browser Source environment using the internal `javascript_event` protocol.
- **Flexible Targeting**: Broadcast to all browser sources simultaneously or target a specific source by name.
- **Event Name**: `keyboard_monitor_input`
- **Payload**: Standardized JSON containing the key combination and individual key press data.

### 🛠 Native OBS Experience
- **Integrated Settings**: Access all configurations directly from the **Tools** menu.
- **Start with OBS**: Option to automatically enable monitoring as soon as OBS launches.
- **Persistence**: All settings (whitelists, targets, and preferences) are saved and reloaded automatically across sessions.
- **Native UI**: Uses standard OBS/Qt widgets for a consistent look and feel with any OBS theme.

## 🔧 Technical Details

### Browser Event Schema
The plugin emits a `CustomEvent` to the browser source with the following detail structure:

```json
{
  "key_combination": "Ctrl + C",
  "key_presses": [
    { "key": "Ctrl" },
    { "key": "C" }
  ]
}
```

### Compatibility
- **Windows**: Windows 10/11
- **macOS**: 11.0+ (Requires Accessibility Permissions)
- **Linux**: X11 environment (XWayland supported)

## 🏗 Installation

1. Download the latest release for your platform.
2. Place the `keyboard-monitor.plugin` (macOS) or `.dll` (Windows) into your OBS plugins directory.
3. Restart OBS Studio.
4. Go to **Tools** -> **Keyboard Monitor** to enable and configure the plugin.

---
*Based on the original StreamUP Hotkey Display. Focuses on speed, simplicity, and keyboard precision.*
