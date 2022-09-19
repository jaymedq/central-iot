from SX127x.board_config import BOARD
from SX127x.LoRa import *
import time
from copy import copy
""
#!/usr/bin/env python3

""" This program asks a client for data and waits for the response, then sends an ACK. """

# Copyright 2018 Rui Silva.
#
# This file is part of rpsreal/pySX127x, fork of mayeranalytics/pySX127x.
#
# pySX127x is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# pySX127x is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
# details.
#
# You can be released from the requirements of the license by obtaining a commercial license. Such a license is
# mandatory as soon as you develop commercial activities involving pySX127x without disclosing the source code of your
# own applications, or shipping pySX127x with a closed source product.
#
# You should have received a copy of the GNU General Public License along with pySX127.  If not, see
# <http://www.gnu.org/licenses/>.

class CentralLora(LoRa):
    def __init__(self, verbose=False):
        super(CentralLora, self).__init__(verbose)
        self.set_mode(MODE.SLEEP)
        self.set_dio_mapping([0] * 6)
        self.var = 0
        self.payload = 0

    def on_rx_done(self):
        BOARD.led_on()
        # print("\nRxDone")
        self.clear_irq_flags(RxDone=1)
        self.payload = self.read_payload(nocheck=True)
        payload = copy(self.payload)
        print("Receive: ")
        print(payload)
        print(bytes(payload).decode("utf-8", 'ignore'))  # Receive DATA
        BOARD.led_off()
        time.sleep(2)  # Wait for the client be ready
        print("Send: ACK")
        ack = [0x02, 0x01, 0x02, 0x01, 0x01, 0x03]
        self.write_payload(ack)  # Send ACK
        self.set_mode(MODE.TX)
        print(ack)
        self.var = 1

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
