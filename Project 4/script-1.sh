#!/bin/bash

#mkdir

# Function called whenever a test is passed. Increments num_tests_passed
pass() {
  echo PASS
}

# Function called whenever a test is failed.
fail() {
  echo FAIL
  exit 1
}

declare -i n=0

MOUNT=testmount


if [ ! -f "./cs1550" ]; then echo "Compilation Errors"; exit 0; fi

sleep 3
ls -al ${MOUNT} | sed 1d | awk '{print $1, $2, $3, $4, $5, $9}' > output-ls1.txt

echo "mkdir ${MOUNT}/123456789"
err=$(mkdir ${MOUNT}/123456789 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

if [ -d "${MOUNT}/123456789" ]; then fail; else echo "PASS 2"; fi


echo -ne "for i in {0..27}; do\n   mkdir ${MOUNT}/dir_i\ndone\n"

for i in {0..27}; do
  echo "mkdir ${MOUNT}/dir_${i}"
  err=$(mkdir ${MOUNT}/dir_${i} 2>&1)
  echo -ne $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done

for i in {0..27}; do
  if [ -d "${MOUNT}/dir_${i}" ]; then let "n++"; fi
done

echo $n

if [[ n -eq 28 ]]
then
   echo "PASS 0";
else
   fail;
fi

echo -ne "for i in {28..50}; do\n   mkdir ${MOUNT}/dir_i\ndone\n"

n=0

for i in {28..50}; do
  echo "mkdir ${MOUNT}/dir_${i}"
  err=$(mkdir ${MOUNT}/dir_${i} 2>&1)
  echo -ne $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done


# mkdir ${MOUNT}/dir_29
# mkdir ${MOUNT}/dir_30

for i in {0..50}; do
  if [ -d "${MOUNT}/dir_${i}" ]; then let "n++"; fi
done

echo $n

if [[ n -lt 40 ]]
then
   echo "PASS 1";
else
   fail;
fi

echo "mkdir ${MOUNT}/dir_1/test"
err=$(mkdir ${MOUNT}/dir_1/test 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

if [ -d "${MOUNT}/dir_1/test" ]; then fail; else echo "PASS 3"; fi
