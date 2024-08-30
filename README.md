# Internet Failure Detector

This project is an Internet Failure Detector that monitors internet connectivity using a W5500 Ethernet module connected to a modem. Upon detecting a failure, it automatically reboots the modem by cutting off its power supply via a relay circuit. The system also informs the user of the action taken via SMS, using the SIM800L module. The entire setup is controlled by an Arduino Nano and programmed using the Arduino framework.

## Circuit Diagram & Components Used

todo...

## Next step

- Implement the same logic using an ESP32-based circuit.
  - Utilize a wireless connection via Wi-Fi instead of using the W5500 Ethernet module.
  - Reboot the modem using Telnet commands instead of a relay circuit.
