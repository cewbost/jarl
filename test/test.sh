#!/usr/bin/bash

runner=test_runner
results_file=test.out

function run_test {
  echo "Testing $1"
  
  valgrind --leak-check=full --error-exitcode=2 ./$runner \
    ${1/%.out/.jarl} 1>$1 2>${1/%.out/.grind}
  case $? in
  0)
    echo "Success!" >>$1
    ;;
  1)
    echo "Failed!" >>$1
    ;;
  2)
    echo "Valgrind caught errors." >>$1
    cat ${1/%.out/.grind} >>$1
    echo "Failed!" >>$1
    ;;
  esac
  rm ${1/%.out/.grind}
  cat $1
}

function compile_test_results {
  tests=($(ls *.jarl))
  tests=${tests[*]/%.jarl/.out}
  
  good=0
  bad=0
  new_good=0
  new_bad=0
  
  for test in $tests; do
    if [ -e $test ]; then
      if [ $test -nt $results_file ]
      then is_new=1
      else is_new=0
      fi
      line=$(cat $test | tail -n 1)
      if [ $line = "Success!" ]; then
        let good=good+1
        let new_good=new_good+1*is_new
      else
        let bad=bad+1
        let new_bad=new_bad+1*is_new
      fi
      echo "$test: $line" >>"$results_file.temp"
    fi
  done
  
  final_line="tests: $((good+bad)), success: $good, fail: $bad"
  echo $final_line >>"$results_file.temp"
  mv "$results_file.temp" $results_file
  
  if [ "$1" = "new_tests" ]
  then echo "tests: $((new_good+new_bad)), success: $new_good, fail: $new_bad"
  else echo $final_line
  fi
}

if [ "$1" = "" ];
then compile_test_results
elif [ "$1" = "new_tests" ];
then compile_test_results new_tests
else
  while [ "$1" != "" ]; do
    run_test $1
    shift
  done
fi
