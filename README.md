# ESP8266 Messenger Chat Server

A lightweight **WiFi Messenger Chat Server** built using the ESP8266.
Users can connect to the ESP8266 hotspot and chat with each other through a browser.

This project is designed for **IoT learning, networking experiments, and CSIT projects**.

---

## ✨ Features

* Local WiFi chat server
* Messenger-style chat UI
* Username login system
* Delete messages for everyone
* Mobile and desktop responsive interface
* No internet required
* Runs entirely on ESP8266

---

## 🛠 Hardware Required

* ESP8266 (NodeMCU / ESP-12E)
* USB cable
* Laptop or phone

---

## 💻 Software Required

* Arduino IDE
* ESP8266 Board Package

---

## 📡 How It Works

1. ESP8266 creates a WiFi hotspot
2. Users connect to the hotspot
3. Open a browser and go to:

```
http://192.168.4.1
```

4. Enter username
5. Start chatting with other connected users

---

## 🚀 Installation

### 1. Install ESP8266 Board

Open Arduino IDE

```
File → Preferences
```

Add board manager URL:

```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

Then install:

```
Tools → Board Manager → ESP8266
```

---

### 2. Upload Code

Open

```
src/esp8266_chat_server.ino
```

Select board:

```
NodeMCU 1.0 (ESP-12E Module)
```

Upload the code.

---

## 📶 Connect to Chat

WiFi Name:

```
ESP_CHAT
```

Password:

```
12345678
```

Open browser:

```
192.168.4.1
```

---

## 📷 Screenshot

Add a screenshot inside the `docs/` folder.

---

## 🤝 Contributing

Contributions are welcome!
You can improve UI, performance, or features.

See **CONTRIBUTING.md** for details.

---

## 📜 License

This project is licensed under the MIT License.

---

## ⭐ Support

If you like this project, please **star the repository**.
