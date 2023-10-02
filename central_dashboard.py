from time import sleep
import pandas as pd
import plotly.express as px

import dash
from dash import dcc
from dash import html
import plotly.express as px
import pandas as pd
from flask import Flask
from sqlalchemy import create_engine
 
app = Flask(__name__)


colors = {
    'background': '#F0F8FF',
    'text': '#00008B'
}
external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']
 
engine = create_engine("")
df = pd.read_sql_query('select * from "dados"',con=engine)
# df = pd.read_csv('download.csv')
 
# Plot the scatterplot using Plotly. We ploy y vs x (#Confirmed vs Date)
fig = px.scatter(df, x='data_hora', y='valor', color='grandeza')
fig.update_traces(mode='markers+lines')
fig.show()

if __name__ == '__main__':
    app = dash.Dash(__name__)
    app.layout = html.Div(children=[
        html.H1(children='Central IoT Dashboard'),
    
        html.Div(children='''
            Entradas de sensores ao longo do tempo:
        '''),
    
        dcc.Graph(
            id='Eng Workshop',
            figure=fig
        )
    ])
    app.run_server(debug=True)