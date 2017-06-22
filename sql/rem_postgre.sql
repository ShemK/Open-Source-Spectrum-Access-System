
DROP TABLE IF EXISTS SpectrumInfo;;
DROP TABLE IF EXISTS NodeInfo;
DROP TABLE IF EXISTS ChannelStates;
DROP TABLE IF EXISTS ChannelInfo;

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
	channelID serial PRIMARY KEY,
	startfreq FLOAT,
	endfreq FLOAT,
	occ INT DEFAULT NULL
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

CREATE OR REPLACE function populate()
RETURNS void as $$
	DECLARE 
	i INTEGER DEFAULT 1;
	LF FLOAT DEFAULT 960000000;
	t text;
	sql text;
BEGIN
    FOR i in 1..64 
	LOOP
        INSERT INTO ChannelInfo(channelID, startfreq, endfreq) VALUES (i, LF, LF + 312500);
		i := i+1;
		LF := LF + 312500;
    END LOOP;
	    INSERT INTO NodeInfo(nodeID, nodeType, nodeMAC, nodeIP) VALUES (1, 1, 1,1);
END
$$ LANGUAGE plpgsql;




select populate();
