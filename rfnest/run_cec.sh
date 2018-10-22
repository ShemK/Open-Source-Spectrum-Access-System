#!/bin/bash
echo $0
echo "basename: `basename $0`"
echo "dirname: `dirname $0`"
dir_path=`dirname $0`
echo "----------------------------"
echo $dir_path
cd $dir_path
./run_debug.sh > /dev/udp/127.0.0.1/9867
