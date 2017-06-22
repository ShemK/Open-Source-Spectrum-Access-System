import psycopg2 as pg
import pandas.io.sql as psql 
import plotly.offline as offline
import plotly.graph_objs as go
from plotly import tools

connection = pg.connect("dbname = rem user = wireless password = wireless")

df = psql.read_sql("SELECT channels, latitude, longitude, timetag from ChannelStates LIMIT 100", connection)

print df.shape

df.rename(columns={0: 'channels', 1:'latitude', 2:'longitude', 3:'timetag'}, inplace = True) 

y =  [1]*df['timetag'].shape[0]

a = df['channels'].values

print a.dtype


