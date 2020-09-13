num = $1
if [ $num < 10]; then
  
bazel build //c01:p
../bazel-bin/c01/p01
bazel clean