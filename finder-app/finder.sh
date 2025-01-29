#!/bin/sh
print_usage() {
    echo "Usage: $0 <filesdir> <searchstr>"
    echo " <filesdir> Path to the directory to search"
    echo " <searchstr> String to search for in files within the search directory"
}
if [ $# -ne 2 ] || [ ! -d "$1" ]; then
    echo "Error: invalid parameters"
    print_usage
 	exit 1
fi
file_count=$(find "$1" -type f | wc -l)
matching_lines=$(grep -rl "$2" "$1" | wc -l)
echo "The number of files are $file_count and the number of matching lines are $matching_lines"
