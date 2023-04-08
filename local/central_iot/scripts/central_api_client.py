import datetime
import requests
import json


class CentralAPIClient:
    def __init__(self, base_url):
        self.base_url = base_url
        self.token = None
    
    def login(self, username, password):
        endpoint = '/login'
        url = self.base_url + endpoint
        data = {'username': username, 'password': password}
        headers = {'Content-Type': 'application/json'}
        response = requests.post(url, data=json.dumps(data), headers=headers)
        if response.status_code == 200:
            self.token = response.json().get('token')
            print('Successfully logged in!')
        else:
            print('Error logging in:', response.json().get('message'))
    
    def get_sensors(self):
        if not self.token:
            print('You must login first!')
            return
        endpoint = '/sensors'
        url = self.base_url + endpoint
        headers = {'Authorization': 'Bearer ' + self.token}
        response = requests.get(url, headers=headers)
        json_data = response.json()
        if response.status_code == 200:
            print('Sensors:', json_data)
        else:
            print('Error getting sensors:', json_data.get('message'))
        sensor_list = []
        for sensor in json_data:
            sensor_list.append({
            "id_dispositivo": sensor.get("device_id"), 
            "empresa": sensor.get("owner"), 
            "id_sensor": sensor.get("sensor_id")
            })
        return sensor_list

    def post_measure(self, id, data_hora: datetime, id_dispositivo, id_sensor, valor, grandeza) -> requests.Response:
        """Executes a post request to central

        Args:
            data_hora (datetime): datetime in iso format.
            device_id (int): device id
            sensor_id (int): sensor id
            value (int): measurement
            measure_type (_type_): V, A, Wh, W, PF, etc.
        Returns:
            Response: Post request result
        """
        if not self.token:
            print('You must login first!')
            return
        headers = {'Authorization': 'Bearer ' + self.token}
        date_time = data_hora or datetime.now()
        jsonData = {
            "date_time": date_time.strftime("%Y-%m-%d %H:%M:%S"), 
            "device_id": id_dispositivo, 
            "sensor_id": id_sensor, 
            "value": valor, 
            "measure_type": str(grandeza)
        }
        r = requests.post(url = self.base_url+'/insert_measurement',json=jsonData, headers=headers)
        if r.status_code == 200:
            print(f"Measurement posted successfully = {jsonData.values()}")
        return r

