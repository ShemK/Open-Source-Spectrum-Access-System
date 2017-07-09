DROP TRIGGER IF EXISTS truncateTrigger ON spectrumInfo;
DROP TABLE IF EXISTS SpectrumInfo;;
DROP TABLE IF EXISTS NodeInfo;
DROP TABLE IF EXISTS ChannelStates;
DROP TABLE IF EXISTS ChannelInfo;
DROP TABLE IF EXISTS truncatetime;
DROP FUNCTION IF EXISTS truncateSpectrumInfo();


/* CBSDS */

DROP TABLE IF EXISTS blacklisted_cbsds;
DROP TYPE IF EXISTS cbsd_category;
CREATE TYPE category AS ENUM ('A','B');
CREATE TABLE blacklisted_cbsds (
  userId varchar(19) NOT NULL,
  fccId varchar(19) NOT NULL,
  cbsdSerialNumber varchar(64) DEFAULT NULL UNIQUE,
  callSign varchar(45) DEFAULT NULL,
  cbsdCategory cbsd_category NOT NULL,
  cbsdInfo text,
  airInterface text,
  installationParam text,
  groupingParam text,
  measCapability text,
  cbsdId varchar(45) NOT NULL,
  PRIMARY KEY (userId,fccId,cbsdId),
  UNIQUE (cbsdId)
);

INSERT INTO blacklisted_cbsds VALUES ('cbd2','cbd562','hask124ba','CB987','A','yap','Nay','{\n				\"latitude\": 37.425056,\n				\"longitude\": -122.084113,\n				\"height\": 9.3,\n				\"heightType\": \"AGL\",\n				\"indoorDeployment\": false,\n				\"antennaAzimuth\": 271,\n				\"antennaDowntilt\": 3,\n				\"antennaGain\": 16,\n				\"antennaBeamwidth\": 30\n			}',NULL,' [\"EUTRA_CARRIER_RSSI_ALWAYS\",\n						\"EUTRA_CARRIER_RSSI_NON_TX\"\n			]','1237gasd9yfa');

DROP TYPE IF EXISTS cbsd_type;
CREATE TYPE cbsd_type AS ENUM ('A','B');

DROP TABLE IF EXISTS cbsd_channels;

CREATE TABLE cbsd_channels (
  lowFrequency float NOT NULL,
  highFrequency float NOT NULL,
  available smallint DEFAULT NULL,
  grantId varchar(45) DEFAULT 'UNKNOWN',
  channelType cbsd_type DEFAULT NULL,
  PRIMARY KEY (lowFrequency,highFrequency)
);


DROP TABLE IF EXISTS grants;

DROP TYPE IF EXISTS grant_category;
CREATE TYPE grant_category AS ENUM ('GRANTED','AUTHORIZED');

CREATE TABLE grants (
  grantId varchar(45) NOT NULL,
  grantExpireTime int NOT NULL,
  heartBeatInterval int NOT NULL,
  maxEirp int NOT NULL,
  grantState grant_category DEFAULT 'GRANTED',
  PRIMARY KEY (grantId),
  UNIQUE (grantId)
); 

DROP TABLE IF EXISTS registered_cbsds;

CREATE TABLE registered_cbsds (
  userId varchar(19) NOT NULL,
  fccId varchar(19) NOT NULL,
  cbsdSerialNumber varchar(64) DEFAULT NULL,
  callSign varchar(45) DEFAULT NULL,
  cbsdCategory cbsd_category NOT NULL,
  cbsdInfo text,
  airInterface text,
  installationParam text,
  groupingParam text,
  measCapability text,
  cbsdId varchar(45) NOT NULL,
  PRIMARY KEY (userId,fccId,cbsdId),
  UNIQUE(cbsdId)
); 

INSERT INTO registered_cbsds VALUES ('cbd1','cbd561','hask124ba','CB987','A','yap','Nay','{\n				\"latitude\": 37.425056,\n				\"longitude\": -122.084113,\n				\"height\": 9.3,\n				\"heightType\": \"AGL\",\n				\"indoorDeployment\": false,\n				\"antennaAzimuth\": 271,\n				\"antennaDowntilt\": 3,\n				\"antennaGain\": 16,\n				\"antennaBeamwidth\": 30\n			}',NULL,' [\"EUTRA_CARRIER_RSSI_ALWAYS\",\n						\"EUTRA_CARRIER_RSSI_NON_TX\"\n			]','1237gasd9yfa');

/* REM */
/*
**
**
*/

CREATE TABLE IF NOT EXISTS NodeInfo(
	nodeID serial PRIMARY KEY,
	nodeType INT NOT NULL, /*our network, competing network, incumbent, unknown*/
	nodeMAC VARCHAR(20) NOT NULL,
	nodeIP VARCHAR(20) NOT NULL,
	loc POINT DEFAULT NULL, /*last known location*/
	last_active timestamp,	/*last known time of contact*/
	Stat INT); /*indicator for node status, deprecated. can be reused for as counter for last known
--Add m-sequence for each node, validate parameters (IP, MAC)*/

CREATE TABLE IF NOT EXISTS ChannelInfo(
	channelID SERIAL PRIMARY KEY,
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
	nodeID INT ,
	channelID INT ,
	timetag timestamp,
	latitude float DEFAULT NULL,
	longitude float DEFAULT NULL, /*check if precision is enough for GPS, else just use separate lat long*/
	psd float[] DEFAULT NULL,
	occ real DEFAULT 1.0,
	center_freq float DEFAULT NULL,
	bandwidth float DEFAULT NULL,
	FOREIGN KEY (nodeID) REFERENCES NodeInfo (nodeID),
	FOREIGN KEY (channelID) REFERENCES ChannelInfo (channelID)
);

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
    FOR i in 1..75
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
