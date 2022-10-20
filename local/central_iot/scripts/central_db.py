#!/usr/bin/python
from datetime import datetime
import os
import sqlite3
import sqlalchemy
from sqlalchemy.orm import declarative_base
from sqlalchemy import MetaData, insert, Table, select, delete, TIMESTAMP, text
from sqlalchemy.engine.url import URL


class CentralDb(object):

    Base = declarative_base()

    def __init__(self, db_url: URL = 'sqlite:////home/pi/central-iot/local/database/central.db'):
        self.db_url = db_url
        self.engine = sqlalchemy.create_engine(db_url, echo=True, future=True)
        self.metadata = MetaData()
        self.metadata.reflect(bind=self.engine)
        self.sensores_table = self.metadata.tables['sensores']
        self.dados_table = self.metadata.tables['dados']

    def insert_measure(self, device_id, sensor_id, value, unit):
        """Inserts a measure in local table"""
        with self.engine.connect() as conn:
            # stt = insert(self.dados_table).values(
            data_hora=datetime.now().isoformat()
            id_dispositivo=device_id
            id_sensor=sensor_id
            valor=value
            grandeza=str(unit)
            # )
            compiled = f'INSERT INTO dados (data_hora, id_dispositivo, id_sensor, valor, grandeza) VALUES {data_hora, id_dispositivo, id_sensor, valor, grandeza};'
            # compiled = stt.compile()
            conn.execute(text(compiled))
            conn.commit()

    def getRegisteredDevices(self) -> list:
        """Executes a select from Device table and returns a list of registered devices IDs"""
        registeredDevices = []
        with self.engine.connect() as conn:
            stt = select(self.sensores_table)
            result = conn.execute(stt)
            for row in result:
                registeredDevices.append(row.id_dispositivo)
        #remove repeated values
        return [*set(registeredDevices)]

    def getAllMeasures(self) -> list:
        """Executes a select from measure table and returns a list of results"""
        measurements = []
        with self.engine.connect() as conn:
            stt = select(self.dados_table)
            result = conn.execute(stt)
            for row in result:
                measurements.append(row)
        return measurements

    def deleteAllMeasures(self) -> list:
        """Executes a delete from measure table"""
        with self.engine.connect() as conn:
            stt = delete(self.dados_table)
            conn.execute(stt)
            conn.commit()


if __name__ == "__main__":
    db = CentralDb()
    print(db.getAllMeasures())
