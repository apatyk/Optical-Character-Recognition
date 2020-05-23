# Optical-Character-Recognition
## Overview
### Optical character recognition software utilizing thinning, branchpoint detection, endpoint detection

In this project, a matched filter (normalized cross-correlation) is used in coordination with thinning as well as branchpoint and endpoint detection algorithms to recognize letters in an image of text. To do so, the 9 x 15 pixel window around each ground truth location that passed as an “e” for a given threshold was copied from the original image. This window was then thresholded at T = 128 and thinned down to a single pixel. Then, based on this thinned version, the number of branchpoints and endpoints were calculated. Various thresholds were tested for accuracy on the normalized MSF image by computing the number of false positives (locations where a letter was detected, but not designated in the ground truth) and true positives (locations where a letter was detected by the program and actually present) taking into account the number of endpoints and branchpoints. These values were used in MATLAB to plot the receiver operating characteristic (ROC) curve and determine the best threshold value.
 
It was found that a threshold value of 199 was the best balance and produced a total of 137 true positives and 44 false positives. Given that there were 151 instances of the letter “e” in the image, 90.7% of them were correctly identified and 44 other letters were falsely detected in the process. Compared to pure template matching with 146 true positives and 69 false positives, it can be seen that checking endpoints and branchpoints can cut down on the number of false positives. However, it also cuts down some on the number of true positives albeit at a lower threshold (199 vs. 206). It appears from the data that not all “e”s were detected due to noise in their pixel windows preventing their endpoint and branchpoint numbers from being accurate.

## Usage

`./ocr [<input_file>.ppm] [<template_file.ppm] [ground_truth.txt] [msf_file.ppm]`

## Test Case

The bundled files can be used as a test case:

| input image | parenthood.ppm |
| template image | parenthood_e_template.ppm |
| ground truth | parenthood_gt.txt |
| matched spatial filtering image | msf_e.ppm |

This test case searches for the letter "e" in the image.
