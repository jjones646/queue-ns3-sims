#!/bin/bash

kill_all_sims ()
{
    # Get our process group id
    PGID=$(ps -o pgid= $$ | grep -o [0-9]*)
    # Kill it in a new new process group
    setsid kill -- -$PGID

    on_exit 1
}

on_exit ()
{
    popd &>/dev/null

    if [ "$1" ]; then
        exit $1
    else
        exit 0
    fi
}

function run_waf {
    CWD="$PWD"
    cd $NS3DIR >/dev/null
    ./waf --cwd="$CWD" "$@" >> "$CWD"/output.log
    cd - >/dev/null
}

trap kill_all_sims SIGINT
trap on_exit EXIT

# make sure waf is ready to run without any errors
run_waf

# set library path for linker and boost libraries
export LD_LIBRARY_PATH="/usr/local/lib"

# create directory for the results and make it our working directory
DIR_NAME="$(date +%Y_%m_%d_%H_%M_%S)_results"
mkdir "$DIR_NAME"
pushd "$DIR_NAME" &>/dev/null

# how long to run each simulation for
ENDTIME=500

# safe pcap traces?
TRACE_EN="true"

# arrays to iterate over
CONF_NUMS=(1 2 3)
RTT_MS=(1 2 3 4)
DATAR_MS=(1 2 3)

Q_TYPE="RedQueue"
for CONF_NUM in "${CONF_NUMS[@]}"; do
    for RTT_M in "${RTT_MS[@]}"; do
        for DATAR_M in "${DATAR_MS[@]}"; do
            XML_CONFIG="/home/jonathan/Documents/queue-sims/ns3-config${CONF_NUM}.xml"
            WAF_CMD="p2 --endTime=${ENDTIME} --xml=${XML_CONFIG} --trace=${TRACE_EN} --queueType=${Q_TYPE} --rttMultiplier=${RTT_M} --datarateMultiplier=${DATAR_M} --traceFile=${Q_TYPE}-rtt${RTT_M}-data${DATAR_M}-conf${CONF_NUM}"
            run_waf --run "$WAF_CMD" &
        done
    done
    # wait for these to finish before starting up another set of simulations
    wait
done

wait

Q_TYPE="DropTailQueue"
for CONF_NUM in "${CONF_NUMS[@]}"; do
    for RTT_M in "${RTT_MS[@]}"; do
        for DATAR_M in "${DATAR_MS[@]}"; do
            XML_CONFIG="/home/jonathan/Documents/queue-sims/ns3-config${CONF_NUM}.xml"
            WAF_CMD="p2 --endTime=${ENDTIME} --xml=${XML_CONFIG} --trace=${TRACE_EN} --queueType=${Q_TYPE} --rttMultiplier=${RTT_M} --datarateMultiplier=${DATAR_M} --traceFile=${Q_TYPE}-rtt${RTT_M}-data${DATAR_M}-conf${CONF_NUM}"
            run_waf --run "$WAF_CMD" &
        done
    done
    # wait for these to finish before starting up another set of simulations
    wait
done

wait
