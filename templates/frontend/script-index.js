const form = document.querySelector("form");

let usuario;
let senha;
let users;

form.addEventListener("submit", (e)=>{
    e.preventDefault();
    usuario = e.target.usuario.value;
    senha = e.target.senha.value;
    fetch('https://central-iot.herokuapp.com/')
    .then(response => response.json())
    .then(users => checkUser(users));
})

function redirectPage(url){
    window.location.href = url;
}

function checkUser(users){
    let user = users.find(dbuser => dbuser.usuario === usuario && dbuser.senha === senha);
    if (user){
        redirectPage("inicio.html");
        alert("Logado com sucesso!");
    } else {
        alert("Usuário inválido!");
    }
}
