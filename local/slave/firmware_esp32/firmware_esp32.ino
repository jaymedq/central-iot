#include "heltec.h"
#include <ModbusMaster.h>

#define MAX485_DE 25
#define MAX485_RE_NEG 33
#define CENTRAL_IOT_PROTOCOL_RETRIES 3
#define BAND 915E6 // you can set band here directly,e.g. 868E6,915E6
#define FIRMWARE_DELAY_SECONDS 900 // 15 minutes in seconds

#define DEBUG 1
#if DEBUG == 1
  #define debugf(x, ...) Serial.printf(x, ##__VA_ARGS__)
#else
  #define debugf(x, ...)
#endif

struct Measurement {
  uint16_t value;
  uint8_t measureType;
};

ModbusMaster em210Modbus;
typedef enum
{
  eUNIT_UN = 0,
  eUNIT_M,
  eUNIT_KG,
  eUNIT_S,
  eUNIT_A,
  eUNIT_PF,
  eUNIT_MOL,
  eUNIT_CD,
  eUNIT_OHM,
  eUNIT_SIE,
  eUNIT_Wb,
  eUNIT_T,
  eUNIT_Wh,
  eUNIT_W,
  eUNIT_RAD,
  eUNIT_BQ,
  eUNIT_LM,
  eUNIT_V,
  eUNIT_NTU,
  eUNIT_PPM,
  eUNIT_C,
  eUNIT_PC
} eCentralUnits;

uint8_t stx = 0x02;          // start of transmission
uint8_t localAddress = 0x01; // address of this device
uint8_t destination = 0x01;  // destination to send to
uint8_t sensorId = 0x01;     // sensor identification
uint8_t measureType = 0x01;  // Type of the measurement
uint8_t crc = 0x01;          // crc of transmission
uint8_t etx = 0x03;          // end of transmission
uint32_t centralFailures = 0;

uint8_t msgCount = 0;                    // count of outgoing messages
long lastSendTime = 0;                // last send time
int interval = 5000;                  // interval between sends
uint8_t u8AvailableMeters[] = {1,2,3,4,5,6,7,8,9,10}; //,3,4,5,6};

void preTransmission()
{
  digitalWrite(MAX485_DE, 1);
  digitalWrite(MAX485_RE_NEG, 1);
  delay(1);
}

void postTransmission()
{
  delay(1);
  digitalWrite(MAX485_DE, 0);
  digitalWrite(MAX485_RE_NEG, 0);
}

void setup()
{
  // WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Serial.begin(9600);
  Serial.flush();
  Serial.println("Heltec.LoRa Duplex");
  Serial2.begin(9600, SERIAL_8N1, 22, 23);
  delay(50);
  Serial.print("Serial initial done\r\n");
  LoRa.setSpreadingFactor(8);
  LoRa.setSignalBandwidth(500e3);

  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE_NEG, OUTPUT);
  digitalWrite(MAX485_DE, 0);
  digitalWrite(MAX485_RE_NEG, 0);
  em210Modbus.begin(1, Serial2);
  em210Modbus.preTransmission(preTransmission);
  em210Modbus.postTransmission(postTransmission);
}

typedef enum
{
  eEM210_VLL = 0, // 0026h
  eEM210_W,       // 0028h
  eEM210_WH,      // 0112h
  eEM210_PF,      // 010Ch
  eEM210_N
} em210RegistersDescription;

const uint8_t AVAILABLE_METERS_COUNT = sizeof(u8AvailableMeters) / sizeof(u8AvailableMeters[0]);
const uint8_t MAX_MEASUREMENT_CONTEXT = 4;
const uint8_t MODBUS_RETRY_COUNT = 40;
const unsigned long MIN_INTERVAL = 1000;
const unsigned long MAX_INTERVAL = 3000;

Measurement measurementBuffer[MAX_MEASUREMENT_CONTEXT];

void sendMeasurementToCentralIoT(Measurement measurement)
{
  if(0 != measurement.measureType)
  {
    sendMessage(measurement);
    Serial.println("[LORA] Measurement sent to Central: Type " + String(measurement.measureType) + " Value " + String(measurement.value));
  }
}

bool readMeasurement(ModbusMaster& em210Modbus, uint16_t registerAddress, uint8_t measureType, uint8_t measurementIndex)
{
  for (uint8_t retryCount = 0; retryCount < MODBUS_RETRY_COUNT; retryCount++)
  {
    Serial.print("[MODBUS] Sending modbus command...");
    em210Modbus.clearResponseBuffer();
    em210Modbus.clearTransmitBuffer();

    uint8_t modbusRequestResult = em210Modbus.readHoldingRegisters(registerAddress, 2);
    debugf("\n--- Expected Slave ADDR: %d ",em210Modbus._u8MBSlave);
    debugf("Actual: %d ---\n", em210Modbus._u8ModbusADU[0]);
    debugf("\n --- Whole buffer: ---\n");
    for(uint8_t i = 0; i<30;i++)
    {
      debugf(" %d", em210Modbus._u8ModbusADU[i]);
    }

    debugf("\nModBusRequestResult\n%d", modbusRequestResult);
    if (modbusRequestResult == em210Modbus.ku8MBSuccess)
    {
      uint16_t responseBuffer = em210Modbus.getResponseBuffer(0x00);
      Serial.println(" Success, value: " + String(responseBuffer));
      em210Modbus.clearResponseBuffer();
      measurementBuffer[measurementIndex] = {responseBuffer, measureType};
      return true;
    }
    else if(modbusRequestResult == em210Modbus.ku8MBResponseTimedOut)
    {
      Serial.println(" Failed to read, due timeout.");
      return false;
    }
    else
    {
      Serial.println(" Failed to read, retrying...");
      delay(10);
    }
  }

  return false;
}

