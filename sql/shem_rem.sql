DROP TRIGGER IF EXISTS truncateTrigger ON spectrumInfo;
DROP TABLE IF EXISTS SpectrumInfo;
DROP TABLE IF EXISTS ChannelStates;
DROP TABLE IF EXISTS ChannelInfo;
DROP TABLE IF EXISTS truncatetime;
DROP FUNCTION IF EXISTS truncateSpectrumInfo();


/* CBSDS */

DROP TABLE IF EXISTS blacklisted_cbsds;
DROP TABLE IF EXISTS registered_cbsds;

DROP TYPE IF EXISTS cbsd_category;
CREATE TYPE cbsd_category AS ENUM ('A','B');


DROP TYPE IF EXISTS channel_type;
CREATE TYPE channel_type AS ENUM ('PAL','GAA');


DROP TYPE IF EXISTS grant_category;
CREATE TYPE grant_category AS ENUM ('GRANTED','AUTHORIZED','NULL');

CREATE TABLE IF NOT EXISTS blacklisted_cbsds (
  "userId" varchar(19) NOT NULL,
  "fccId" varchar(19) NOT NULL,
  "cbsdSerialNumber" varchar(64) DEFAULT NULL UNIQUE,
  "callSign" varchar(45) DEFAULT NULL,
  "cbsdCategory" cbsd_category NOT NULL,
  "cbsdInfo" text,
  "airInterface" text,
  "installationParam" text,
  "groupingParam" text,
  "measCapability" text,
  "cbsdId" varchar(45) NOT NULL,
  id SERIAL,
  PRIMARY KEY ("userId","fccId","cbsdId"),
  UNIQUE ("cbsdId")
);

CREATE TABLE registered_cbsds (
  "userId" varchar(19) NOT NULL,
  "fccId" varchar(19) NOT NULL,
  "cbsdSerialNumber" varchar(64) DEFAULT NULL,
  "callSign" varchar(45) DEFAULT NULL,
  "cbsdCategory" cbsd_category NOT NULL,
  "cbsdInfo" text,
  "airInterface" text,
  "installationParam" text,
  "groupingParam" text,
  "measCapability" text,
  "cbsdId" varchar(45) NOT NULL,
  "last_active" INT DEFAULT 0,
  id SERIAL,
  PRIMARY KEY ("userId","fccId","cbsdId"),
  UNIQUE("cbsdId"),
  UNIQUE("fccId")
);

CREATE OR REPLACE function populate_cbsd_table(LowerFreq FLOAT, fccID varchar(45))
RETURNS void as $$
DECLARE
  i INTEGER DEFAULT 1;
  LF FLOAT DEFAULT 1000000000;
  t text;
  sql text;
BEGIN
    LF:= LowerFreq;
    FOR i in 1..20
    LOOP

    EXECUTE format('
      INSERT INTO %s("lowFrequency", "highFrequency","grantState","channelType") VALUES(%s,%s,%L,%L);',
      'cbsdinfo_'||fccID,LF,LF + 10000000,'GRANTED','PAL');

    i := i+1;
    LF := LF + 10000000;
    END LOOP;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION CREATE_CBSD_CHANNEL_TABLE()
RETURNS trigger AS $$

