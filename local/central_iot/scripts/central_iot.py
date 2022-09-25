import crcmod
import sys
import time
from central_lora import CentralLora
from central_db import CentralDb
from SX127x.board_config import BOARD

DEFAULT_LORA_PARAMS = {
    "pa_select":1, 
    "max_power":21, 
    "output_power":10,
    "bw":500e3,
    "coding_rate":1,
    "spreading_factor":8,
    "sync_word":0x34,
    "rx_crc":True,
    "freq":915,
    "agc_auto_on":True,
    "implicit_header_mode":False,
}

class NodeMessage(object):
    """Class to represent central iot node messages
    """        

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
        self.payload = payload
        self.stx = payload[0]
        self.node_addr = payload[1]
        self.centrl_addr = payload[2]
        self.sensor_id = payload[3]
        self.type = payload[4]
        self.value = payload[5] << 16 + payload[6]
        self.crc = payload[7]
        self.etx = payload[8]
        self.calculated_crc = self.calculate_crc8(bytes(payload))
        self.is_valid = self.is_message_valid()

    def calculate_crc8(data):
        """Calculates crc using crcmod module

        Args:
            data (list): bytes to be 

        Returns:
            int: crc result
        """        
        crc8 = crcmod.Crc(0x107,initCrc=0,rev=False,xorOut=0)
        crc8.update(data)
        # print('CRC8: ' + str(crc8.crcValue))
        # print('CRC8: ' + str(crc8.hexdigest()))
        return crc8.crcValue

    def is_message_valid(self):
        return self.crc == self.calculated_crc and self.stx == 0x02 and self.etx == 0x03

class CentralIot(object):

    def __init__(self, address: int = 0):
        self.address = address
        self.lora = CentralLora(verbose=True)
        self.db = CentralDb('C:\\Users\\claud\\Desktop\\Jayme\\central-iot\\local\\database\\central.db')
        self.msg_history = []

    def setup_lora(self, loraParamsMap: dict = DEFAULT_LORA_PARAMS):
        
        self.lora.set_pa_config(pa_select=1, max_power=21, output_power=10)
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
            while (not self.lora.payload or time.time() - start_time < 10):  # wait until receive data or 10s
                pass
            parsed_message = self.parse_message(self.lora.payload)
            self.lora.payload = None
            if parsed_message.is_valid:
                self.db.insert_measure(parsed_message.sensor_id, value = parsed_message.value)
                self.lora.send_ack(central_addr = parsed_message.centrl_addr, node_addr = parsed_message.node_addr)
            else:
                self.lora.send_nack(central_addr = parsed_message.centrl_addr, node_addr = parsed_message.node_addr)

    def parse_message(self, message:bytes)->NodeMessage:
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
finally:
    sys.stdout.flush()
    print("Exit")
    central_iot.lora.set_mode(0x80)
    BOARD.teardown()

