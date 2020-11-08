#! /bin/sh


PIPE='udpsrc port=5000 caps="application/x-rtp" ! rtph264depay ! avdec_h264 ! appsink'

	
~/Desktop/openvslam/build/run_video_slam -v ./orb_vocab.dbow2 \
    -m "$PIPE" \
    -c ./config.yaml \
    --map-db map.msg



