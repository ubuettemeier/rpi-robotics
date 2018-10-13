echo "---- go.sh ----"
echo "---- Programm < ../build/test_driver_A4988 > wird gestarttet ----"
echo "---- Core 3 selected ----"
echo "< sudo ionice -c1 chrt -v -r 99 taskset -c 3 ../build/test_driver_A4988 >"
echo ""

# ionice -c1 -n7    set program io scheduling class and priority. 
#                   -c1 for real time
#                   -n7 The scheduling class data. This defines the class data, if the class accepts an argument. 
#                       For real time and best-effort, 0-7 is valid data.
# chrt -v -r 99     manipulate the real-time attributes of a process.
#                   -v Show status information.
#                   -r 99 Set scheduling policy to SCHED_RR with max priority 99
# taskset -c 3      prognam use core
#                   -c 3 core 3

# -- core isolation --
# sudo nano /boot/cmdline.txt

sudo ionice -c1 -n7 chrt -v -r 99 taskset -c 3 ../build/test_driver_A4988
# sudo ionice -c1 -n7 chrt -v -r 99 ../build/test_driver_A4988
# sudo chrt -v -r 99 ../build/test_driver_A4988

# sudo ionice -c1 -n7  ../build/test_driver_A4988
