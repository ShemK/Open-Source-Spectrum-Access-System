import psycopg2 as pg
import pandas.io.sql as psql
from pandas import DataFrame
from pandas import concat
import pandas as pd
import numpy as np
import json

class DbReader():
    def __init__(self, dbname, host, user, password):
        self.dbname = dbname
        self.host = host
        self.user = user
        self.password = password
        self.conn = None

    def connect(self):
        try:
            self.conn =  pg.connect(dbname = self.dbname ,host = self.host , user = self.user , password = self.password)
            return self.conn;
        except Exception as e:
            self.conn = None
            print e
            return self.conn

    # Create a list of columns to fetch and the name of the table
    # create a dictionary for the conditions
    def fetch_data(self,columns,table,conditions = None, extra_conditions = None):
        if self.conn != None:
            sql_req = "SELECT "
            i = 0
            if columns != None:
                while i < len(columns):
                    if i < (len(columns)-1):
                        sql_req = sql_req + '"' + columns[i] + '",'
                    else:
                        sql_req = sql_req + '"' + columns[i] + '" '
                    i = i+1
            else:
                sql_req = sql_req + "* "

            sql_req = sql_req + " FROM "
            if not isinstance(table, (list, tuple)):
                sql_req = sql_req + table
            else:
                i = 0
                while i < len(table):
                    if i < (len(table)-1):
                        sql_req = sql_req + table[i] + " JOIN "
                    else:
                        sql_req = sql_req + table[i] + " "
                    i = i+1
            if conditions!=None:
                sql_req = sql_req + " WHERE "
                for key,value in conditions.items():
                    sql_req = sql_req + ' "' + str(key) + '" '+ str(value) + " AND "

                sql_req = sql_req[:-4]

            if extra_conditions!= None:
                sql_req = sql_req + extra_conditions

            sql_req = sql_req + ";"
            try:
                f = psql.read_sql(sql_req, self.conn)
                return f
            except Exception as e:
                print "Wrong sql statement: ", sql_req
                print e
                return None;



    def update_data(self,columns,table,conditions = None):
        if self.conn != None:
            sql_req = "UPDATE " + table + " SET "

            i = 0

            for key,value in columns.items():
                sql_req = sql_req + ' "' + str(key) + '" = '+ str(value) + ","

            sql_req = sql_req[:-1]
            if conditions!=None:
                sql_req = sql_req + " WHERE "
                for key,value in conditions.items():
                    sql_req = sql_req + ' "' + str(key) + '" = '+ str(value) + " AND "

                sql_req = sql_req[:-4]

            sql_req = sql_req + ";"

            #print sql_req
            try:
                #f = psql.read_sql(sql_req, self.conn)
                cur = self.conn.cursor()
                cur.execute(sql_req)
                f = self.conn.commit()
                return f
            except Exception as e:
                print "Wrong sql statement: ", sql_req
                print e
                return None;

def main():
    reader = DbReader("rem","localhost","wireless","wireless")
    reader.connect()
    conditions = {'id':'> 2'}
    f = reader.fetch_data(None,'registered_cbsds',conditions)
    print f
    #f = reader.update_data({'id':'2'},'registered_cbsds',conditions)
    #print f

if __name__ == '__main__':
    main()
