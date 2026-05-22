# ESP32 Posture Detection System

## Overview

This system uses **3 ESP32 modules** to detect body posture in real time:

- **`esterno`** — ESP32 with MPU6050 sensor placed on the **chest**
- **`lombar`** — ESP32 with MPU6050 sensor placed on the **lower back**
- **`central`** — ESP32 that receives data from both sensors, runs the model, and outputs a prediction

> **No computer or `server.py` needed at runtime.** The `central` ESP32 acts as the server. The `server.py` script was only used during data collection for training. Once the model is deployed on the central ESP32, the system is fully standalone.

---

## Hardware Requirements

- 3x ESP32 (ESP32-WROOM-DA or compatible)
- 2x MPU6050 IMU sensor (one per sensor node)
- USB cables with **data lines** (not charge-only)
- A WiFi network that all 3 ESP32s can connect to

---

## Network Configuration

All 3 ESP32s must be on the **same WiFi network**. Update the following in each sketch:

```cpp
const char* SSID     = "YOUR_WIFI_NAME";
const char* PASSWORD = "YOUR_WIFI_PASSWORD";
```

### IP Addresses

The `central` ESP32 gets a dynamic IP assigned by your router. To find it:

1. Upload the `central` sketch first
2. Open Serial Monitor at **115200 baud**
3. You will see: `Connected: 192.168.x.xxx` — that is the central ESP32's IP

Then update both sensor sketches with that IP:

```cpp
const char* PC_IP = "192.168.x.xxx";  // IP of the central ESP32
```

> If the central ESP32 restarts and gets a new IP, you will need to re-upload the sensor sketches with the updated IP. To avoid this, consider reserving a static IP for the central ESP32 in your router's DHCP settings.

### UDP Port

All nodes communicate over **UDP port 4210**. This is the same across all 3 sketches and does not need to be changed.

---

## USB Port Mapping

When connecting all 3 ESP32s to your computer for uploading, identify which USB port corresponds to which board. On macOS, the relevant ports will appear as:

- `/dev/cu.usbserial-XXXX`

To identify each board:

1. Unplug all ESP32s
2. Plug them in **one at a time** and note which port appears in Arduino IDE (Tools → Port)
3. Open Serial Monitor — the board will print its `NODE_ID` (`"esterno"`, `"lombar"`, or connect message for central)

Keep a note of which port is which, for example:
- `/dev/cu.usbserial-0001` → `esterno`
- `/dev/cu.usbserial-1130` → `lombar`
- `/dev/cu.usbserial-7` → `central`

---

## Arduino Board Settings

In Arduino IDE, select:

- **Board:** `ESP32-WROOM-DA Module`
- **Upload Speed:** `115200`
- **Port:** whichever matches the board you are uploading to

---

## Upload Order

Follow this order every time:

### 1. Upload `central` sketch
- Select the central ESP32's port
- Upload and open Serial Monitor
- Wait for it to connect to WiFi and note the IP address printed

### 2. Update sensor sketches with the central IP
- In both `esterno` and `lombar` sketches, set `PC_IP` to the IP from step 1

### 3. Upload `lombar` sketch
- Select the lombar ESP32's port
- Upload

### 4. Upload `esterno` sketch
- Select the esterno ESP32's port
- Upload

Once all three are running, the central ESP32's Serial Monitor will print predictions in real time:

```
EST: 460.0 -120.0 17464.0 | LOM: 2592.0 -652.0 15908.0 --- Prediction: 3
```

---

## Prediction Rate

The system is configured to output **one prediction every 100ms (10Hz)**. This is controlled by:

```cpp
const unsigned long PREDICTION_COOLDOWN_MS = 100;
```

Adjust this value in the `central` sketch to change the prediction frequency.

---

## Replacing the Model

The `central` sketch contains a placeholder function:

```cpp
int simulateModel() {
  // TODO: replace this with actual model inference
  // All 12 input values are available as global variables:
  //   est_ax, est_ay, est_az, est_gx, est_gy, est_gz
  //   lom_ax, lom_ay, lom_az, lom_gx, lom_gy, lom_gz
  return random(1, 7); // returns a random class 1-6
}
```

To deploy the real model, replace the body of `simulateModel()` with the actual inference code. The 12 input variables are global and available directly — no need to pass them as arguments.

---

## Troubleshooting

**Upload fails with "No serial data received"**
Hold the **BOOT** button on the ESP32 while clicking Upload, release once writing starts.

**Upload fails mid-way / corrupted flash**
Run in Terminal:
```bash
python3 -m esptool --port /dev/cu.usbserial-XXXX erase_flash
```
Then re-upload. Also try plugging the ESP32 directly into the computer instead of through a USB hub.

**Central ESP32 receives no data**
- Confirm all 3 ESP32s are on the same WiFi network
- Confirm `PC_IP` in both sensor sketches matches the central ESP32's current IP
- The sensor ESP32s do not need to be restarted if the central restarts — they will resume automatically once the central is back online

**Only 2 USB ports appear instead of 3**
- One cable may be charge-only (no data lines) — swap it
- The USB hub may not supply enough power for 3 devices simultaneously — plug directly into the computer
