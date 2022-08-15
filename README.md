# ESP32RobotVacuum
A simple prototype of a robot vacuum based on an ESP32 microcontroller, a driver module for the motors and a relay module for the vacuum fans. The reason for selecting an ESP32 microcontroller for this project over other microcontrollers such as an ESP8266 or Arduino boards is that the ESP32 has a faster processor as well as a good memory size, it also supports Bluetooth and Wi-Fi functionality unlike the Arduino boards. 

Along with this hardware setup, an android app has been developed using MIT app Inventor to control the direction of the robot vacuum as well as the functioning of the vacuum fans. This app is connected to the ESP32 board via Bluetooth.

Another feature is the ability to update the firmware using Wi-Fi OTA whereby frequent connection to a laptop to upload firmware can be avoided.

The source code for this project is completely written in C++ using the PlatformIO extension in Visual Studio Code.


# Components:
* Adafruit HUZZAH32 - ESP32 Feather Board
* Sparkfun TB6612 Motor Driver
* Songle 5V Dual Channel Relay Module with Optocoupler
* 11.1 V LI-PO Battery as power source for motors as well as vacuum fans
* Power Bank used as 5V input supply(V<sub>in</sub>) to power the esp32.
* 2x 300 RPM BO Motors along with wheels
* 2x 12 V Brushless DC Cooling Blade Fan
* 3x HC-SR04 Ultrasonic Distance Sensor

----
Mobile application developed with the help of MIT App Inventor
http://ai2.appinventor.mit.edu/#4939517416308736
