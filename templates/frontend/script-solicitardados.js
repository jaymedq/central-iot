const form = document.querySelector("form");
const button = document.querySelectorAll("button")[1];
const inputIdDispositivo = document.querySelector("[name=idDispositivo]");
const inputDataInicial = document.querySelector("[name=dtInicial]");
const inputDataFinal = document.querySelector("[name=dtFinal]");

let idDispositivo;
let dtInicial;
let dtFinal;
let dados;
let tableBody = document.querySelector('.js-body');
let table = document.querySelector('.central-table');

form.addEventListener("submit", (e) => {
    e.preventDefault();
    idDispositivo = e.target.idDispositivo.value.trim();
    dtInicial = e.target.dtInicial.value;
    dtFinal = e.target.dtFinal.value;

    const d1 = new Date(dtInicial);
    const d2 = new Date(dtFinal);

    if (d1 > d2) {
        alert("A data inicial não pode ser maior que a data final.")
    } else {
        dtInicial += " 00:00:00";
        dtFinal += " 23:59:59";

        fetch('https://central-iot.herokuapp.com/dados?' + new URLSearchParams({
            idDispositivo, dtInicial, dtFinal
        }))
            .then(response => response.json())
            .then(response => montTable(response))
    }
});

button.addEventListener("click", () => {
    const idDispositivo1 = inputIdDispositivo.value.trim();
    let dtInicial1 = inputDataInicial.value;
    let dtFinal1 = inputDataFinal.value;

    const d1 = new Date(dtInicial1);
    const d2 = new Date(dtFinal1);

    if (d1 > d2) {
        alert("A data inicial não pode ser maior que a data final.")
    } else {
        dtInicial1 += " 00:00:00";
        dtFinal1 += " 23:59:59";

        fetch('https://central-iot.herokuapp.com/dados?' + new URLSearchParams({
            idDispositivo: idDispositivo1, dtInicial: dtInicial1, dtFinal: dtFinal1
        }))
            .then(response => response.json())
            .then(response => {
                const formatador = new json2csv.Parser(response);
                const csv = formatador.parse(response);
                const instrucao = "data:text/csv;charset=utf-8,";
                window.open(instrucao + csv);
            })
    }
})

function montTable(dado) {
    table.style = "display: table";
    if (dado.length > 0) {
        let row = "";
        dado.forEach(dado => {
            row += `
            <tr>
                <td>${dado.id_sensor}</td>
                <td>${dado.data_hora}</td>
                <td>${dado.valor}</td>
                <td>${dado.grandeza}</td>
            </tr>
            `
            tableBody.innerHTML = row;
        })
    } else {
        alert("Não existem dados para esta pesquisa.");
        table.style = "display: none";
    }

};
