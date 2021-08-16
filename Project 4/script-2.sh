#!/bin/bash

#READDIR

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
# ./cs1550 -d testmount >& output-cs1550-2.txt &

sleep 3

declare -i n=0

echo -ne "for i in {0..9}; do\n   mkdir ${MOUNT}/dir_i\ndone\n"

for i in {0..9}; do
  echo "mkdir ${MOUNT}/dir_${i}"
  err=$((mkdir ${MOUNT}/dir_${i}) 2>&1)
  echo $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done

echo -ne "ls -al ${MOUNT}\n"
err=$((ls -al ${MOUNT} | sed 1d | awk '{print $1, $2, $3, $4, $5, $9}' > output-ls2.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo -ne "for i in {0..9}; do\n   echo -ne \"\" > ${MOUNT}/dir_0/file_i.txt\ndone\n"

for i in {0..9}; do
  echo "echo -ne \"\" > ${MOUNT}/dir_0/file_i.txt"
  err=$((echo -ne "" > ${MOUNT}/dir_0/file_${i}.txt) 2>&1)
  echo $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done

echo -ne "ls -al ${MOUNT}/dir_0\n"
err=$((ls -al ${MOUNT}/dir_0 | sed 1d | awk '{print $1, $2, $3, $4, $5, $9}' > output-ls3.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi
