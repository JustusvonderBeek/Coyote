#!/bin/bash

# Reset
Color_Off='\033[0m'       # Text Reset
ResetC=$Color_Off

# Regular Colors
Black='\033[0;30m'        # Black
Red='\033[0;31m'          # Red
Green='\033[0;32m'        # Green
Yellow='\033[0;33m'       # Yellow
Blue='\033[0;34m'         # Blue
Purple='\033[0;35m'       # Purple
Cyan='\033[0;36m'         # Cyan
White='\033[0;37m'        # White

print_color() {
    prt=$1
    color=$2
    echo -e "${color}${prt}${ResetC}"
}

print_yellow() {
    print_color "$1" "$Yellow"
}

print_dash() {
    print_yellow "----------------------------------------------"
    print_yellow "$1"
    print_yellow "----------------------------------------------"
}

print_usage() {
    echo -e "$0 [-i <iteration>] [-a <applications>] [-v <vFPGAs>] [-c <clients>] [-h]\n"
    echo -e "-i <iterations>:\tThe number of iterations per client."
    echo -e "-a <applications>:\tThe number of applications per client per iteration."
    echo -e "-v <vFPGAs>:\t\tThe vFPGAs that the schedule manager should manage."
    echo -e "-c <clients>:\t\tThe number of clients to start in parallel."
    exit 1
}

# Default variables
iteration=1
application=4
vfpga=0
clients=1

# Parsing the command line
while [[ $# -gt 0 ]]; do
  case $1 in
    -i|--iteration)
      # Parsing happens later
      iteration=$2
      shift # past argument
      shift # past value
      ;;
    -a|--application)
      application=$2
      shift # past argument
      shift # past value
      ;;
    -v|--vfpga)
      vfpga=$2
      shift
      shift
      ;;
    -c|--clients)
      clients=$2
      shift
      shift
      ;;
    -h|--help)
      print_usage
      ;;
    -*|--*)
      echo "Unknown option $1"
      print_usage
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done

print_dash "Starting test"

# Expecting the file to be in the sw_service folder!

cd build_reconfiguration_bench_sw_service

# Starting the service
sudo ./main -i $vfpga

# Starting the client(s)
for ((i=1 ; i<=$clients ; i++));
do
    echo "Starting client $i"
    # Starting in background so that they do not block
    sudo ../build_reconfiguration_bench_sw_client/main -i $iteration -n $application &
done

echo "$(jobs -l)"

# Waiting for all clients to finish
wait $(jobs -p)

print_dash "Test finished"