for show in phase2-dev/data/{LCP,BFM}*.js
do
	echo $show
	cat $show | grep \.png | cut -f2 -d'"' | awk '{print "'$(dirname $(dirname $show))'/images/"$0}'
done
