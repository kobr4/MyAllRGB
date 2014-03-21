MyAllRGB
========

All RGB code challenge implementation

The All RGB challenge consists in displaying every rgb space color in one picture. Every color should appear one time.

My algorithm is straight :
* generate palette color
* create seed pixels (at least one)
* repeat the following until no more color available or screen is entirely colored : find a colored pixel, crawl horizontally or vertically to find the first non-colored pixel, color the pixel with the closest available color (using norm 1 distance against the last visited color)

Usage :
=======
Set image generation parameters

Click on "Generate !" button

Wait until the image is fully computed, result should be available as under "output.tga" at the root application directory. 

Requires :
==========
* Apache Portable Runtime (multithreading)
* FLTK (UI toolkit)

win32 build : http://www.nicolasmy.com/blog/uplodad/MyAllRGB-win32.zip

![alt tag](http://www.nicolasmy.com/blog/upload/myallrgb3.jpg)
