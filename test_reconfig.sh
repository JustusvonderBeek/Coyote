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

print_green() {
    print_color "$1" "$Green"
}

print_dash() {
    print_yellow "----------------------------------------------"
    print_yellow "$1"
    print_yellow "----------------------------------------------"
}

from_nano_to_readable() {
    (( $# )) || { printf '%s\n' 'provide atleast one argument' >&2 ; }
    input="$1"
    withNano="$(( $input % 1000000000 ))"
    withoutNano="$(date -d@"$(( $input / 1000000000 ))" +"%s")"
    echo "${withoutNano}.${withNano}s"
}

print_usage() {
    echo -e "$0 [-i <iteration>] [-a <applications>] [-v <vFPGAs>] [-c <clients>] [-k] [-e] [-h]\n"
    echo -e "-i <iterations>:\tThe number of iterations per client."
    echo -e "-a <applications>:\tThe number of applications per client per iteration."
    echo -e "-v <vFPGAs>:\t\tThe vFPGAs that the schedule manager should manage."
    echo -e "-c <clients>:\t\tThe number of clients to start in parallel."
    echo -e "-k :\t\tIf set running services are killed."
    echo -e "-e :\t\tIf set the service is ended after execution."
    exit 1
}

check_running() {
  print_yellow "Checking if any service is currently running..."

  process=$(pgrep -fl "$1")
    # echo "Out: $process"
    if [[ -z $process ]]; then
        return
    fi
    # Killing the running process(es)
    sudo pkill "$1"

    print_green "All services killed"
    return
}

# Default variables
iteration=1
application=4
vfpga=1
clients=1
kill_running=0
end=0

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
    -k|--kill)
      kill_running=1
      shift #Only shift past argument
      ;;
    -e|--end)
      end=1
      shift #Only shift past argument
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

if [[ $kill_running -eq 1 ]]; then
  check_running "main"
fi

# Expecting the file to be in the main folder!
cd test_scheduling_sw_service

# Starting the service
sudo ./main -i $vfpga

# Start taking time
STARTTIME=$(date +%s%N)

# Used to schedule the clients equally to the vFPGAs
clientVFPGA=0
# Starting the client(s)
for ((i=1 ; i<=$clients ; i++));
do
    echo "Starting client $i on vFPGA $clientVFPGA"
    # Starting in background so that they do not block
    sudo ../test_scheduling_sw_client/main -v $clientVFPGA -i $iteration -n $application &
    clientVFPGA=$((++clientVFPGA % $vfpga))
done

echo "$(jobs -l)"

# Waiting for all clients to finish
wait $(jobs -p)

ENDTIME=$(date +%s%N)

duration=$(from_nano_to_readable $((ENDTIME-STARTTIME)))
print_green "Execution time for all $clients clients: ${duration}"

if [[ $end -eq 1 ]]; then
  check_running "main"
fi

print_dash "Test finished"