void sendMeasurementsFromBuffer()
{
  bool centralOk = false;
  for (uint8_t measurementIndex = 0; measurementIndex < MAX_MEASUREMENT_CONTEXT; measurementIndex++)
  {
    if(0 != measurementBuffer[measurementIndex].measureType)
    {
      /* Central LoRa */
      if (false == (millis() - lastSendTime > interval))
      {
        delay(millis() - lastSendTime);
      }
      for (uint8_t u8Retries = 0; u8Retries < CENTRAL_IOT_PROTOCOL_RETRIES; u8Retries++)
      {
        sendMeasurementToCentralIoT(measurementBuffer[measurementIndex]);
        Serial.println("Status: " + String(measurementBuffer[measurementIndex].value));
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
    }
  }
}

uint8_t detectedMeters = 6;
void detectAndAddMeters()
{
  detectedMeters = 0;
  uint8_t detectedMeterIndex = 0;
  uint8_t maxDetectedMeters = sizeof(u8AvailableMeters) / sizeof(u8AvailableMeters[0]);

  for (uint8_t sensorId = 1; sensorId <= maxDetectedMeters; sensorId++) {
    em210Modbus.begin(sensorId, Serial2);
    if (readMeasurement(em210Modbus, 0x0028, eUNIT_W, 0)) {  // Read eUNIT_W register
      u8AvailableMeters[detectedMeterIndex++] = sensorId;
      detectedMeters++;
      if (detectedMeters >= maxDetectedMeters) {
        break;  // Maximum number of detected meters reached
      }
    }
  }

  // Print the detected meters
  Serial.print("Detected meters: ");
  for (uint8_t i = 0; i < detectedMeters; i++) {
    Serial.print(u8AvailableMeters[i]);
    Serial.print(" ");
  }
  Serial.println();
}


void loop()
{
  debugf("Begin of outer loop");

  eCentralUnits unit;
  em210RegistersDescription context;

  for (uint8_t meterIndex = 0; meterIndex < detectedMeters; meterIndex++)
  {
    debugf("---- Using sensor %d ----\n", meterIndex);
    sensorId = u8AvailableMeters[meterIndex];
    em210Modbus.begin(sensorId, Serial2);

    for(uint8_t index = 0; index < MAX_MEASUREMENT_CONTEXT; index++)
    {
      measurementBuffer[index].value = 0;
      measurementBuffer[index].measureType = 0;
    }

    for (uint32_t measurementIndex = 0; measurementIndex < MAX_MEASUREMENT_CONTEXT; measurementIndex++)
    {
      uint16_t registerAddress = 0;
      uint8_t measureType = 0;
      
      switch (measurementIndex)
      {
        case 0:
          Serial.print("V L-L: ");
          registerAddress = 0x0026;
          measureType = eUNIT_V;
          break;
        case 1:
          Serial.print("Watt: ");
          registerAddress = 0x0028;
          measureType = eUNIT_W;
          break;
        case 2:
          Serial.print("Watt hour: ");
          registerAddress = 0x0112;
          measureType = eUNIT_Wh;
          break;
        case 3:
          Serial.print("Power Factor: ");
          registerAddress = 0x010C;
          measureType = eUNIT_PF;
          break;
        default:
          Serial.print("SHOULD NOT BE HERE, CHECK! ");
          break;
      }

      if (registerAddress)
      {
        if (readMeasurement(em210Modbus, registerAddress, measureType, measurementIndex))
          continue;
        else
        {
          measurementBuffer[measurementIndex].value = 0;
          measurementBuffer[measurementIndex].measureType = 0;
        }
      }

      Serial.println("Failed to read measurement after retries.");
    }

    sendMeasurementsFromBuffer();

    debugf("[DEBUG] End of meter measurement loop ");
  }

  debugf("[DEBUG] End of %d meters reading loop ", detectedMeters);
  detectAndAddMeters();
  debugf("[DEBUG] Detected %d meters, sleeping for %d seconds",detectedMeters, FIRMWARE_DELAY_SECONDS);
  delay(FIRMWARE_DELAY_SECONDS*1000);
}



void sendMessage(Measurement outputMeasurement)
{
  uint8_t outgoing[2] = {outputMeasurement.value >> 8, outputMeasurement.value & 0xFF};
  LoRa.beginPacket(); // start packet

  LoRa.write(stx);          // start of transmission
  LoRa.write(localAddress); // address of this device
  LoRa.write(destination);  // destination to send to
  LoRa.write(sensorId);     // sensor identification
  LoRa.write(outputMeasurement.measureType);  // Type of the measurement
  LoRa.write(outgoing[0]);  // add payload
  LoRa.write(outgoing[1]);  // add payload
  uint8_t data[] = {stx, localAddress, destination, sensorId, outputMeasurement.measureType, outgoing[0], outgoing[1]};
  crc = crc8(data, 7, 0);
  LoRa.write(crc); // end of transmission
  LoRa.write(etx); // end of transmission

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
  uint8_t stx = LoRa.read();      // STX
  uint8_t sender = LoRa.read();   // End origem
  uint8_t receiver = LoRa.read(); // End destino
  uint8_t status = LoRa.read();   // Status
  uint8_t crc = LoRa.read();      // CRC
  uint8_t erx = LoRa.read();      // ETX

  // if the recipient isn't this device or broadcast,
  if (receiver != localAddress)
  {
    Serial.println("This message is not for me.");
    return 0; // skip rest of function
  }
  if (status == 0x01)
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
