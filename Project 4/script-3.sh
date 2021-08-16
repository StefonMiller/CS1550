#!/bin/bash

#WRITE AND READ

# Function called whenever a test is passed. Increments num_tests_passed
pass() {
  echo PASS
}

# Function called whenever a test is failed.
fail() {
  echo FAIL
  exit 1
}

MOUNT=testmount
if [ ! -f "./cs1550" ]; then echo "Compilation Errors"; exit 0; fi

# killall lt-cs1550
# fusermount -u $MOUNT
# rm -rf ${MOUNT}
# mkdir ${MOUNT}
# dd bs=1K count=5K if=/dev/zero of=.disk
# ./cs1550 -d ${MOUNT} >& output-cs1550-3.txt &

declare -i n=0

sleep 3

echo -ne "mkdir ${MOUNT}/dir0"
err=$(mkdir ${MOUNT}/dir0 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi
echo -ne "echo \"asdf\" > ${MOUNT}/dir0/file0.dat"
err=$((echo "asdf" > ${MOUNT}/dir0/file0.dat) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo -ne "cat ${MOUNT}/dir0/file0.dat"
err=$(cat ${MOUNT}/dir0/file0.dat 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

if grep -Fxq "asdf" ${MOUNT}/dir0/file0.dat
then
    echo PASS 0;
else
    fail;
fi

echo -ne "Forbids reading from a directory via error..."
echo -ne "cat ${MOUNT}/dir0"
err=$(cat ${MOUNT}/dir0 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo -ne "echo \"Fairwell, CS1550\" > ${MOUNT}/dir0/file0.dat"
echo -ne "echo \"Fairwell, CS1550\" >> ${MOUNT}/dir0/file0.dat"
echo -ne "echo \"Fairwell, CS1550\" >> ${MOUNT}/dir0/file0.dat"
echo -ne "echo \"Fairwell, CS1550\" >> ${MOUNT}/dir0/file0.dat"

echo "Fairwell, CS1550" > ${MOUNT}/dir0/file0.dat
echo "Fairwell, CS1550" >> ${MOUNT}/dir0/file0.dat
echo "Fairwell, CS1550" >> ${MOUNT}/dir0/file0.dat
err=$((echo "Fairwell, CS1550" >> ${MOUNT}/dir0/file0.dat) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

err=$((cat ${MOUNT}/dir0/file0.dat > output-write.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi


echo "Correctly does reading including files that span multiple disk blocks..."
s=" Fairwell, CS1550."
for((i=0;i<100;i++))
do
  echo  "echo \"$s\" >>${MOUNT}/dir0/file0.dat"
  err=$((echo  "$s" >>${MOUNT}/dir0/file0.dat) 2>&1)
  echo $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done
err=$((cat ${MOUNT}/dir0/file0.dat > output-write2.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi
