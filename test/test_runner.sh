#!/usr/bin/bash

green_c='\033[1;32m'
blue_c='\033[1;34m'
red_c='\033[1;31m'
no_c='\033[0m'

runner=test_runner
results_file=test.out

function run_test {
  
  local success
  
  if [ -n "$2" ]; then
    valgrind --leak-check=full --error-exitcode=2 ./$runner \
      ${1/%.out/.jarl} 1>$1 2>${1/%.out/.grind}
  else
    ./$runner ${1/%.out/.jarl} 1>$1 2>${1/%.out/.grind}
  fi
  case $? in
  0)
    echo -e "Success!" >>$1
    success=true
    ;;
  1)
    echo -e "Failed!" >>$1
    ;;
  *)
    echo "Valgrind caught errors." >>$1
    cat ${1/%.out/.grind} >>$1
    echo -e "Failed!" >>$1
    ;;
  esac
  rm ${1/%.out/.grind}
  
  if [ -z $success ]
  then local format="%s:\n$red_c%s$no_c\n"
  else local format="%s:\n$green_c%s$no_c\n"
  fi
  printf $format "${1/%.out/}" "$(cat $1)"
}

function compile_test_results {
  local tests=($(ls *.jarl))
  tests=${tests[*]/%.jarl/.out}
  
  local good=0
  local bad=0
  local new_good=0
  local new_bad=0
  local is_new=0
  
  for test in $tests; do
    if [ -e $test ]; then
      if [ $test -nt $results_file ]
      then is_new=1
      else is_new=0
      fi
      local line=$(cat $test | tail -n 1)
      if [ $line == "Success!" ]; then
        let good=good+1
        let new_good=new_good+1*is_new
      else
        let bad=bad+1
        let new_bad=new_bad+1*is_new
      fi
      echo "$test: $line" >>"$results_file.temp"
    fi
  done
  
  echo "tests: $((good+bad)), success: $good, fail: $bad" >>"$results_file.temp"
  mv "$results_file.temp" $results_file
  
  if [ "$1" == "new_tests" ]; then
    if [ $new_bad -ne 0 ]
    then echo -ne $red_c
    else echo -ne $blue_c
    fi
    echo -e "tests: $((new_good+new_bad)), success: $new_good, fail: $new_bad"
  else
    if [ $bad -ne 0 ]
    then echo -ne $red_c
    else echo -ne $blue_c
    fi
    echo -e "tests: $((good+bad)), success: $good, fail: $bad"
  fi
  echo -ne $no_c
}

if [ "$1" == "" ];
then compile_test_results
elif [ "$1" == "new_tests" ];
then compile_test_results new_tests
else
  run_test $1 $2
fi
