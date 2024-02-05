# BMP Image Processor Utility
Command line pipeline utility for transforming BMP image data; written entirely in well-structured C, the top choice for low-level systems programming. 

## Build
```gcc imageprocessor.c -o imageprocessor```

## Quickstart
Program will read image data from the standard input, apply the desired transformation, and then write the transformed image data to the standard output.

Linux:  
```$ cat input_file.bmp | imageprocessor <operation> > output_file.bmp```

Windows:  
```C:> type input_file.bmp | imageprocessor <operation> > output_file.bmp```

Where \<operation\> can be one of:
- __HFLIP__: To horizontally flip the image.
- __VFLIP__: To vertically flip the image.
- __GRAYSCALE__: To convert the image to grayscale.

For example, to horizontally flip an image:

```$ cat myimage.bmp | imageprocessor HFLIP > flippedimage.bmp```

If users wish to chain operations, they can do so using multiple ```imageprocessor``` calls in a pipeline:

```$ cat myimage.bmp | imageprocessor HFLIP | imageprocessor GRAYSCALE flipped_gray_image.bmp```

__Output Data__   
The transformed image data is written to the standard output (stdout), after which the program will exit with a return code or 0. If any errors occur, the error is written to the standard error (stderr) and the program will exit with a return code of 1. 

__INFO Operation__  
```INFO``` is a special \<operation\> that does not output image data and cannot be chained as input in a pipeline. Users will be able to retrieve image information by running the following on the command line:

```$ cat myimage.bmp | imageprocessor INFO```

Which will result in the following information being printed to the standard output:

```
Pixel width: XXX  
Pixel height: XXX  
Bit depth: XXX  
Row width (bytes): XXX  
Bytes per pixel: XXX  
Data offset: XXX 
```

## BMP File Format
BMP files provide a simple, well-structured format for storing file data. Excellent documentation can be found on the relevant wikipedia page [here](https://en.wikipedia.org/wiki/BMP_file_format).

A useful utility to convert other file formats to BMP can be found [here](https://online-converting.com/image/convert2bmp).

Relevant for users of this utilty is the acceptable color depth for input. The following color depths are valid as inputs:
- 32 (True color, RGBA)
- 32 (True color, RGB)
- 24 (True color)
- 16 (5:5:5:1, RGBA Hi color)
- 16 (5:5:5:1, RGB Hi color)
- 16 (5:6:5, RGB Hi color)
- 8 (Indexed)
