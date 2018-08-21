# fuzzyseg
Fuzzy Image Segmentation, Fast implementation

======================

Usage:

Make

./segmentar image.jpg seedFile.in

seedFile.in is any file that contain one seed per line in this format:

x y class

where class is in [1, 7]

=======================

you can create a seed file using createSeedsFileFromImage:

./createSeedsFileFromImage imageOfSeeds.jpg seedFile.in

this program reads imageOfSeeds.jpg, apply a threshold of 200 for each channel, this info is used to specify its pixel class

for example: (210, 10, 5) is translated to bit sequence 100 (or 4)

(210, 220, 180) is translated to bit sequence 110 (or 6)

if the sequence bit is 000 (or 0), then this pixel is not a seed and is not added to the seed file
