#include "heltec.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#define WIFI_CONNECT_MAX_RETRIES 100
#define CENTRAL_IOT_PROTOCOL_RETRIES 3
#define BAND 915E6 // you can set band here directly,e.g. 868E6,915E6

#define DEBUG 1
#if DEBUG == 1
  #define debugf(x, ...) Serial.printf(x, ##__VA_ARGS__)
#else
  #define debugf(x, ...)
#endif

uint8_t const localAddress = 0x01; // address of this device
uint8_t u8RegisteredDevices[50] = {0x0E}; // address of registered devices

const char* ssid = "Redmi Note 10";
const char* password = "behumble123";

//Your Domain name with URL path or IP address with path
String strObterSensores = "https://central-iot.herokuapp.com/obtersensores";
String strObterSalvarMedidas = "https://central-iot.herokuapp.com/salvarmedidas";


uint8_t stx = 0x02;          // start of transmission
uint8_t destination = 0x01;  // destination to send to
uint8_t sensorId = 0x01;     // sensor identification
uint8_t measureType = 0x01;  // Type of the measurement
uint8_t crc = 0x01;          // crc of transmission
uint8_t etx = 0x03;          // end of transmission
uint32_t centralFailures = 0;

uint8_t msgCount = 0;                    // count of outgoing messages
long lastSendTime = 0;                // last send time
int interval = 5000;                  // interval between sends

void setup()
{
    // Setup Lora
    Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
    Serial.begin(9600);
    Serial.flush();
    Serial.println("Heltec.LoRa Duplex");
    delay(50);
    Serial.print("Serial initial done\r\n");
    LoRa.setSpreadingFactor(8);
    LoRa.setSignalBandwidth(500e3);

    // Wifi setup
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    uint8_t u8Retries = 0;
    while((WiFi.status() != WL_CONNECTED) && (u8Retries < WIFI_CONNECT_MAX_RETRIES)) {
        delay(500);
        Serial.print(".");
        u8Retries++;
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    //Send an HTTP POST request every 10 minutes
    if ((millis() - lastTime) > timerDelay) {
        //Check WiFi connection status
        if(WiFi.status()== WL_CONNECTED){
            HTTPClient http;

            String serverPath = serverName + "?temperature=24.37";
            
            // Your Domain name with URL path or IP address with path
            http.begin(serverPath.c_str());
            
            // If you need Node-RED/server authentication, insert user and password below
            //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
            
            // Send HTTP GET request
            int httpResponseCode = http.GET();
            
            if (httpResponseCode>0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
            }
            else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
            }
            // Free resources
            http.end();
        }
    }
    if (modbusRequestResult == em210Modbus.ku8MBSuccess)
    {
        for (uint8_t u8Retries = 0; u8Retries < CENTRAL_IOT_PROTOCOL_RETRIES; u8Retries++)
        {
            bool centralOk = false;
            /* Central LoRa */
            if (not(millis() - lastSendTime > interval))
            {
            delay(millis() - lastSendTime);
            }
            uint8_t message[2] = {responseBuffer >> 8, responseBuffer & 0xff};
            sendMessage(message);
            Serial.println("Status: " + String(responseBuffer));
            lastSendTime = millis();        // timestamp the message
            interval = random(2000) + 1000; // 2-3 seconds
            LoRa.receive();
            // parse for a packet, and call onReceive with the result:
            int initial = millis();
            while (millis() - initial <= 5000)
            {
            int packetSize = LoRa.parsePacket();
            if (packetSize)
            {
                centralOk = onReceive(packetSize);
            }
            }
            if (centralOk)
            {
                break;
            }
            if (u8Retries > 0)
            {
            centralFailures++;
            Serial.print("Failed to receive response from CentralIoT after 5 seconds, retrying...");
            }
        }
    Serial.printf("\n-----------------\nTotal Failures: %d\n-----------------\n", centralFailures);
    }
}

void sendStatusMessage(uint8_t u8Destination, uint8_t u8StatusCode)
{
    LoRa.beginPacket();         // Start packet
    LoRa.write(stx);            // STX
    LoRa.write(localAddress);   // End origem
    LoRa.write(u8Destination);  // End destino
    LoRa.write(u8StatusCode);   // Status
    uint8_t data[] = {stx, localAddress, u8Destination, u8StatusCode};
    crc = crc8(data, 4, 0);
    LoRa.write(crc);            // CRC
    LoRa.write(etx);            // ETX
    LoRa.endPacket(); // finish packet and send it
    msgCount++;       // increment message ID
}

int onReceive(int packetSize)
{
    int isOk = 0;
    if (packetSize == 0)
        return 0; // if there's no packet, return
    Serial.println("\nDEBUG\n");
    // read packet header bytes:
    uint8_t u8Stx = LoRa.read();
    uint8_t u8NodeAddr = LoRa.read();
    uint8_t u8CentrlAddr = LoRa.read();
    uint8_t u8SensorId = LoRa.read();
    uint8_t u8Unit = LoRa.read();
    uint8_t u8Value = LoRa.read();
    uint8_t u8Crc = LoRa.read();
    uint8_t u8Etx = LoRa.read();
    uint8_t u8Payload[] = {u8Stx, u8NodeAddr, u8CentrlAddr, u8SensorId, u8Unit, u8Value};
    uint8_t u8CalculatedCrc = crc8();
    bool bIsValid = (u8CalculatedCrc == u8Crc) ? true : false;

  // if the recipient isn't this device or broadcast,
  if (receiver != localAddress)
  {
    Serial.println("This message is not for me.");
    return 0; // skip rest of function
  }
  if (bIsValid == true)
  {
    Serial.println("Central sent status ok");
    isOk = 1;
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(receiver, HEX));
  Serial.println("Status: " + String(status));
  Serial.println("Crc: " + String(crc));
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  return (bool)isOk;
}

