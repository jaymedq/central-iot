from SX127x.board_config import BOARD
from SX127x.LoRa import *
import time
from copy import copy
""
#!/usr/bin/env python3

""" This program asks a client for data and waits for the response, then sends an ACK. """

class CentralLora(LoRa):
    def __init__(self, verbose=False):
        super(CentralLora, self).__init__(verbose)
        self.set_mode(MODE.SLEEP)
        self.set_dio_mapping([0] * 6)
        self.var = 0
        self.payload = None

    def on_rx_done(self):
        BOARD.led_on()
        # print("\nRxDone")
        self.clear_irq_flags(RxDone=1)
        self.payload = self.read_payload(nocheck=True)
        payload = copy(self.payload)
        print("Receive: ")
        print(payload)
        # print(bytes(payload).decode("utf-8", 'ignore'))  # Receive DATA
        print(f'SNR = {self.get_pkt_snr_value()}')
        print(f'RSSI= {self.get_pkt_rssi_value()}')
        BOARD.led_off()
        self.set_mode(MODE.SLEEP)

    def send_ack(self, central_addr:int = 0x01, node_addr:int = 0x02):
        """Create ack request based on received message

        Args:
            central_addr (int): integer representing the address
            node_addr (int): integer representing the address
        """
        print("Send: ACK")
        ack = [0x02, central_addr, node_addr, int(True), 0x01, 0x03]
        self.write_payload(ack)  # Send ACK
        self.set_mode(MODE.TX)
        time.sleep(0.2)
        print(ack)
    
    def send_nack(self, central_addr:int = 0x01, node_addr:int = 0x02):
        """Create NACK request based on received message

        Args:
            central_addr (int): integer representing the address
            node_addr (int): integer representing the address
        """
        print("Send: NACK")
        ack = [0x02, central_addr, node_addr, int(False), 0x01, 0x03]
        self.set_mode(MODE.TX)
        self.write_payload(ack)  # Send ACK
        time.sleep(0.2)
        print(ack)

    def on_tx_done(self):
        print("\nTxDone")
        print(self.get_irq_flags())

    def on_cad_done(self):
        print("\non_CadDone")
        print(self.get_irq_flags())

    def on_rx_timeout(self):
        print("\non_RxTimeout")
        print(self.get_irq_flags())

    def on_valid_header(self):
        print("\non_ValidHeader")
        print(self.get_irq_flags())

    def on_payload_crc_error(self):
        print("\non_PayloadCrcError")
        print(self.get_irq_flags())

    def on_fhss_change_channel(self):
        print("\non_FhssChangeChannel")
        print(self.get_irq_flags())
