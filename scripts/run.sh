#!/bin/bash

workload="../workloads/workloada.spec"
dbpath="/tmp/testrocksdb"
../bin/TestYCSBC -db rocksdb -dbpath $dbpath -threads 4 -P $workload -dbstatistics true
