# ESP32-CAM Telegram Photo Alarm
In [this video](https://www.youtube.com/watch?v=GQDIlpdFAmA&t), I made an alarm using the ESP32-CAM board, which captures and sends a photo to Telegram when motion is detected. You can also start an HTTP video stream from Telegram using this alarm. You can use a web interface that runs on a local web server or Telegram to configure alarm parameters. You can also watch [this video](https://www.youtube.com/watch?v=AkVpEbDvJB0&t), in which I talked about the components required to create this alarm, printing the alarm body on a 3D printer and other technical details.

# Arduino Libraries
Before uploading the sketch to the ESP32-CAM board, you must download these libraries for the Arduino IDE:
- [Universal Telegram Bot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/tree/V1.2.0)
- [Arduino Json 6.16.1](https://downloads.arduino.cc/libraries/github.com/bblanchon/ArduinoJson-6.16.1.zip)