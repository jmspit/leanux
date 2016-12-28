#/bin/sh

DIR=$1

for file in $(find "${DIR}" -name '*.hpp' | grep -v '/build/')
do
  sed -i 's;[ \t]*$;;g' $file
  sed -i 's;\t;  ;g' $file
done

for file in $(find "${DIR}" -name '*.cpp' | grep -v '/build/')
do
  sed -i 's;[ \t]*$;;g' $file
  sed -i 's;\t;  ;g' $file
done
