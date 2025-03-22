# Introduction 
As you know `GStreamer` is perfect framework for media streaming.  
Using `v4l2src` you can easily read data from `v4l2` compatible camera and stream it into required pipeline.  
But `sun6i-isp-capture` is not compatible with `v4l2src`.  
Or I couldn't make it work!  

## v4l2src problem
Try this simple pipeline:  
```
gst-launch-1.0  v4l2src device=/dev/video2 \
! video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1,colorimetry=2:0:0:0,interlace-mode=progressive \
! videoconvert \
! fakesink
```
And will get following error:
```
Device '/dev/video2' does not support 2:0:0:0 colorimetry
```
Whatever I did, I couldn't make `v4l2src` to read data from `sun6i-isp-capture`.  
So,  
What now?

# appsrc 
As I described earlier, you can take frame(s) from `/dev/video2` via `v4l2-ctl`.  
Which means `/dev/video2` is working!  
Now I want to read data from it and stream it to my pipeline.  
If `v4l2src` had worked, my pipeline would be something like this:  
```
gst-launch-1.0 v4l2src device=/dev/video2 \
! video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1,
! videoscale \
! video/x-raw,width=800,height=480 \
! videoconvert \
! fbdevsink
``` 

`Gstreamer` has an element called `appsrc`.  
This element lets for create a customized `source` via it's API programming.  
In simply lets you read your raw data from wherever you want.  
Then push it into pipeline buffers. 
This what I did in `main.c`

# Compile
Setup your environment (yocto base).  
Compile with required library and headers of `gstreamer-app-1.0`.  
```
source source /opt/sdk/licheepi-zero/environment-setup-cortexa7t2hf-neon-poky-linux-gnueabi
$CC -Wall -Wextra $(pkg-config --libs --cflags gstreamer-app-1.0) main.c -o main
```

# Resources
1. [capture_raw_frames.c](https://gist.github.com/maxlapshin/1253534)
2. [gst-appsrc.c](https://gist.github.com/floe/e35100f091315b86a5bf)