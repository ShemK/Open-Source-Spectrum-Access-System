#!/bin/bash


install_dependencies(){
  echo "Installing REM"

  echo "Installing pip"

  sudo apt-get -y install python-pip

  echo "Python-pip installation complete"

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

  sudo service postgresql restart

  sudo -i -u postgres psql -c"DROP DATABASE IF EXISTS rem;"

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
  sudo apt-get -y install pgadmin3
  fi

  echo "Installing Dependencies - Going to take a long time"

  sudo apt-get install -y libblas-dev liblapack-dev libatlas-base-dev gfortran

  sudo pip install scipy pynmea2 numpy psycopg2 pandas plotly libconf

  sudo chmod -R 755 /usr/local/lib/python2.7/dist-packages/*

  sudo rm -r SAS_Setup
  
  git clone -b rem https://github.com/ShemK/Open-Source-Spectrum-Access-System SAS_Setup
}

install_sensor_component(){
  echo "--------------------------------------------------------------"
  echo "Cloning rem branch from github repository"
  echo "--------------------------------------------------------------"

  #git clone -b rem https://github.com/ShemK/Open-Source-Spectrum-Access-System rem
  cd SAS_Setup

  git checkout rem
  echo "--------------------------------------------------------------"
  echo "Installing GNURadio Dependencies for REM"
  echo "--------------------------------------------------------------"
  #<<COMMENT
  sudo rm -r sensor/gr-sas/build

  mkdir sensor/gr-sas/build

  cd sensor/gr-sas/build

  cmake ..

  make -j8

  sudo make install

  sudo ldconfig

  cd ../../../

  sudo rm -r sensor/gr-utils/build

  mkdir sensor/gr-utils/build

  cd sensor/gr-utils/build

  cmake ..

  make -j8

  sudo make install

  sudo ldconfig

  cd ../../../
}

#COMMENT
install_server(){
  echo "Populating database"

  git checkout rem
  psql -h localhost -U wireless rem < sensor/sql/shem_rem.sql

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

  git checkout sas_php

  sudo rm -r /var/www/html/spectrumAccessSystem

  #sudo rm -r spectrumAccessSystem

  #git clone -b sas_php https://github.com/ShemK/Open-Source-Spectrum-Access-System spectrumAccessSystem

  sudo mv server /var/www/html/spectrumAccessSystem

  sudo chmod 644 /var/www/html/spectrumAccessSystem/*


}


install_gps(){
  echo "--------------------------------------------------------------"
  echo "Downloading and installing GPSD"
  echo "--------------------------------------------------------------"

  #install scons
  sudo apt-get -y install scons

  #cloning gpsd repository
  git clone https://git.savannah.nongnu.org/git/gpsd.git


  #install gpsd
  cd gpsd && sudo scons && sudo scons udev-install

  sudo ldconfig

  cd ..

  sudo chmod -R 755 /usr/lib/python2.7/dist-packages/gps

  # make libconf files accessible
  sudo chmod  755 /usr/local/lib/python2.7/dist-packages/libc*

  ip_address=$(ip -4 addr show eth0 | grep -oP '(?<=inet\s)\d+(\.\d+){3}')

  node_id=$(echo $ip_address | cut -d . -f 4)
  node_id=$(bc -l <<< "$node_id * 100.0")

  #node_id=5000.1
  echo $node_id

  sudo rm -r gpsd
}

install_services(){
  echo "--------------------------------------------------------------"
  echo "Downloading and installing service files"
  echo "--------------------------------------------------------------"

  git checkout rem
  sudo rm -r /opt/sas

  sudo mkdir /opt/sas

  sudo chmod 665 /opt/sas

  sudo cp sensor/apps/top_block.py /opt/sas/

  sudo cp sensor/apps/sas_rem.py /opt/sas/

  sudo chmod 665 /opt/sas/*


  echo "--------------------------------------------------------------"
  echo "Creating fake gps file"
  echo "--------------------------------------------------------------"

  echo "\$GPRMC,125412.48,A,$node_id,N,$node_id,E,00.0,000.0,230506,00,E" > fake.log

  sudo mkdir /opt/sas/gps/

  sudo mv fake.log /opt/sas/gps/

  sudo chmod 665 /opt/sas/gps

  sudo chmod 664 /opt/sas/gps/*

  #sudo rm -r auto_scripts

  #git clone -b auto_scripts https://github.com/ShemK/Open-Source-Spectrum-Access-System auto_scripts

  git checkout auto_scripts

  sudo cp services/channel_analysis.service /etc/systemd/system/

  sudo chmod 664 /etc/systemd/system/channel_analysis.service

  sudo cp services/sdr_phy.service /etc/systemd/system/

  sudo chmod 664 /etc/systemd/system/sdr_phy.service

  sudo cp services/fakegps.service /etc/systemd/system/

  sudo chmod 664 /etc/systemd/system/fakegps.service

  sudo cp services/cbsd_start.service /etc/systemd/system/

  sudo chmod 664 /etc/systemd/system/cbsd_start.service

  sudo systemctl daemon-reload

  sudo systemctl enable sdr_phy.service

  sudo systemctl enable channel_analysis.service

  sudo systemctl enable fakegps.service

  sudo systemctl restart fakegps.service

  sudo cp services/sas_sudoers /etc/sudoers.d/sas

  sudo chmod 440 /etc/sudoers.d/sas

  #cd
}
while getopts dsgve option;
do
  case "$option"
    in
    d)
      install_dependencies
    ;;
    s)
      install_sensor_component
    ;;

    g)
      install_gps
    ;;

    v)
      install_services
    ;;

    e)
      install_server
    ;;
    *)

    ;;
  esac

done

if [ $OPTIND -eq 1 ]; then

  install_dependencies
  install_sensor_component
  install_server
  install_gps
  install_services

fi

# installing the central controllers
#sudo rm -r central_controller

#git clone -b central_controller https://github.com/ShemK/Open-Source-Spectrum-Access-System central_controller

git checkout crts

sudo mkdir /opt/sas/central/

#sudo cp central_controller/controller.py /opt/sas/central/

sudo cp cbsd/config_scripts/controller.py /opt/sas/central/

sudo chmod 665 /opt/sas/central

sudo chmod 665 /opt/sas/central/*

sudo systemctl daemon-reload

git checkout auto_scripts

sudo cp services/cbsd_control.service /etc/systemd/system/

sudo chmod 664 /etc/systemd/system/cbsd_control.service

sudo systemctl daemon-reload

sudo systemctl enable cbsd_control.service

sudo systemctl restart cbsd_control.service

sudo mkdir /opt/sas/aggregator/

#sudo rm -r aggregator

#git clone -b rem_connector https://github.com/ShemK/Open-Source-Spectrum-Access-System aggregator

git checkout rem_connector

cd aggregator

chmod a+x compile.sh

./compile.sh

sudo cp test /opt/sas/aggregator/

sudo cp gps_proxy.py /opt/sas/gps/

sudo chmod 665 /opt/sas/gps

sudo chmod 664 /opt/sas/gps/*


cd ..

sudo chmod -R 665 /opt/sas/aggregator/

git checkout auto_scripts

sudo cp services/aggregator.service /etc/systemd/system/

sudo chmod 664 /etc/systemd/system/aggregator.service

sudo systemctl daemon-reload

sudo systemctl enable aggregator.service

sudo systemctl restart aggregator.service

sudo cp services/gps_proxy.service /etc/systemd/system/

sudo systemctl daemon-reload

sudo systemctl enable gps_proxy.service

sudo systemctl restart fakegps.service
