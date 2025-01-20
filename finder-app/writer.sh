#!/bin/bash
print_usage() {
    echo "Usage: $0 <writefile> <writestr>"
    echo " <writefile> Path to the file to write"
    echo " <writestr> String to write to the file"
}
if [ $# -ne 2 ]; then
    echo "Error: invalid parameters"
    print_usage
 	exit 1
fi
mkdir -p "$(dirname "$1")"
echo "$2" > "$1"

if [ $? -ne 0 ]; then
    echo "Error: unable to write to file $1"
    exit 1
fi
