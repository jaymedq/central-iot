/*
  Heltec.LoRa Multiple Communication

  This example provide a simple way to achieve one to multiple devices
  communication.

  Each devices send datas in broadcast method. Make sure each devices
  working in the same BAND, then set the localAddress and destination
  as you want.
  
  Sends a message every half second, and polls continually
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFD as the broadcast address. You can set this address as you
  want.

  Note: while sending, Heltec.LoRa radio is not listening for incoming messages.
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include "heltec.h"
#include <ModbusMaster.h>

#define MAX485_DE 25
#define MAX485_RE_NEG 33

#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

ModbusMaster em210Modbus;

byte stx = 0x02;              // start of transmission
byte localAddress = 0x02;     // address of this device
byte destination = 0x01;      // destination to send to
byte sensorId = 0x01;         // sensor identification
byte measureType = 0x01;      // Type of the measurement
byte crc = 0x01;              // crc of transmission
byte etx = 0x03;              // end of transmission

byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
int interval = 5000;          // interval between sends


void preTransmission()
{
  digitalWrite(MAX485_DE, 1);
  digitalWrite(MAX485_RE_NEG, 1);
  delay(1);
}

void postTransmission()
{
  delay(2);
  digitalWrite(MAX485_DE, 0);
  digitalWrite(MAX485_RE_NEG, 0);
}

void setup()
{
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Serial.begin(9600);
  Serial.flush();
  Serial.println("Heltec.LoRa Duplex");
  delay(50);
  Serial.print("Serial initial done\r\n");
  LoRa.setSpreadingFactor(8);
  LoRa.setSignalBandwidth(500e3);

  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE_NEG, OUTPUT);
  digitalWrite(MAX485_DE, 0);
  digitalWrite(MAX485_RE_NEG, 0);
  em210Modbus.begin(1, Serial);
  em210Modbus.preTransmission(preTransmission);
  em210Modbus.postTransmission(postTransmission);
}

void loop()
{
  /* Central LoRa */
  if (millis() - lastSendTime > interval)
  {
    String message = "PPPP";   // send a message
    sendMessage(message);
    //Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;    // 2-3 seconds
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
  
  /* RS485 Modbus EM210 */
  uint8_t result;

  // Read 2 registers starting at 300001)
  result = em210Modbus.readInputRegisters(0x0000, 2);
  if (result == em210Modbus.ku8MBSuccess)
  {
    Serial.print("V L1-N: ");
    Serial.println(em210Modbus.getResponseBuffer(0x01));
    Serial.println(em210Modbus.getResponseBuffer(0x02));
  }
  else
  {
    //Serial.print("Failed ");
  } 
  delay(1000);

}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet

  LoRa.write(stx);                      // start of transmission
  LoRa.write(localAddress);             // address of this device
  LoRa.write(destination);              // destination to send to
  LoRa.write(sensorId);                 // sensor identification
  LoRa.write(measureType);              // Type of the measurement
  LoRa.print(outgoing);                 // add payload
  LoRa.write(crc);                      // end of transmission
  LoRa.write(etx);                      // end of transmission

  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

int onReceive(int packetSize)
{
  int isOk = 0;
  if (packetSize == 0) return 0;          // if there's no packet, return

  // read packet header bytes:
  byte stx = LoRa.read();               // STX
  byte sender = LoRa.read();            // End origem
  byte receiver = LoRa.read();          // End destino
  byte status = LoRa.read();            // Status
  byte crc = LoRa.read();               // CRC
  byte erx = LoRa.read();               // ETX

  // if the recipient isn't this device or broadcast,
  if (receiver != localAddress) {
    Serial.println("This message is not for me.");
    return 0;                             // skip rest of function
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
  return isOk;
}