#!/usr/bin/python  
from datetime import datetime
import os
import sqlite3
import sqlalchemy
from sqlalchemy.orm import declarative_base
from sqlalchemy import MetaData, insert, Table, select, delete


class CentralDb(object):

    Base = declarative_base()

    def __init__(self, db_path: os.path = 'sqlite:////home/pi/central-iot/local/database/central.db'):
        self.db_path = db_path
        self.engine = sqlalchemy.create_engine(f'sqlite:///{db_path}', echo = True, future = True)
        self.metadata = MetaData()
        self.metadata.reflect(bind = self.engine)
        self.user_table = self.metadata.tables['User']
        self.device_table = self.metadata.tables['Device']
        self.sensor_table = self.metadata.tables['Sensor']
        self.measure_table = self.metadata.tables['Measure']

    def insert_measure(self, sensor_id, value):
        """Inserts a measure in local table"""
        with self.engine.connect() as conn:
            stt = insert(self.measure_table).values(
                Sensor_ID= sensor_id, 
                Date_Time= datetime.now().timestamp(), 
                Value= value
            )
            compiled = stt.compile()
            conn.execute(compiled)
            conn.commit()

    def getRegisteredDevices(self) -> list:
        """Executes a select from Device table and returns a list of registered devices IDs"""
        registeredDevices = []
        with self.engine.connect() as conn:
            stt = select(self.device_table)
            result = conn.execute(stt)
            for row in result:
                registeredDevices.append(row.Device_ID)
        return registeredDevices

if __name__ == "__main__":
    db = CentralDb()
    db.insert_measure(1,50)
