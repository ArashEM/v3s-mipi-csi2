# Resoureses
Here is some useful resources about `v4l2` and `v3s`

## Book
- [Mastering Linux Device Driver Development](https://www.amazon.com/Mastering-Linux-Device-Driver-Development/dp/178934204X) by `John Madieu`

## Slides 
- [Advanced Camera Support Allwinner SoCs mainline linux](https://bootlin.com/pub/conferences/2023/fosdem/kocialkowski-advanced-camera-support-allwinner-socs-mainline-linux/kocialkowski-advanced-camera-support-allwinner-socs-mainline-linux.pdf) by `Paul Kocialkowski`

## Websites 
- [V4l2 and media controller](https://www.marcusfolkesson.se/blog/v4l2-and-media-controller/)
- [STM32MP13 V4L2 camera overview](https://wiki.st.com/stm32mpu/wiki/STM32MP13_V4L2_camera_overview)
- [Allwinner A31/A83T MIPI CSI-2 Support and A31 ISP Support](https://lore.kernel.org/linux-arm-kernel/YgaO8bfP4gKW8BM0@aptenodytes/T/)
- [Initial Allwinner V3 ISP support in mainline Linux](https://bootlin.com/blog/initial-allwinner-v3-isp-support-in-mainline-linux/)
- [v4l2-bayer](https://github.com/paulkocialkowski/v4l2-bayer)
- [Add MIPI D-PHY and MIPI CSI-2 interface nodes](https://patchwork.kernel.org/project/linux-media/patch/20201023174546.504028-11-paul.kocialkowski@bootlin.com/)
- [RGB "Bayer" Color and MicroLenses](http://www.siliconimaging.com/RGB%20Bayer.htm)

# HW information
Based on media topology you can list each `subdev` information via `v4l2-ctl` too.  
Take a log at `v4l2-ctl --help-subdev`

## Camera 
In my setup `ov5647` is accessible via `/dev/v4l-subdev3`.  
Let's paly with it.  

It only supports media bayer `BGGR10` bus format.
```
root@licheepi-zero:~# v4l2-ctl -d /dev/v4l-subdev3 --list-subdev-mbus-code 0
ioctl: VIDIOC_SUBDEV_ENUM_MBUS_CODE (pad=0)
	0x3007: MEDIA_BUS_FMT_SBGGR10_1X10
```

It has limited number of frame size (also frame rates):   
```
root@licheepi-zero:~# v4l2-ctl -d /dev/v4l-subdev3 --list-subdev-framesize pad=0,code=0x3007
ioctl: VIDIOC_SUBDEV_ENUM_FRAME_SIZE (pad=0)
	Size Range: 2592x1944 - 2592x1944
	Size Range: 1920x1080 - 1920x1080
	Size Range: 1296x972 - 1296x972
	Size Range: 640x480 - 640x480
```

And default pixel format:  
```
root@licheepi-zero:~# v4l2-ctl -d /dev/v4l-subdev3 --get-subdev-fmt 0
ioctl: VIDIOC_SUBDEV_G_FMT (pad=0)
	Width/Height      : 640/480
	Mediabus Code     : 0x3007 (MEDIA_BUS_FMT_SBGGR10_1X10)
	Field             : None
	Colorspace        : sRGB
	Transfer Function : Default (maps to sRGB)
	YCbCr/HSV Encoding: Default (maps to ITU-R 601)
	Quantization      : Default (maps to Full Range)

```

## sun6i-isp-proc
ISP can be configured via `sun6i-isp-param` which is accessible through `/dev/video3` (check your topology!).  
It's media format can configured via `/dev/v4l-subdev0`:
```
root@licheepi-zero:~# v4l2-ctl -d /dev/v4l-subdev0 --list-subdev-mbus-code
ioctl: VIDIOC_SUBDEV_ENUM_MBUS_CODE (pad=0)
	0x3001: MEDIA_BUS_FMT_SBGGR8_1X8
	0x3013: MEDIA_BUS_FMT_SGBRG8_1X8
	0x3002: MEDIA_BUS_FMT_SGRBG8_1X8
	0x3014: MEDIA_BUS_FMT_SRGGB8_1X8
	0x3007: MEDIA_BUS_FMT_SBGGR10_1X10
	0x300e: MEDIA_BUS_FMT_SGBRG10_1X10
	0x300a: MEDIA_BUS_FMT_SGRBG10_1X10
	0x300f: MEDIA_BUS_FMT_SRGGB10_1X10
```

# Links
As you may notices to [topology](/pic/media-pipeline.png), `sun6i-csi-bridge` is connected to `sun6i-isp-proc`.  
You can cut this link and connect bridge to `sun6i-csi-capture`.  
This way you can take picture directly via `/dev/video1` without `isp`.  
In order to set new line:
```
media-ctl -d /dev/media1 --reset
media-ctl -d /dev/media1 -l "'sun6i-csi-bridge':1 -> 'sun6i-csi-capture':0[1]"
media-ctl -d /dev/media1 -l "'sun6i-mipi-csi2':1 -> 'sun6i-csi-bridge':0[1]"
```