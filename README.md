# Open-Source-Spectrum-Access-System
Open Source Spectrum Access System

install postgresql, create a database called rem. Create a role called wireless (superuser) and grant it all privileges on database rem. 
DETAILED INSTRUCTIONS ON HOW TO DO THIS WILL BE ADDED SOON, GOOGLE IS YOUR FRIEND FOR NOW.
This link could be useful: 
https://www.digitalocean.com/community/tutorials/how-to-install-and-use-postgresql-on-ubuntu-14-04

install libpqxx3-dev from apt-get

Use the rem_postgre.sql file in /path/to/this/branch/sql to populate the database rem. 
{Cron jobs for maintenance are not activated for this module yet, truncate tables as necessary. 
PSD recording is activated.}


Install gr-sas. Install my OOT module (ahmadsj) gr-utils from the DARPA SC2 gitlab for now, functionalities will be ported over to this module later.


Use the flowgraph in the apps folder as a guildeline. 
- Leave the number of subchannels in the Energy-Detector block as 1, and the number of channels in the psql_insert block as 1. Detection performance will be enhance by using these later.


