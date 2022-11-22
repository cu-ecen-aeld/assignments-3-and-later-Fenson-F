#!/bin/sh
#By F Fenlon Last Updated Oct 27 - NPC w/ Docker Fixes 1

filesdir=$1
searchstr=$2

if [ $# -ne 2 ]
then
	echo "Error: No path directory and/or search string given"
	exit 1
elif [ ! -d "$filesdir" ] 
then
	echo "Error: $filesdir does not exist"
	exit 1
else
	filesdir=$1
	searchstr=$2
	#echo Searching for "$searchstr" at "$filesdir"
	X=$(find "$filesdir" -type f | wc -l)
	
	Y="$(grep -r -o -i $searchstr $filesdir | wc -l)"

	echo "The number of files are $X and the number of matching lines are $Y"
	exit 0
fi

echo "Error: could complete search"
exit 1 
