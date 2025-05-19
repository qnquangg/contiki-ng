PATH_RESULT=results/
python analyze-log-downward.py $PATH_RESULT/mote-output-$1
python ~/sources/contiki-ng/tools/parse_power_consumption.py $PATH_RESULT/power-$1
