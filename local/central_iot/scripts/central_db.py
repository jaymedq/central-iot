#!/usr/bin/python
from datetime import datetime
import os
import sqlite3
import sqlalchemy
from sqlalchemy.orm import declarative_base
from sqlalchemy import MetaData, select, delete, text, Table, Column
from sqlalchemy.engine import Result
from sqlalchemy.dialects.sqlite import DATETIME
from sqlalchemy.engine.url import URL
import requests
from requests import Response

from central_api_client import CentralAPIClient

CENTRAL_URL = "https://api.vistalux.com.br"

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
            data_hora=data_hora.isoformat() if data_hora else datetime.now().isoformat()
            id_dispositivo=device_id
            id_sensor=sensor_id
            valor=value
            grandeza=str(unit)
            compiled = f'INSERT INTO dados (data_hora, id_dispositivo, id_sensor, valor, grandeza) VALUES {data_hora, id_dispositivo, id_sensor, valor, grandeza};'
            # compiled = stt.compile()
            conn.execute(text(compiled))
            conn.commit()

    def get_registered_devices(self) -> list:
        """Executes a select from Device table and returns a list of registered devices IDs"""
        registered_devices = []
        with self.engine.connect() as conn:
            stt = select(self.sensores_table)
            result = conn.execute(stt)
            for row in result:
                registered_devices.append(row.id_dispositivo)
        #remove repeated values
        return [*set(registered_devices)]
    
    def get_registered_sensors(self) -> Result:
        """Executes a select from sensors table and returns :class:`_engine.Result`."""
        with self.engine.connect() as conn:
            stt = select(self.sensores_table)
            result = conn.execute(stt)
        return result

    def get_all_measures(self) -> Result:
        """Executes a select from measure table and returns a Result"""
        # measurements = []
        with self.engine.connect() as conn:
            stt = select(self.dados_table)
            result = conn.execute(stt).mappings().all()
            # for row in result:
            #     measurements.append(row)
        return result

    def delete_all_measures(self) -> list:
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


# OLD IMPLEMENTATION (Directly to remote database):
# def update_remote_measures_and_local_sensors():
#     remote_db = CentralDb(CentralDb.get_url_from_file())
#     local_db = CentralDb()
#     all_local_measures = local_db.get_all_measures()
#     with remote_db.engine.connect() as remote_conn:
#         remote_conn.execute(remote_db.dados_table.insert(),all_local_measures)
#         remote_conn.commit()
#     # if all_local_measures:
#     #     print(all_local_measures)
#     #     for measure in all_local_measures:
#     #         remote_db.insert_measure(*measure)
#     with local_db.engine.connect() as conn:
#         remote_values = remote_db.get_registered_sensors().mappings().all()
#         conn.execute(local_db.sensores_table.delete())
#         conn.execute(local_db.sensores_table.insert(),remote_values)
#         conn.commit()
#         y = conn.execute(local_db.sensores_table.select()).mappings().all()
#     local_db.delete_all_measures()

def update_remote_measures_and_local_sensors():
    """
    Updates the remote measures with local database entries 
    and also updates the local sensors table with remote values
    """
    local_db = CentralDb()
    api_client = CentralAPIClient(CENTRAL_URL)
    api_client.login("central","geptcc22")
    all_local_measures = local_db.get_all_measures()
    for measure in all_local_measures:
        api_client.post_measure(**measure)
    remote_registered_sensors = api_client.get_sensors()
    with local_db.engine.connect() as conn:
        conn.execute(local_db.sensores_table.delete())
        conn.execute(local_db.sensores_table.insert(),remote_registered_sensors)
        conn.commit()
        # y = conn.execute(local_db.sensores_table.select()).mappings().all()
    local_db.delete_all_measures()

if __name__ == "__main__":
    update_remote_measures_and_local_sensors()