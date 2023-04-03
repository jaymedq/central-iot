import crcmod
import sys
import os
import time
from enum import IntEnum
if __name__ == '__main__':
    sys.path.append(os.path.join(os.path.dirname(__file__)))
from central_lora import CentralLora
from central_db import CentralDb
from SX127x.board_config import BOARD
from datetime import datetime

DEFAULT_LORA_PARAMS = {
    "pa_select": 1,
    "max_power": 21,
    "output_power": 10,
    "bw": 500e3,
    "coding_rate": 1,
    "spreading_factor": 8,
    "sync_word": 0x34,
    "rx_crc": True,
    "freq": 915,
    "agc_auto_on": True,
    "implicit_header_mode": False,
}

class CentralUnits(IntEnum):
    UN = 0
    M = 1
    KG = 2
    S = 3
    A = 4
    PF = 5
    MOL = 6
    CD = 7
    OHM = 8
    SIE = 9
    Wb = 10
    T = 11
    Wh = 12
    W = 13
    RAD = 14
    BQ = 15
    LM = 16
    V = 17
    NTU = 18
    PPM = 19
    C = 20
    PC = 21


class NodeMessage(object):
    """Class to represent central iot node messages
    """
    payload = None
    stx = None
    node_addr = None
    centrl_addr = None
    sensor_id = None
    unit = None
    value = None
    crc = None
    etx = None
    calculated_crc = None
    is_valid = None

    def __init__(self, payload: list):
        """Construct the node message object from a given payload
        STX (1 byte)
        Node Addr (1 byte)
        Central Addr (1 byte)
        Sensor ID (1 byte)
        Type(1 byte)
        Value (2 bytes)
        CRC (1 byte)
        ETX (1 byte)

        Args:
            payload (list): list of bytes received from node
        """
        try:
            self.payload = payload
            self.stx = payload[0]
            self.node_addr = payload[1]
            self.centrl_addr = payload[2]
            self.sensor_id = payload[3]
            self.unit = CentralUnits(payload[4])
            self.value = int.from_bytes(bytes(payload[5:7]), 'big')
            self.crc = payload[7]
            self.etx = payload[8]
            self.calculated_crc = self.calculate_crc8(bytes(payload[0:7]))
            self.is_valid = self.is_message_valid()
        except Exception:
            self.is_valid = False

    def calculate_crc8(self, data):
        """Calculates crc using crcmod module

        Args:
            data (list): bytes to be 

        Returns:
            int: crc result
        """
        crc8 = crcmod.Crc(0x107, initCrc=0, rev=False, xorOut=0)
        crc8.update(data)
        # print('CRC8: ' + str(crc8.crcValue))
        # print('CRC8: ' + str(crc8.hexdigest()))
        return crc8.crcValue

    def is_message_valid(self):
        return self.crc == self.calculated_crc and self.stx == 0x02 and self.etx == 0x03


class CentralIot(object):

    def __init__(self, address: int = 1):
        self.address = address
        self.lora = CentralLora(verbose=True)
        # self.db = CentralDb(os.environ.get('CENTRAL_POSTGRES_URL', CentralDb.get_url_from_file()))
        self.db = CentralDb()
        self.registeredDevices = self.db.get_registered_devices()
        self.msg_history = []

    def setup_lora(self, loraParamsMap: dict = DEFAULT_LORA_PARAMS):

        self.lora.set_pa_config(pa_select=1, max_power=21, output_power=14)
        self.lora.set_bw(9)
        self.lora.set_coding_rate(1)
        self.lora.set_spreading_factor(8)
        self.lora.set_sync_word(0x34)
        self.lora.set_rx_crc(True)
        self.lora.set_freq(915)
        self.lora.set_agc_auto_on(True)
        self.lora.set_implicit_header_mode(False)

    def start(self):
        print(f'\nConfiguration: {self.lora}\n')
        while True:
            self.lora.reset_ptr_rx()
            self.lora.set_mode(0x85)  # Receiver mode
            start_time = time.time()
            # wait until receive data or 10s
            while (not self.lora.payload and time.time() - start_time < 10):
                pass
            if self.lora.payload and self.lora.payload[1]:
                parsed_message = self.parse_message(self.lora.payload)
                if parsed_message.node_addr in self.registeredDevices:
                    #Wait receiver to be ready
                    time.sleep(0.3)
                    if parsed_message.is_valid:
                        self.db.insert_measure(None, None, device_id=parsed_message.node_addr, sensor_id=parsed_message.sensor_id,
                                            value=parsed_message.value, unit=parsed_message.unit.name)
                        self.lora.send_ack(
                            central_addr=self.address, node_addr=parsed_message.node_addr)
                    else:
                        self.lora.send_nack(
                            central_addr=self.address, node_addr=parsed_message.node_addr)
            self.lora.payload = None

    def parse_message(self, message: bytes) -> NodeMessage:
        """Parses a message, save to history and reset payload value.

        Args:
            message (list): payload from node
        """
        parsed_message = NodeMessage(message)
        self.msg_history.append(parsed_message)
        return parsed_message


BOARD.setup()
BOARD.reset()
central_iot = CentralIot()
central_iot.setup_lora()
assert (central_iot.lora.get_agc_auto_on() == 1)

try:
    print("START")
    central_iot.start()
except KeyboardInterrupt:
    sys.stdout.flush()
    print("Exit")
    sys.stderr.write("KeyboardInterrupt\n")
except Exception as e:
    print(e)
    raise e
finally:
    sys.stdout.flush()
    print("Exit")
    central_iot.lora.set_mode(0x80)
    BOARD.teardown()
