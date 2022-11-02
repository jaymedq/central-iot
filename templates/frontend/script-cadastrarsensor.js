const form = document.querySelector("form");

let idDispositivo;
let idSensor;
let empresa;

form.addEventListener("submit", (e) => {
    e.preventDefault();
    idDispositivo = e.target.idDispositivo.value.trim();
    idSensor = e.target.idSensor.value.trim();
    empresa = e.target.empresa.value.trim();

    if (idDispositivo === "" || idSensor === "" || empresa === "") {
        alert("O campo não pode ser em branco!");
    } else if (isNaN(idDispositivo) || isNaN(idSensor)) {
        alert("ID do Dispositivo e ID do Sensor precisam ser números!")
    } else {
        fetch('https://central-iot.herokuapp.com/sensores', {
            method: 'post',
            body: JSON.stringify ({
                idDispositivo, idSensor, empresa
            }),
            headers: {
                "Content-Type": "application/json"
            }
        })
            .then(response => response.json())
            .then(response => alert(response.mensagem))
            .catch(erro => alert(erro.mensagem))
    }
});