// Automatically generated CRC function
// polynomial: 0x107
uint8_t crc8(uint8_t *data, int len, uint8_t crc)
{
  static const uint8_t table[256] = {
    0x00U, 0x07U, 0x0EU, 0x09U, 0x1CU, 0x1BU, 0x12U, 0x15U,
    0x38U, 0x3FU, 0x36U, 0x31U, 0x24U, 0x23U, 0x2AU, 0x2DU,
    0x70U, 0x77U, 0x7EU, 0x79U, 0x6CU, 0x6BU, 0x62U, 0x65U,
    0x48U, 0x4FU, 0x46U, 0x41U, 0x54U, 0x53U, 0x5AU, 0x5DU,
    0xE0U, 0xE7U, 0xEEU, 0xE9U, 0xFCU, 0xFBU, 0xF2U, 0xF5U,
    0xD8U, 0xDFU, 0xD6U, 0xD1U, 0xC4U, 0xC3U, 0xCAU, 0xCDU,
    0x90U, 0x97U, 0x9EU, 0x99U, 0x8CU, 0x8BU, 0x82U, 0x85U,
    0xA8U, 0xAFU, 0xA6U, 0xA1U, 0xB4U, 0xB3U, 0xBAU, 0xBDU,
    0xC7U, 0xC0U, 0xC9U, 0xCEU, 0xDBU, 0xDCU, 0xD5U, 0xD2U,
    0xFFU, 0xF8U, 0xF1U, 0xF6U, 0xE3U, 0xE4U, 0xEDU, 0xEAU,
    0xB7U, 0xB0U, 0xB9U, 0xBEU, 0xABU, 0xACU, 0xA5U, 0xA2U,
    0x8FU, 0x88U, 0x81U, 0x86U, 0x93U, 0x94U, 0x9DU, 0x9AU,
    0x27U, 0x20U, 0x29U, 0x2EU, 0x3BU, 0x3CU, 0x35U, 0x32U,
    0x1FU, 0x18U, 0x11U, 0x16U, 0x03U, 0x04U, 0x0DU, 0x0AU,
    0x57U, 0x50U, 0x59U, 0x5EU, 0x4BU, 0x4CU, 0x45U, 0x42U,
    0x6FU, 0x68U, 0x61U, 0x66U, 0x73U, 0x74U, 0x7DU, 0x7AU,
    0x89U, 0x8EU, 0x87U, 0x80U, 0x95U, 0x92U, 0x9BU, 0x9CU,
    0xB1U, 0xB6U, 0xBFU, 0xB8U, 0xADU, 0xAAU, 0xA3U, 0xA4U,
    0xF9U, 0xFEU, 0xF7U, 0xF0U, 0xE5U, 0xE2U, 0xEBU, 0xECU,
    0xC1U, 0xC6U, 0xCFU, 0xC8U, 0xDDU, 0xDAU, 0xD3U, 0xD4U,
    0x69U, 0x6EU, 0x67U, 0x60U, 0x75U, 0x72U, 0x7BU, 0x7CU,
    0x51U, 0x56U, 0x5FU, 0x58U, 0x4DU, 0x4AU, 0x43U, 0x44U,
    0x19U, 0x1EU, 0x17U, 0x10U, 0x05U, 0x02U, 0x0BU, 0x0CU,
    0x21U, 0x26U, 0x2FU, 0x28U, 0x3DU, 0x3AU, 0x33U, 0x34U,
    0x4EU, 0x49U, 0x40U, 0x47U, 0x52U, 0x55U, 0x5CU, 0x5BU,
    0x76U, 0x71U, 0x78U, 0x7FU, 0x6AU, 0x6DU, 0x64U, 0x63U,
    0x3EU, 0x39U, 0x30U, 0x37U, 0x22U, 0x25U, 0x2CU, 0x2BU,
    0x06U, 0x01U, 0x08U, 0x0FU, 0x1AU, 0x1DU, 0x14U, 0x13U,
    0xAEU, 0xA9U, 0xA0U, 0xA7U, 0xB2U, 0xB5U, 0xBCU, 0xBBU,
    0x96U, 0x91U, 0x98U, 0x9FU, 0x8AU, 0x8DU, 0x84U, 0x83U,
    0xDEU, 0xD9U, 0xD0U, 0xD7U, 0xC2U, 0xC5U, 0xCCU, 0xCBU,
    0xE6U, 0xE1U, 0xE8U, 0xEFU, 0xFAU, 0xFDU, 0xF4U, 0xF3U,
  };

  while (len > 0)
  {
    crc = table[*data ^ (uint8_t)crc];
    data++;
    len--;
  }
  return crc;
}