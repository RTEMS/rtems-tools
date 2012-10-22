#! /bin/sh
#
# Make an application using the object files in the RTL application.
# Please build the RTL application first.
#

if [ $# -ne 2 ]; then
  echo "error: bad arguments: ./mkapp <path to RTL app> <path to C compiler>"
  exit 2
fi

if [ ! -d $1 ]; then
  echo "error: not a directory: $1"
  exit 3
fi

if [ ! -f $2 ]; then
  echo "error: not a file: $2"
  exit 3
fi

o1="$1/xa.c.1.o"
o2="$1/x-long-name-to-create-gnu-extension-in-archive.c.1.o"

if [ ! -e ${o1} ]; then
  echo "error: cannot find: ${o1}"
  exit 4
fi

if [ ! -e ${o2} ]; then
  echo "error: cannot find: ${o2}"
  exit 4
fi

case $(uname -s) in
  Darwin) platform="darwin" ;;
  Linux)  platform="linux2" ;;
  WIN32)  platform="win32" ;;
  *)
    echo "error: unsupported platform"
    exit 5
esac

echo "./build-${platform}/rtems-ld -S --base $1/rtld --cc $2 -o rtl-app.rap ${o1} ${o2}"
./build-${platform}/rtems-ld -S --base $1/rtld --cc $2 -o rtl-app.rap ${o1} ${o2}

exit 0
