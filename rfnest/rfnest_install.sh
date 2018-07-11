#!/bin/bash
sudo cp cec.service /etc/systemd/system/
sudo chmod 664 /etc/systemd/system/cec.service
sudo systemctl daemon-reload
sudo systemctl enable cec.service
sudo systemctl start cec.service


sudo cp cec_com.service /etc/systemd/system/
sudo chmod 664 /etc/systemd/system/cec_com.service
sudo systemctl daemon-reload
sudo systemctl enable cec_com.service
