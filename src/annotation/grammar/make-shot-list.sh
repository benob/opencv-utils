for show in {.,phase1-dev,phase1-test,phase2-train}/data/{LCP,BFM}*.js
do
	echo $show
	cat $show | grep \.png | cut -f2 -d'"' | awk '{print "'$(dirname $(dirname $show))'/images/"$0}'
done
