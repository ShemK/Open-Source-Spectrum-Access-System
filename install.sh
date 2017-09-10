#!/bin/bash

echo "Installing REM"

echo "Installing pip"

sudo apt-get -y install python-pip

echo "Python-pip installation complete"

echo "Installing Dependencies - Going to take a long time"

sudo apt-get install -y libblas-dev liblapack-dev libatlas-base-dev gfortran

sudo pip install scipy pynmea2 numpy psycopg2 pandas plotly

#sudo pip install -U pandas

sudo chmod -R 755 /usr/local/lib/python2.7/dist-packages/*

echo "Dependency installation complete"

echo "Installing PostGre SQL"

sudo apt-get install -y postgresql postgresql-contrib

echo "Creating user - wireless with password wireless"

echo "-------------------------------------------------------"

echo "Password can be changed with the following command:"

echo "sudo -i -u postgres psql -c \"ALTER USER wireless WITH PASSWORD 'wireless';\""

echo "-------------------------------------------------------"

sudo -i -u postgres psql -c"CREATE USER wireless WITH PASSWORD 'wireless';"

sudo -i -u postgres psql -c"ALTER USER wireless WITH SUPERUSER;"

sudo -i -u postgres psql -c"CREATE DATABASE rem;"

echo "Installing libpqxx-dev for C++ psql connector"


if [[ `lsb_release -rs` == "14.04" ]]
then
sudo apt-get -y install libpqxx3-dev
sudo apt-get -y install pgadmin3
fi

if [[ `lsb_release -rs` == "16.04" ]]
then
sudo apt-get -y install libpqxx-dev
fi
echo "--------------------------------------------------------------"
echo "Cloning rem branch from github repository"
echo "--------------------------------------------------------------"

git clone -b rem https://github.com/ShemK/Open-Source-Spectrum-Access-System rem

echo "--------------------------------------------------------------"
echo "Installing GNURadio Dependencies for REM"
echo "--------------------------------------------------------------"

sudo rm -r rem/gr-sas/build

mkdir rem/gr-sas/build

cd rem/gr-sas/build

cmake ..

make -j8

sudo make install

sudo ldconfig

cd ../../../

sudo rm -r rem/gr-utils/build

mkdir rem/gr-utils/build

cd rem/gr-utils/build

cmake ..

make -j8

sudo make install

sudo ldconfig

cd ../../../

echo "---------------------------------------------"
echo "Installing Apache Web Server"
echo "---------------------------------------------"

if [[ `lsb_release -rs` == "14.04" ]]
then
sudo apt-get -y install apache2
sudo apt-get -y install php5
sudo apt-get -y install libapache2-mod-php5
sudo apt-get -y install php5-pgsql
sudo service apache2 restart
fi

echo "--------------------------------------------------------------"
echo "Downloading Server Files"
echo "--------------------------------------------------------------"

git clone -b sas_php https://github.com/ShemK/Open-Source-Spectrum-Access-System spectrumAccessSystem

sudo mv spectrumAccessSystem /var/www/html/spectrumAccessSystem

sudo chmod 644 /var/www/html/spectrumAccessSystem/*

echo "--------------------------------------------------------------"
echo "Downloading and installing service files"
echo "--------------------------------------------------------------"


sudo rm -r /opt/sas

sudo mkdir /opt/sas

sudo chmod 665 /opt/sas

sudo cp rem/apps/top_block.py /opt/sas/

sudo cp rem/apps/sas_rem.py /opt/sas/

sudo chmod 665 /opt/sas/*

git clone -b auto_scripts https://github.com/ShemK/Open-Source-Spectrum-Access-System auto_scripts

sudo cp auto_scripts/channel_analysis.service /etc/systemd/system/

sudo chmod 664 /etc/systemd/system/channel_analysis.service

sudo cp auto_scripts/sdr_phy.service /etc/systemd/system/

sudo chmod 664 /etc/systemd/system/sdr_phy.service

sudo systemctl daemon-reload

sudo systemctl enable channel_analysis.service
