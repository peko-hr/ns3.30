#!/bin/bash
#simulation start seed
echo -n NODE_NUM
read node_num
echo -n $node_num
Seed=10000
#simulation finish seed
echo -n SEED
read input_s_seed
Seed=$input_s_seed
Finish_Seed=10010
while true
do
  echo 'simulation run seed'
  echo $Seed
  Seed=`echo "$Seed+1" | bc`
  if [ $Seed -eq $Finish_Seed ]; then
    exit 0
  fi
      ./waf build
      ./waf --run "scratch/ns2-mobility-trace --traceFile=/home/nsl-ubu/ped2000pas1000.tcl --nodeNum=$node_num --seed=$Seed --duration=301.0 --logFile=ns2-mob.log" 

  
done