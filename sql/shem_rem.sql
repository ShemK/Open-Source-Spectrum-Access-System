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
  PRIMARY KEY ("userId","fccId","cbsdId"),
  UNIQUE ("cbsdId")
);

INSERT INTO blacklisted_cbsds VALUES ('cbd2','cbd562','hask124ba','CB987','A','yap','Nay','{\n        \"latitude\": 37.425056,\n        \"longitude\": -122.084113,\n       \"height\": 9.3,\n        \"heightType\": \"AGL\",\n        \"indoorDeployment\": false,\n        \"antennaAzimuth\": 271,\n        \"antennaDowntilt\": 3,\n       \"antennaGain\": 16,\n        \"antennaBeamwidth\": 30\n      }',NULL,' [\"EUTRA_CARRIER_RSSI_ALWAYS\",\n           \"EUTRA_CARRIER_RSSI_NON_TX\"\n     ]','1237gasd9yfa');

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
  PRIMARY KEY ("userId","fccId","cbsdId"),
  UNIQUE("cbsdId")
);

CREATE OR REPLACE function populate_cbsd_table(LowerFreq FLOAT, cbsdID varchar(45))
RETURNS void as $$
DECLARE
  i INTEGER DEFAULT 1;
  LF FLOAT DEFAULT 1000000000;
  t text;
  sql text;
BEGIN
    LF:= LowerFreq;
    FOR i in 1..15
    LOOP

    EXECUTE format('
      INSERT INTO %s("lowFrequency", "highFrequency") VALUES(%s,%s);',
      'cbsdinfo_'||cbsdID,LF,LF + 10000000);

    i := i+1;
    LF := LF + 10000000;
    END LOOP;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION CREATE_CBSD_CHANNEL_TABLE()
RETURNS trigger AS $$

BEGIN
EXECUTE format('DROP TABLE IF EXISTS %s ;', 'cbsdinfo_'||OLD."cbsdId");
EXECUTE format('
  CREATE TABLE IF NOT EXISTS %s(
    "lowFrequency" float NOT NULL,
    "highFrequency" float NOT NULL,
    available smallint DEFAULT 1,
    "grantId" varchar(45),
    "channelType" channel_type DEFAULT NULL,
    "pu_absent" smallint DEFAULT 1, 
    PRIMARY KEY ("lowFrequency","highFrequency")
  );', 'cbsdinfo_'||NEW."cbsdId");

  perform populate_cbsd_table(400000000.0,NEW."cbsdId");
  perform populate_cbsd_table(800000000.0,NEW."cbsdId");
  perform populate_cbsd_table(1000000000.0,NEW."cbsdId");
  perform populate_cbsd_table(3500000000.0,NEW."cbsdId");

  RETURN NEW;
END
$$ LANGUAGE plpgsql;


DROP TRIGGER cbsd_id_update_trigger ON registered_cbsds;

CREATE TRIGGER cbsd_id_update_trigger
  AFTER UPDATE OF "cbsdId" ON registered_cbsds
  FOR EACH ROW
  EXECUTE PROCEDURE CREATE_CBSD_CHANNEL_TABLE();


INSERT INTO registered_cbsds VALUES ('cbd1','cbd561','hask124ba','CB987','A','yap','Nay','{\n       \"latitude\": 37.425056,\n        \"longitude\": -122.084113,\n       \"height\": 9.3,\n        \"heightType\": \"AGL\",\n        \"indoorDeployment\": false,\n        \"antennaAzimuth\": 271,\n        \"antennaDowntilt\": 3,\n       \"antennaGain\": 16,\n        \"antennaBeamwidth\": 30\n      }',NULL,' [\"EUTRA_CARRIER_RSSI_ALWAYS\",\n           \"EUTRA_CARRIER_RSSI_NON_TX\"\n     ]','1237gasd9yfa',NULL);

DROP TABLE IF EXISTS cbsd_channels;

DROP TYPE IF EXISTS channel_type;
CREATE TYPE channel_type AS ENUM ('PAL','GAA');

CREATE TABLE cbsd_channels (
  "lowFrequency" float NOT NULL,
  "highFrequency" float NOT NULL,
  "available" smallint DEFAULT NULL,
  "grantId" varchar(45) DEFAULT 'UNKNOWN',
  "channelType" channel_type DEFAULT NULL,
  PRIMARY KEY ("lowFrequency","highFrequency")
);


DROP TABLE IF EXISTS grants;

DROP TYPE IF EXISTS grant_category;
CREATE TYPE grant_category AS ENUM ('GRANTED','AUTHORIZED');

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

select populate_cbsd_channels(400000000);
select populate_cbsd_channels(800000000);
select populate_cbsd_channels(1000000000);
select populate_cbsd_channels(3500000000);
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
  Stat INT); /*indicator for node status, deprecated. can be reused for as counter for last known
--Add m-sequence for each node, validate parameters (IP, MAC)*/


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

CREATE OR REPLACE FUNCTION CREATE_NODE_CHANNEL_TABLE()
RETURNS trigger AS $$

BEGIN

EXECUTE format('
  CREATE TABLE IF NOT EXISTS %s(
    channelID serial PRIMARY KEY,
    startfreq FLOAT UNIQUE,
    endfreq FLOAT,
    occ FLOAT DEFAULT NULL
  );', 'channelinfo_'||NEW.nodeID);

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


INSERT INTO NodeInfo(nodeID, nodeType, nodeMAC, nodeIP) VALUES (1, 1, 1,1);

select populate(400000000);
select populate(800000000);
select populate(1000000000);
select populate(3500000000);

INSERT INTO NodeInfo(nodeID, nodeType, nodeMAC, nodeIP) VALUES (1, 1, 1,1);

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
