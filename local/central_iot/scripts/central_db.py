#!/usr/bin/python  
from datetime import datetime
import os
import sqlite3
import sqlalchemy
from sqlalchemy.orm import declarative_base
from sqlalchemy import MetaData, insert, Table


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
        with self.engine.connect() as conn:
            stt = insert(self.user_table).values(
                Measure_ID = 1,
                Sensor_ID= sensor_id, 
                Date_Time= datetime.now().timestamp(), 
                Value= value
            )
            compiled = stt.compile()
            result = conn.execute(stt)
            conn.commit()

    