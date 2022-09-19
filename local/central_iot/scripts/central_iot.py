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

class CentralIot(object):

    def __init__(self, address: int = 0):
        self.address = address
        self.lora = CentralLora(verbose=True)
        self.db = CentralDb('/home/pi/central-iot/local/database/central.db')

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
            # self.reset_ptr_rx()
            # self.set_mode(MODE.RXCONT) # Receiver mode
            # while self.var == 0:
            #     pass;
            self.lora.var = 0
            while (self.lora.var == 0):
                self.lora.reset_ptr_rx()
                self.lora.set_mode(0x85)  # Receiver mode

                start_time = time.time()
                while (time.time() - start_time < 10):  # wait until receive data or 10s
                    pass
            

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

