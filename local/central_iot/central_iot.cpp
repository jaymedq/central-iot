#include <cstdio>
#include <unistd.h>
#include "lora.h"

#define SPI_CHANNEL 0
#define SS_PIN      6
#define DIO0_PIN    7
#define RST_PIN     0

static const char *message = "";
int main() {
	printf("Setting up LoRa\n");
	LoRa lora(SPI_CHANNEL, SS_PIN, DIO0_PIN, RST_PIN);
	if (lora.begin()) {
		printf("LoRa setup successful: chipset version 0x%02x\n", lora.version());
		LoRaPacket p((unsigned char *)message, strlen(message));
		printf("Constructed packet: payload='%s', length=%d\n", p.getPayload(), p.payloadLength());
		printf("Configuring radio\n");
		lora.setFrequency(LoRa::FREQ_915)
			->setTXPower(10)
			->setSpreadFactor(LoRa::SF_8)
			->setBandwidth(LoRa::BW_500k)
			->setCodingRate(LoRa::CR_45)
			->setSyncWord(0x34)
			->setHeaderMode(LoRa::HM_EXPLICIT)
			->enableCRC();
		printf("  TX power     : %d dB\n", lora.getTXPower());
		printf("  Frequency    : %d Hz\n", lora.getFrequency());
		printf("  Spread factor: %d\n", lora.getSpreadFactor());
		printf("  Bandwidth    : %d Hz\n", lora.bw[lora.getBandwidth()]);
		printf("  Coding Rate  : 4/%d\n", lora.getCodingRate() + 4);
		printf("  Sync word    : 0x%02x\n", lora.getSyncWord());
		printf("  Header mode  : %s\n", lora.getHeaderMode() == LoRa::HM_IMPLICIT ? "Implicit" : "Explicit");
		printf("Transmitting packet\n");
		while(1)
		{
			usleep(1000000);
			size_t bytes = lora.transmitPacket(&p);
			printf("  %d bytes transmitted\n", bytes);
		}
	}
}