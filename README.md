# Table of content 
-  Introduciton
-  HW
-  SW
-  Resourses

# Introduction 
This docment is about how to add Raspberry Pi camera `module v1.3` to `lichee pi zero dock` board.  
As you know, `v3s` has `mipi-csi2` interface. All of required drivers are developed and available in mainline Linux kernel (6.12.0).  
Also Rpi camera v1.3 (which is basd on `ov5647`) has working driver in kernel.  
So in this document, we are about to connect these two and take a tiny picture! 
![lichee-camera](pic/lichee-camera.jpg)

# HW
Overall hardware related steps includes 
1. Populate `J8` connector of _Lichee pi zero dock_ with `15 pin FPC, pitch 1mm`
1. Add `i2c` connection to `J8`
1. Create a addapter cable 
1. Hard wire camera `en` pin to `vdd` (It's pin `11` on flat cable)

You may be wondering where all these magics come from!  
It' just because `J8` connector pinout on `Lichee Pi Zero Dock` is not compatible with `Rpi camera`.  
More accouratly, `J8` pinout is **WRONG**.  
This is `J8` (Lichee Pi Zero Dock) pinout:
![lichee-j8](pic/lichee-j8.jpg)

This is `Rpi camera` pinout:  
![rpi-camera-pinout](pic/rpi-camera-pinout.jpg)

After Steps 1 and 2, you may have someting like this.  
Those two wires connects `i2c` pins of camera module to `i2c0` of `v3s`.  
I used pin `11` and `12` of `J8` which were unused.
![j8-mission](pic/add-j8-and-i2c.jpg)

You may noticed that connector I used has **16 pins** :D  
Yes, you're right. I could **not** find `15 pins` flat cable. So I used 16 pin connector and falt.  

Step 3 is about creating adapter cable Which handles wrong connections of `J8`.  
It's somthing like this:  
![adapter-1](pic/adapter-1.jpg)
![adapter-2](pic/adapter-2.jpg)

Step 4 is about hard wiring camera enable pin (pin `11`) to `vdd`.  
I did it because there was no free pin left on `J8` (And I messed up with `pin 16` too!)  
Just keep it simple!
![camera-en](pic/rpi-camera-en-pin.jpg)

Ok ...  
Now were are ready for software part!

