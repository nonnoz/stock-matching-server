#!/bin/bash

make clean
make

start_time=$(date +%s)
for((i = 0; i < 50; i++))
do
{
    ./test test1.xml
    ./test test2.xml  
    ./test test3.xml  
    ./test test4.xml  
    ./test test5.xml  
    ./test test6.xml  
    ./test test7.xml  
    ./test test8.xml    

}
done
end_time=$(date +%s)
cost_time=$[ $end_time-$start_time ]
echo "reply time is $cost_time s"