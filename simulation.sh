#!/bin/bash
#simulation start seed
echo -n INPUT_PROTOCOL_1=SIGO_0=LSGO
read protocol
echo $protocol
echo -n NODE_NUM
read node_num
echo -n $node_num
echo -n START_SEED
read input_s_seed
echo $input_s_seed
Seed=10000
#simulation finish seed
Seed=$input_s_seed
Finish_Seed=10050
while true
do
  echo 'simulation run seed'
  echo $Seed
  Seed=`echo "$Seed+1" | bc`
  if [ $Seed -eq $Finish_Seed ]; then
    exit 0
  fi
    if [ $protocol -eq 1 ]; then
      ./waf build
      ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=8 --lossModel=4 --scenario=3 --nodes=$node_num --seed=$Seed"
    else
      ./waf build
      ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=6 --lossModel=4 --scenario=3 --nodes=$node_num --seed=$Seed"
    fi
done