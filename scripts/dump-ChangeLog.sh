#!/bin/bash
# Generate a ChangeLog with git and bash
# Author: Sébastien Boisvert

firstCommit=$(git log|grep ^commit|tail -n1|awk '{print $2}')

totalNumberOfCommits=$(git log --pretty=oneline|wc -l)


echo HEAD > sortedTags.txt

git log|grep Author|sort |uniq|sed 's/Author: //g' > authors.txt

numberOfAuthors=$(cat authors.txt|wc -l)

for gitTag in $(git tag|grep "\.")
do
	timeStamp=$(git log --pretty=format:%at -1 $gitTag)
	echo "$timeStamp $gitTag"
done|sort -n -r|awk '{print $2}' >> sortedTags.txt

numberOfTags=$(cat sortedTags.txt|wc -l)

echo "$numberOfTags tags, $totalNumberOfCommits commits"
echo ""

for i in $(seq 1 $numberOfTags)
do
	currentTag=$(head -n $i sortedTags.txt|tail -n1)
	tagDate=$(git show $currentTag|grep ^Date|head -n1|sed 's/Date:   //g')
	
	previousTag=$firstCommit
	if test $i -lt $numberOfTags
	then
		previousTag=$(head -n $(($i+1)) sortedTags.txt|tail -n1)
	fi

	currentCommit=$(git show $currentTag|grep ^commit|awk '{print $2}')
	previousCommit=$(git show $previousTag|grep ^commit|awk '{print $2}')

	numberOfCommits=$(git log --pretty=oneline $previousCommit..$currentCommit|wc -l)

	echo "$currentTag        $tagDate        $numberOfCommits commits"
	for author in $(seq 1 $numberOfAuthors)
	do
		authorName=$(head -n $author authors.txt|tail -n1)
		git log --author="$authorName" --pretty=format:"       %h %s" $previousCommit..$currentCommit > commits
		numberOfAuthorCommits=$(cat commits|awk '{print $1}'|wc -w)
		if test $numberOfAuthorCommits -gt 0
		then
			echo ""
			echo "  $authorName      $numberOfAuthorCommits commits"
			echo ""
			cat commits
			echo ""
		fi
	done
	echo ""
done

rm sortedTags.txt
rm authors.txt
