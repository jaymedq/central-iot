#!/usr/bin/python
from datetime import datetime
import os
import sqlite3
import sqlalchemy
from sqlalchemy.orm import declarative_base
from sqlalchemy import MetaData, select, delete, text, Table, Column
from sqlalchemy.dialects.sqlite import DATETIME
from sqlalchemy.engine.url import URL


class CentralDb(object):

    Base = declarative_base()

    def __init__(self, db_url: URL = 'sqlite:////home/pi/central-iot/local/database/central.db'):
        self.db_url = db_url
        self.engine = sqlalchemy.create_engine(db_url, echo=True, future=True)
        self.metadata = MetaData()
        # self.metadata.reflect(bind=self.engine)
        # self.sensores_table = self.metadata.tables['sensores']
        self.sensores_table = Table(
            "sensores",
            self.metadata,
            # additional Column objects which require no change are reflected normally
            autoload_with=self.engine,
        )
        if self.engine.driver == 'pysqlite':
            self.dados_table = Table(
                "dados",
                self.metadata,
                Column("data_hora", DATETIME(timezone = True, regexp=r"(\d+)-(\d+)-(\d+)T(\d+):(\d+):(\d+)\.?(\d*)")),
                # additional Column objects which require no change are reflected normally
                autoload_with=self.engine,
            )
        else:
            self.dados_table = Table(
                "dados",
                self.metadata,
                Column("data_hora", DATETIME(timezone = True)),
                # additional Column objects which require no change are reflected normally
                autoload_with=self.engine,
            )

    def insert_measure(self, id, data_hora: datetime, device_id, sensor_id, value, unit):
        """Inserts a measure in local table"""
        with self.engine.connect() as conn:
            # stt = insert(self.dados_table).values(
            data_hora=data_hora.isoformat() or datetime.now().isoformat()
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
    
    @staticmethod
    def get_url_from_file(path: os.path = None):
        with open(path or os.path.join(os.path.dirname(__file__),'central_url.txt')) as file:
            content = file.read()
        return content

if __name__ == "__main__":
    remoteDb = CentralDb(CentralDb.get_url_from_file())
    localDb = CentralDb()
    allLocalMeasures = []
    # print(remoteDb.getAllMeasures())
    allLocalMeasures.extend(localDb.getAllMeasures())
    if allLocalMeasures:
        print(allLocalMeasures)
        for measure in allLocalMeasures:
            remoteDb.insert_measure(*measure)
    localDb.deleteAllMeasures()
