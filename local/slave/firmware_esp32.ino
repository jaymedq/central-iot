/*
  This is a simple example show the Heltec.LoRa recived data in OLED.

  The onboard OLED display is SSD1306 driver and I2C interface. In order to make the
  OLED correctly operation, you should output a high-low-high(1-0-1) signal by soft-
  ware to OLED's reset pin, the low-level signal at least 5ms.

  OLED pins to ESP32 GPIOs via this connecthin:
  OLED_SDA -- GPIO4
  OLED_SCL -- GPIO15
  OLED_RST -- GPIO16
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include "heltec.h" 
#include "images.h"

#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6
String rssi = "RSSI --";
String packSize = "--";
String packet ;

void logo(){
  Heltec.display->clear();
  Heltec.display->drawXbm(0,5,logo_width,logo_height,logo_bits);
  Heltec.display->display();
}

void LoRaData(){
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0 , 15 , "Received "+ packSize + " bytes");
  Heltec.display->drawStringMaxWidth(0 , 26 , 128, packet);
  Heltec.display->drawString(0, 0, rssi);  
  Heltec.display->display();
}

void cbk(int packetSize) {
  char tmp;
  packet ="";
  packSize = String(packetSize,DEC);
  Serial.printf("\npacketSize: %d => ", packetSize);
  
  for (int i = 0; i < packetSize; i++) { 

  tmp = (char) LoRa.read();
    
    packet += (char) tmp;

     Serial.printf("%02X ", (0xFF & tmp));
    
    }
  rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
  LoRaData();
}

void setup() { 
  
  
  
   //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
 
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  logo();
  delay(1500);
  Heltec.display->clear();
  
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 10, "Wait for incoming data...");
  Heltec.display->display();
  delay(1000);
  //LoRa.onReceive(cbk);
  LoRa.receive();
  LoRa.setSpreadingFactor(8);
  LoRa.setSignalBandwidth(500e3);
  Serial.begin(115200);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) { cbk(packetSize);  }
  delay(10);
  //Serial.printf(".\n");
}

//https://www.fernandok.com/2020/05/automacao-lora-e-app-fernando-k.html
//Envia um pacote LoRa
void sendLoRaPacket(String str) {
  //Inicializa o pacote
  LoRa.beginPacket();
  //Coloca a string no pacote
  LoRa.print(str);
  //Finaliza e envia o pacote
  LoRa.endPacket();
}
//Faz a leitura de um pacote (se chegou algum)
String readLoRaPacket() {
    String packet = "";
    //Verifica o tamanho do pacote
    int packetSize = LoRa.parsePacket();
    //Lê cada caractere e concatena na string 
    for (int i = 0; i < packetSize; i++) { 
      packet += (char) LoRa.read(); 
    }
    return packet;
}