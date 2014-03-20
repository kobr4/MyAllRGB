MyAllRGB
========

All RGB code challenge implementation

The All RGB challenge consists in displaying every rgb space color in one picture. Every color should appear one time.

My algorithm is straight :
* generate palette color
* create seed pixels (at least one)
* find a colored pixel, crawl horizontally or vertically to find the first non-colored pixel, color the pixel with the closest available color (using norm 1 distance against the last visited color)


Requires : 
Apache Portable Runtime (multithreading)
FLTK (UI toolkit)

win32 build : http://www.nicolasmy.com/blog/uplodad/MyAllRGB-win32.zip
