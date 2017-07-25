import psycopg2 as pg
import pandas.io.sql as psql 
import plotly.offline as offline
import plotly.graph_objs as go
from plotly import tools
import math 

connection = pg.connect("dbname = rem user = wireless password = wireless")

df = psql.read_sql("select occ, timetag, center_freq, bandwidth from spectruminfo order by timetag DESC", connection)

print df.shape

print df[df.occ >0.0]
#print df.keys
y =  [1]*df['occ'].shape[0]
#print y
trace1 = go.Heatmap(
    z=df['occ'],
    x=df['center_freq'],
    y=df['timetag'],
    colorscale = 'Viridis',
    
    zmin=0,
    zmax=0.1,
    name =  'Channel State',

)
fig = [trace1]

offline.plot(fig, image='png')