BEGIN
EXECUTE format('
  CREATE TABLE IF NOT EXISTS %s(
    "lowFrequency" float NOT NULL,
    "highFrequency" float NOT NULL,
    available smallint DEFAULT 1,
    "grantId" varchar(45),
    "grantExpireTime" int DEFAULT 0,
    "heartBeatInterval" int DEFAULT 0,
    "maxEirp" int DEFAULT NULL,
    "grantState" grant_category,
    "channelType" channel_type,
    "pu_absent" smallint DEFAULT 1,
    PRIMARY KEY ("lowFrequency","highFrequency")
  );', 'cbsdinfo_'||NEW."fccId");

  perform populate_cbsd_table(400000000.0,NEW."fccId");
  perform populate_cbsd_table(800000000.0,NEW."fccId");
  perform populate_cbsd_table(1000000000.0,NEW."fccId");
  perform populate_cbsd_table(3500000000.0,NEW."fccId");

  RETURN NEW;
END
$$ LANGUAGE plpgsql;


DROP TRIGGER cbsd_id_update_trigger ON registered_cbsds;

CREATE TRIGGER cbsd_id_update_trigger
  AFTER INSERT ON registered_cbsds
  FOR EACH ROW
  EXECUTE PROCEDURE CREATE_CBSD_CHANNEL_TABLE();

DROP TABLE IF EXISTS cbsd_channels;

CREATE TABLE cbsd_channels (
  "lowFrequency" float NOT NULL,
  "highFrequency" float NOT NULL,
  "available" smallint DEFAULT NULL,
  "grantId" varchar(45) DEFAULT 'UNKNOWN',
  "channelType" channel_type DEFAULT NULL,
  PRIMARY KEY ("lowFrequency","highFrequency")
);


DROP TABLE IF EXISTS grants;

CREATE TABLE IF NOT EXISTS grants (
  "grantId" varchar(45) NOT NULL,
  "grantExpireTime" int NOT NULL,
  "heartBeatInterval" int NOT NULL,
  "maxEirp" int NOT NULL,
  "grantState" grant_category DEFAULT 'GRANTED',
  PRIMARY KEY ("grantId"),
  UNIQUE ("grantId")
);

CREATE OR REPLACE function populate_cbsd_channels(LowerFreq FLOAT)
RETURNS void as $$
DECLARE
  i INTEGER DEFAULT 1;
  LF FLOAT DEFAULT 1000000000;
  t text;
  sql text;
BEGIN
    LF:= LowerFreq;
    FOR i in 1..20
    LOOP
    INSERT INTO cbsd_channels("lowFrequency", "highFrequency","available","channelType") VALUES (LF, LF + 10000000,1,'PAL');
    i := i+1;
    LF := LF + 10000000;
    END LOOP;
END
$$ LANGUAGE plpgsql;

/* REM */
/*
**
**
*/

DROP TRIGGER IF EXISTS node_id_trigger ON NodeInfo;
DROP FUNCTION IF EXISTS CREATE_NODE_CHANNEL_TABLE();
DROP TABLE IF EXISTS NodeInfo;

CREATE TABLE IF NOT EXISTS NodeInfo(
  nodeID bigserial PRIMARY KEY,
  nodeType INT NOT NULL, /*our network, competing network, incumbent, unknown*/
  nodeMAC VARCHAR(20) NOT NULL,
  nodeIP VARCHAR(20) NOT NULL,
  latitude FLOAT DEFAULT NULL,
  longitude FLOAT DEFAULT NULL, /*last known location*/
  last_active timestamp,  /*last known time of contact*/
  Stat INT,
  id SERIAL); /*indicator for node status, deprecated. can be reused for as counter for last known
--Add m-sequence for each node, validate parameters (IP, MAC)*/



CREATE TABLE IF NOT EXISTS ChannelInfo(
  channelID serial PRIMARY KEY,
  startfreq FLOAT,
  endfreq FLOAT,
  occ FLOAT DEFAULT NULL
); /*dummy indicator for occupancy
--should csv for channel quality matrix be attached to this table?*/


CREATE TABLE IF NOT EXISTS ChannelStates(
  timetag timestamp,
  latitude float,
  longitude float,
  channels float array[64]
);

CREATE TABLE IF NOT EXISTS SpectrumInfo(
	nodeID bigint ,
	channelID INT ,
	timetag timestamp,
	latitude float DEFAULT NULL,
	longitude float DEFAULT NULL, /*check if precision is enough for GPS, else just use separate lat long*/
	psd float[] DEFAULT NULL,
	occ float[64] DEFAULT NULL,
	center_freq float DEFAULT NULL,
	bandwidth float DEFAULT NULL,
	noise_floor float[] DEFAULT NULL,
	FOREIGN KEY (nodeID) REFERENCES NodeInfo (nodeID),
	FOREIGN KEY (channelID) REFERENCES ChannelInfo (channelID)
) ;

CREATE TABLE IF NOT EXISTS truncatetime(
  id INTEGER UNIQUE PRIMARY KEY,
  timetag TIMESTAMP,
  insert_number INT DEFAULT 0
);

CREATE OR REPLACE function populate(LowerFreq FLOAT)
RETURNS void as $$
DECLARE
  i INTEGER DEFAULT 1;
  LF FLOAT DEFAULT 1000000000;
  t text;
  sql text;
BEGIN
    LF:= LowerFreq;
    FOR i in 1..100
    LOOP
    INSERT INTO ChannelInfo(startfreq, endfreq) VALUES (LF, LF + 2000000);
    i := i+1;
    LF := LF + 2000000;
    END LOOP;
END
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION truncateSpectrumInfo()
RETURNS trigger AS $NOTHING$
  DECLARE
  current_time_stamp TIMESTAMP;
  previous_time_stamp TIMESTAMP;
  current_inserts INT;
BEGIN
  SELECT timetag INTO current_time_stamp
  FROM spectrumInfo
  ORDER BY timetag DESC LIMIT 1;

  IF EXISTS (SELECT * FROM truncateTime) THEN

    UPDATE truncateTime
    SET insert_number = insert_number + 1
    WHERE id = 1;


    SELECT insert_number INTO current_inserts
    FROM truncateTime WHERE id=1;

    IF current_inserts > 4000 THEN

      IF EXISTS (SELECT * FROM spectrumInfo) THEN
        SELECT timetag INTO current_time_stamp
        FROM spectrumInfo
        ORDER BY timetag DESC LIMIT 1;

        SELECT timetag INTO previous_time_stamp
        FROM truncateTime
        ORDER BY timetag DESC LIMIT 1;

        /*RAISE NOTICE 'Current Time Stamp Threshold: %', previous_time_stamp;*/
        SELECT previous_time_stamp + (1 * interval '1 minute')
        INTO previous_time_stamp;

      /*  RAISE NOTICE 'Time Stamp Threshold: %', previous_time_stamp;
        RAISE NOTICE 'Time Stamp From spectrumInfo: %', current_time_stamp;
        */
        IF current_time_stamp > previous_time_stamp THEN

          TRUNCATE TABLE truncateTime;
          INSERT INTO truncateTime(id,timetag,insert_number)
          VALUES (1,current_time_stamp,0);
        /*  RAISE NOTICE 'Time Stamp Updated %', current_time_stamp; */
          DELETE FROM spectrumInfo WHERE timetag < current_time_stamp;
          RETURN NEW;
        END IF;
        RETURN NEW;
      END IF;
      RETURN NEW;
    END IF;
    RETURN NEW;
  ELSE
  /*  RAISE NOTICE 'Time Tag: %', current_time_stamp; */
    INSERT INTO truncateTime(id,timetag)
    VALUES (1,current_time_stamp);
    RETURN NEW;
  END IF;

END
$NOTHING$
LANGUAGE plpgsql;

CREATE TRIGGER truncateTrigger
  AFTER INSERT ON spectrumInfo
  FOR EACH ROW
  EXECUTE PROCEDURE truncateSpectrumInfo();



DROP TABLE IF EXISTS SensorCBSDConnection;

CREATE TABLE IF NOT EXISTS SensorCBSDConnection(
  "fccId" varchar(19) NOT NULL,
  nodeID bigserial,
  "distance" FLOAT DEFAULT 99999999,
  "pu_flag" INT DEFAULT 3,
  "pu_possible_distance" FLOAT DEFAULT 9999999,
  FOREIGN KEY (nodeID) REFERENCES NodeInfo (nodeID) ON DELETE CASCADE,
  FOREIGN KEY ("fccId") REFERENCES  registered_cbsds("fccId") ON DELETE CASCADE,
  PRIMARY KEY ("fccId",nodeID)
);

CREATE OR REPLACE FUNCTION CREATE_CBSD_SENSOR_LINK()
RETURNS TRIGGER AS $$

DECLARE
i INTEGER DEFAULT 1;
temp_id VARCHAR(19);
node_id INT;
table_len INTEGER;
temp_table VARCHAR(19);

BEGIN
 /*Check the table the caused the trigger */
 IF TG_TABLE_NAME = 'nodeinfo' THEN
  temp_table = 'registered_cbsds';
 ELSE
  temp_table = 'nodeinfo';
 END IF;


 EXECUTE format('
   SELECT COUNT(*) FROM %s;',
      temp_table) INTO table_len;

 IF table_len > 0 THEN
  FOR i IN 1..table_len
  LOOP

   IF TG_TABLE_NAME = 'nodeinfo' THEN
   EXECUTE format('
      SELECT "fccId" FROM %s WHERE id = %s;',
      temp_table,i) INTO temp_id;

   EXECUTE format('
      INSERT INTO sensorcbsdconnection("fccId", "nodeid") VALUES(%L,%s);',
      temp_id,NEW.nodeID);

   ELSE

   EXECUTE format('
      SELECT nodeID FROM %s WHERE id = %s;',
      temp_table,i) INTO node_id;

   EXECUTE format('
      INSERT INTO sensorcbsdconnection("fccId", "nodeid") VALUES(%L,%s);',
      NEW."fccId",node_id);

   END IF;
  END LOOP;

 end IF;
 RETURN NEW;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION VALID_JSON(str varchar)
returns boolean language plpgsql as $$
declare
    j json;
begin
    j:= str;
    return true;
exception
    when others then return false;
end $$;




DROP FUNCTION IF EXISTS CALCULATE_CBSD_SENSOR_DISTANCE(TEXT,TEXT,TEXT,INT);
CREATE OR REPLACE FUNCTION CALCULATE_CBSD_SENSOR_DISTANCE(TEXT,TEXT,TEXT,INT)
RETURNS TEXT AS $$

DECLARE
  result FLOAT;
  info TEXT;
  cbsd_lat FLOAT;
  cbsd_long FLOAT;
  sensor_lat FLOAT;
  sensor_long FLOAT;
  json_info JSON;
  check12 BOOLEAN;
  cbsd_table TEXT;
  sensor_table TEXT;
BEGIN

  cbsd_table = $1;
  sensor_table = $2;

  EXECUTE FORMAT('
    SELECT "installationParam"
    FROM %s
    WHERE "fccId" = %L
  ',$1,$3) INTO info;


  EXECUTE format('
  SELECT valid_json(%L)',info) INTO check12;

  IF check12 = 't' THEN
   json_info := info;
   info = json_info -> 'latitude';
   cbsd_lat = json_info -> 'latitude';
   cbsd_long = json_info -> 'longitude';

   EXECUTE FORMAT('
     SELECT latitude FROM %s WHERE nodeid = %s;
   ',$2,$4) INTO sensor_lat;

   EXECUTE FORMAT('
     SELECT longitude FROM %s WHERE nodeid = %s;
     ',$2,$4) INTO sensor_long;

     /*https://sciencing.com/convert-gps-coordinates-feet-7695232.html*/
  result = 131332796.6*(acos(cos(cbsd_lat)*cos(cbsd_long)*cos(sensor_lat)*cos(sensor_long) + cos(cbsd_lat)*sin(cbsd_long)*cos(sensor_lat)*sin(sensor_long) + sin(cbsd_lat)*sin(sensor_lat))/360)*0.3048;
  info = result;

  ELSIF check12 = 'f' THEN
   info = 999999999999;
  END IF;

  return info;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION UPDATE_CBSD_SENSOR_DISTANCE()
RETURNS TRIGGER AS $$

DECLARE
  result FLOAT;
  info TEXT;
  node_id INT;
  distance FLOAT;
BEGIN

  FOR node_id IN
  SELECT sensorcbsdconnection.nodeid,sensorcbsdconnection.distance FROM
  sensorcbsdconnection
  WHERE sensorcbsdconnection."fccId" = NEW."fccId"
  LOOP
    distance = CALCULATE_CBSD_SENSOR_DISTANCE('registered_cbsds','NodeInfo',NEW."fccId",node_id);
    EXECUTE FORMAT('
    UPDATE sensorcbsdconnection
    SET distance = %s
    WHERE nodeid = %s AND "fccId" = %L',distance, node_id,NEW."fccId");

  END LOOP;


   RETURN NEW;
END
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS cbsd_sensor_distance_trigger ON registered_cbsds;

CREATE TRIGGER cbsd_sensor_distance_trigger
  AFTER INSERT OR UPDATE ON registered_cbsds
  FOR EACH ROW
  EXECUTE PROCEDURE UPDATE_CBSD_SENSOR_DISTANCE();


CREATE TRIGGER sensor_cbsd_trigger
  AFTER INSERT ON registered_cbsds
  FOR EACH ROW
  EXECUTE PROCEDURE CREATE_CBSD_SENSOR_LINK();

CREATE TRIGGER cbsd_sensor_trigger
  AFTER INSERT ON NodeInfo
  FOR EACH ROW
  EXECUTE PROCEDURE CREATE_CBSD_SENSOR_LINK();




CREATE OR REPLACE function populate_node_table(LowerFreq FLOAT, nodeID BIGINT)
RETURNS void as $$
DECLARE
  i INTEGER DEFAULT 1;
  LF FLOAT DEFAULT 1000000000;
  t text;
  sql text;
BEGIN
    LF:= LowerFreq;
    FOR i in 1..100
    LOOP

    EXECUTE format('
      INSERT INTO %s(startfreq, endfreq) VALUES(%s,%s);',
      'channelinfo_'||nodeID,LF,LF + 2000000);

    i := i+1;
    LF := LF + 2000000;
    END LOOP;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION MAKE_DECISION(TEXT)
RETURNS FLOAT AS $$
DECLARE
temp FLOAT;
fccId TEXT;
nodeid INTEGER;
distance FLOAT;
pu_distance FLOAT;
lowfreq FLOAT;
BEGIN
  fccId = $1;
  CREATE TEMPORARY TABLE decision_table(nodeid BIGINT, lowfrequency FLOAT, pu_distance FLOAT, id SERIAL, PRIMARY KEY (id)) ON COMMIT DROP;
  FOR nodeid,distance IN
  SELECT sensorcbsdconnection.nodeid,sensorcbsdconnection.distance
  FROM sensorcbsdconnection
  WHERE "fccId" = $1 AND pu_flag = 1
  LOOP
    FOR lowfreq, pu_distance IN
    EXECUTE FORMAT('
      SELECT startfreq,nearest FROM channelinfo_%s
      WHERE nearest > %s
      ',nodeid,distance)
    LOOP
      lowfreq = TRUNC((lowfreq/10000000))*10000000;
      INSERT INTO decision_table(nodeid,lowfrequency,pu_distance) VALUES (nodeid,lowfreq,pu_distance);

      EXECUTE FORMAT('
        UPDATE %s
        SET pu_absent = 0
        WHERE "lowFrequency" = %s
        ;','cbsdinfo_'||fccId,lowfreq);
    END LOOP;

  END LOOP;

  /*-----Need to add decision for when multiple sensors see same  pu*/
DROP TABLE IF EXISTS decision_table;
return temp;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION MAXIMUM_DISTANCE_LIMIT()
RETURNS trigger AS $$
DECLARE
fcc_id TEXT;
distance FLOAT;
BEGIN
  FOR fcc_id, distance IN
    SELECT sensorcbsdconnection."fccId", sensorcbsdconnection.distance FROM
    sensorcbsdconnection
    WHERE nodeid = TG_ARGV[0]::INTEGER
  LOOP

    IF distance < NEW.nearest THEN
      EXECUTE FORMAT('
      UPDATE sensorcbsdconnection
      SET pu_flag = 1,
      pu_possible_distance = %s
      WHERE nodeid = %s::INTEGER AND "fccId" = %L',NEW.nearest,CAST(TG_ARGV[0] AS INTEGER),fcc_id);
      PERFORM MAKE_DECISION(fcc_id);
    ELSIF distance < NEW.near THEN
      EXECUTE FORMAT('
      UPDATE sensorcbsdconnection
      SET pu_flag = 2,
      pu_possible_distance = %s
      WHERE nodeid = %s::INTEGER AND "fccId" = %L',NEW.near,CAST(TG_ARGV[0] AS INTEGER),fcc_id);
    ELSIF distance < NEW.furthest THEN
      EXECUTE FORMAT('
      UPDATE sensorcbsdconnection
      SET pu_flag = 3,
      pu_possible_distance = %s
      WHERE nodeid = %s::INTEGER AND "fccId" = %L',NEW.furthest,CAST(TG_ARGV[0] AS INTEGER),fcc_id);
    END IF;
  END LOOP;

RETURN NEW;
END
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION CREATE_NODE_CHANNEL_TABLE()
RETURNS trigger AS $$

BEGIN

  EXECUTE format('
  CREATE TABLE IF NOT EXISTS %s(
    channelID serial PRIMARY KEY,
    startfreq FLOAT UNIQUE,
    endfreq FLOAT,
    occ FLOAT DEFAULT NULL,
    nearest FLOAT DEFAULT 0,
    near FLOAT DEFAULT 0,
    furthest FLOAT DEFAULT 0
  );', 'channelinfo_'||NEW.nodeID);

  EXECUTE format('
    CREATE TRIGGER sensor_detection_trigger_%s
      AFTER INSERT OR UPDATE ON %s
      FOR EACH ROW
        EXECUTE PROCEDURE MAXIMUM_DISTANCE_LIMIT(%s);
  ',NEW.nodeID,'channelinfo_'||NEW.nodeID,NEW.nodeID);
  perform populate_node_table(400000000.0,NEW.nodeID);
  perform populate_node_table(800000000.0,NEW.nodeID);
  perform populate_node_table(1000000000.0,NEW.nodeID);
  perform populate_node_table(3500000000.0,NEW.nodeID);
  RETURN NEW;
END
$$ LANGUAGE plpgsql;

CREATE TRIGGER node_id_trigger
  AFTER INSERT ON nodeInfo
  FOR EACH ROW
  EXECUTE PROCEDURE CREATE_NODE_CHANNEL_TABLE();


DROP TRIGGER delete_node_id_trigger ON NodeInfo;

DROP FUNCTION DELETE_NODE_CHANNEL_TABLE();

CREATE OR REPLACE FUNCTION DELETE_NODE_CHANNEL_TABLE()
RETURNS trigger AS $$
BEGIN

EXECUTE format('DROP TABLE %s ;', 'channelinfo_'||OLD.nodeID);

RETURN NEW;
END
$$ LANGUAGE plpgsql;

CREATE TRIGGER delete_node_id_trigger
  AFTER DELETE ON nodeInfo
  FOR EACH ROW
  EXECUTE PROCEDURE DELETE_NODE_CHANNEL_TABLE();


/* Prepopulate Insertions */

INSERT INTO NodeInfo(nodeID, nodeType, nodeMAC, nodeIP,latitude,longitude) VALUES (1, 1, 1,1,37.425056,-122.084113);

INSERT INTO registered_cbsds VALUES ('cbd1','cbd561','hask124ba','CB987','A','yap','Nay','{"latitude": 37.425056,"longitude": -122.084113,"height": 9.3,"heightType":"AGL","indoorDeployment": false,"antennaAzimuth": 271,"antennaDowntilt": 3,"antennaGain": 16,"antennaBeamwidth": 30}',NULL,' ["EUTRA_CARRIER_RSSI_ALWAYS","EUTRA_CARRIER_RSSI_NON_TX]','1237gasd9yfa',NULL);

INSERT INTO registered_cbsds VALUES ('cbd3','cbd563','hask124ba','CB987','A','yap','Nay','{"latitude": 37.425056,"longitude": -122.084113,"height": 9.3,"heightType":"AGL","indoorDeployment": false,"antennaAzimuth": 271,"antennaDowntilt": 3,"antennaGain": 16,"antennaBeamwidth": 30}',NULL,' ["EUTRA_CARRIER_RSSI_ALWAYS","EUTRA_CARRIER_RSSI_NON_TX]','1zzvasasa',NULL);
INSERT INTO registered_cbsds VALUES ('cbd4','cbd564','hask124ba','CB987','A','yap','Nay','{"latitude": 37.425056,"longitude": -122.084113,"height": 9.3,"heightType":"AGL","indoorDeployment": false,"antennaAzimuth": 271,"antennaDowntilt": 3,"antennaGain": 16,"antennaBeamwidth": 30}',NULL,' ["EUTRA_CARRIER_RSSI_ALWAYS","EUTRA_CARRIER_RSSI_NON_TX]','vzvafqqwa',NULL);
INSERT INTO registered_cbsds VALUES ('cbd5','cbd565','hask124ba','CB987','A','yap','Nay','{"latitude": 37.425056,"longitude": -122.084113,"height": 9.3,"heightType":"AGL","indoorDeployment": false,"antennaAzimuth": 271,"antennaDowntilt": 3,"antennaGain": 16,"antennaBeamwidth": 30}',NULL,' ["EUTRA_CARRIER_RSSI_ALWAYS","EUTRA_CARRIER_RSSI_NON_TX]','qfasfscaa',NULL);

INSERT INTO blacklisted_cbsds VALUES ('cbd2','cbd562','hask124ba','CB987','A','yap','Nay','{"latitude": 37.425056,"longitude": -122.084113,"height": 9.3,"heightType":"AGL","indoorDeployment": false,"antennaAzimuth": 271,"antennaDowntilt": 3,"antennaGain": 16,"antennaBeamwidth": 30}',NULL,' ["EUTRA_CARRIER_RSSI_ALWAYS","EUTRA_CARRIER_RSSI_NON_TX]','qfasfscaa',NULL);

select populate_cbsd_channels(400000000);
select populate_cbsd_channels(800000000);
select populate_cbsd_channels(1000000000);
select populate_cbsd_channels(3500000000);


select populate(400000000);
select populate(800000000);
select populate(1000000000);
select populate(3500000000);
