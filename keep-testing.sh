#!/bin/sh
set -e


main() {
  echo "--> main: $@"
  CLEANUP=cleanup inner "$@"
  echo "<-- main"
}


inner() {
  echo "--> inner: $@"



for i in {1..2000..1}; do set -e; echo $i; ./build/lfqueue-example; done
  echo "<-- inner"
}


cleanup() {
  echo "--> cleanup: $@"
  echo "    RUN_CMD = '$RUN_CMD'"
  echo "    RUN_EXIT_CODE = $RUN_EXIT_CODE"
  sleep 0.3
  echo '<-- cleanup'
  return $RUN_EXIT_CODE
}

main "$@"
