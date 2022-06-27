from flask import Flask, redirect, url_for, render_template
import os

app = Flask(__name__)

@app.route("/")
def home():
    return render_template("index.html")
    #return "Hello! This is the main page of Central Iot project <h1>Welcome to the home page</h1>"

@app.route("/<name>")
def user(name:str = "User"):
    return render_template("user.html", name = name)

@app.route("/admin")
def admin():
    return redirect(url_for("home"))

if __name__ == "__main__":
    app.run()

