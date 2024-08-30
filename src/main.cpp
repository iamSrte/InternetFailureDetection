#include <Arduino.h>
#include <Ethernet.h>
#include <Sim800L.h>
#include <SoftwareSerial.h>

#define ACTION_TIME 10     // Min
#define TIMEOUT_DURATION 5 // Min
#define UPDATE_INTERVAL 1  // Sec
#define SMS_ATTEMPTS_LIMIT 15
#define SWITCH_PIN 6

String phone_number = "";

uint16_t sms_attempts = 0;
uint64_t last_update_time = 0;
uint64_t no_ethernet_time = 0;
uint64_t no_internet_time = 0;
uint64_t last_ethernet_connection = 0;
uint64_t last_internet_connection = 0;

const uint64_t action_time = ACTION_TIME * 1000;
const uint64_t update_interval = UPDATE_INTERVAL * 1000;

uint64_t timeout_start = 0;
const uint64_t timeout_duration = TIMEOUT_DURATION * 60000;

bool timeout = false;
bool ethernet = false;
bool auto_action = true;
String sim_message = "";

uint8_t test_server_port = 53;
IPAddress test_server(8, 8, 8, 8);

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress ip(192, 168, 1, 222);
IPAddress my_dns(8, 8, 8, 8);

EthernetClient client;
Sim800L sim(2, 3);

bool sendMessageWithRetry(const String &, const String &, uint8_t);
void reboot(const String &);
void off(const String &);
void on(const String &);

void setup() {

  pinMode(SWITCH_PIN, OUTPUT);

  sim.begin(4800);
  sim.delAllSms();
  while (!sim.prepareForSmsReceive()) {
    delay(1000);
  }

  Serial.begin(115200);

  while (EthernetClass::linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    delay(1000);
  }

  Serial.println("Initialize Ethernet with DHCP:");
  if (EthernetClass::begin(mac) == 1) {
    Serial.print("DHCP assigned IP ");
    Serial.println(EthernetClass::localIP());

  } else {
    while (EthernetClass::hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet interface was not found!");
      delay(1000);
    }
    Serial.println(
        "Failed to configure Ethernet using DHCP, Connecting static!");
    EthernetClass::begin(mac, ip, my_dns);
  }

  delay(1000);
  Serial.print("connecting to ");
  Serial.print(test_server);
  Serial.println("...");

  if (client.connect(test_server, test_server_port)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
  } else {
    Serial.println("connection Failed");
  }
  client.stop();
}

void loop() {
  // Checking for new incoming message
  byte index = sim.checkForSMS();
  if (index != 0) {
    sim_message = sim.readSms(index);
    sim_message.trim();
    sim_message.toUpperCase();
    Serial.println("Received Message: " + sim_message);
    if (sim_message == "RESET")
      reboot(phone_number);
    else if (sim_message == "ON")
      on(phone_number);
    else if (sim_message == "OFF")
      off(phone_number);
  }

  // Checking internet connectivity if there is no timeout
  if (!timeout) {
    if (millis() - last_update_time > update_interval) {
      if (EthernetClass::linkStatus() == LinkON) {
        ethernet = true;
        last_ethernet_connection = millis();
        if (client.connect(test_server, test_server_port) == 1) {
          Serial.println("Connected");
          last_internet_connection = millis();
        } else {
          Serial.println("Connection Failed");
        }
        client.stop();
      } else {
        ethernet = false;
        Serial.println("Ethernet cable is not connected.");
      }
      sms_attempts = 0;
      last_update_time = millis();
      no_ethernet_time = millis() - last_ethernet_connection;
      no_internet_time = millis() - last_internet_connection;

      // Sending acknowledge for internet connection failure
      if (no_internet_time >= action_time && ethernet) {
        String temp = "Failed to connect to the internet for " +
                      String(int(no_internet_time / 1000)) + "s!\n";
        Serial.print(temp);
        sendMessageWithRetry(phone_number, temp, SMS_ATTEMPTS_LIMIT);
        if (auto_action) {
          reboot(phone_number);
        }
        timeout = true;
        timeout_start = millis();
      }

      // Sending acknowledge for ethernet connection failure
      if (no_ethernet_time >= action_time && !ethernet) {
        String temp = "No ethernet cable detected for " +
                      String(int(no_ethernet_time / 1000)) + "s!\n";
        Serial.print(temp);
        sendMessageWithRetry(phone_number, temp, SMS_ATTEMPTS_LIMIT);
        timeout = true;
        timeout_start = millis();
      }
    }
  }

  // Deactivate timeout after a certain amount of time
  if (timeout && ((millis() - timeout_start) > timeout_duration)) {
    timeout = false;
    last_ethernet_connection = millis();
    last_internet_connection = millis();
  }
}

bool sendMessageWithRetry(const String &number, const String &message,
                          uint8_t retry) {
  sms_attempts = 0;
  while (!sim.sendSms((char *)&number, (char *)&message)) {
    sms_attempts += 1;
    Serial.println("Failed to send '" + message + "' acknowledge SMS!");
    if (sms_attempts >= retry) {
      return false;
    }
  }
  return true;
}

void reboot(const String &number) {
  digitalWrite(SWITCH_PIN, LOW);
  delay(5000);
  digitalWrite(SWITCH_PIN, HIGH);
  sendMessageWithRetry(number, "Rebooting!", SMS_ATTEMPTS_LIMIT);
}

void off(const String &number) {
  digitalWrite(SWITCH_PIN, LOW);
  sendMessageWithRetry(number, "Turning OFF!", SMS_ATTEMPTS_LIMIT);
}

void on(const String &number) {
  digitalWrite(SWITCH_PIN, HIGH);
  sendMessageWithRetry(number, "Turning ON!", SMS_ATTEMPTS_LIMIT);
}
