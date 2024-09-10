BASEDIR=$(dirname $0)
export GST_PLUGIN_PATH=${BASEDIR}/../install/gst-plugin
export LD_LIBRARY_PATH=${BASEDIR}/../install/lib

mkdir -p ${BASEDIR}/output
gst-launch-1.0 filesrc location=$1 ! decodebin ! videoconvert !  video/x-raw,format=GRAY8 ! videoconvert ! cviivebackground ! pngenc ! multifilesink location="output/frame%d.png"
