const { Sequelize, Op } = require('sequelize');
const express = require('express');
const { startOfDay, parseISO } = require('date-fns');
const app = express();
const cors = require('cors');
const { utcToZonedTime, zonedTimeToUtc } = require('date-fns-tz');
const port = process.env.PORT || 3000;
app.use(cors());
require('dotenv').config()

const sequelize = new Sequelize(process.env.DATABASE_URL, {
    dialectOptions: {
        useUTC: true,
        ssl: {
            require: true,
            rejectUnauthorized: false
        }
    },
    timezone: '+00:00'
})

sequelize
    .authenticate()
    .then(() => {
        console.log('Connection has been established successfully.');
    })
    .catch(err => {
        console.error('Unable to connect to the database:', err);
    });


/* BANCO DE DADOS */

const Login = sequelize.define(
    "acesso",
    { usuario: Sequelize.STRING, senha: Sequelize.STRING },
    { timestamps: false }
);

const Sensores = sequelize.define(
    "sensores",
    { id_dispositivo: Sequelize.INTEGER, id_sensor: Sequelize.INTEGER, empresa: Sequelize.STRING },
    { timestamps: false, indexes: [{ unique: true, fields: ["id_dispositivo", "id_sensor"] }] }
);

const Dados = sequelize.define(
    "dados",
    { data_hora: Sequelize.DATE, id_dispositivo: Sequelize.INTEGER, id_sensor: Sequelize.INTEGER, valor: Sequelize.INTEGER, grandeza: Sequelize.STRING },
    { timestamps: false }
);

// createTable();

// async function createTable() {
//     Dados.sync();
// }

// criarDado();

/* SELECT NA TABELA ACESSOS */

async function getUsers() {
    const users = await Login.findAll();
    return users;
}

async function getData(idDispositivo, dtInicial, dtFinal) {
    const dataInicial = new Date(dtInicial).toISOString();
    const dataFinal = new Date(dtFinal).toISOString();


    const data = await Dados.findAll({
        where: {
            id_dispositivo: idDispositivo,
            data_hora: {
                [Op.between]: [dataInicial, dataFinal]
            }
        }
    });
    return data.map((obj) => {
        return {
            ...obj.dataValues,
            data_hora: new Date(obj.data_hora).toLocaleString("pt-BR")
        }
    })
}

/* SELECT NA TABELA SENSORES */

// async function getSensors(dispositivo, sensor) {
//     const sensors = await Sensores.findOne(dispositivo, sensor);
//     return sensors;
// }


/* INSERT NA TABELA SENSORES */

async function saveSensors(sensor) {
    await Sensores.create({ ...sensor, id_dispositivo: Number(sensor.idDispositivo), id_sensor: Number(sensor.idSensor) });
}


/* ENDPOINTS */

app.use(express.json());

app.get('/', async function (req, res) {
    res.json(await getUsers());
});


app.post('/sensores', async function (req, res) {

    await saveSensors(req.body)
        .then(() => {
            return res.json({
                erro: false,
                mensagem: "Sensor cadastrado com sucesso!"
            });
        }).catch(() => {
            return res.status(400).json({
                erro: true,
                mensagem: "Erro: Sensor já cadastrado!"
            });
        });

});


app.get('/dados', async function (req, res) {

    const { idDispositivo, dtInicial, dtFinal } = req.query;
    res.json(await getData(idDispositivo, dtInicial, dtFinal));
});



app.listen(port, () => {
    console.info("Aplicação rodando em http://localhost:3000");
});


// async function criarDado(){
//     const dadoCriado = await Dados.create({
//         data_hora: "2022-10-12T16:42:00Z",
//         id_dispositivo: 202210,
//         id_sensor: 333,
//         valor: 500,
//         grandeza: "V"
//       });
//       console.log(dadoCriado);
// }

