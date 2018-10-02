# VGA1306_VT100

Did some more experimenting with different applications of the VGA1306 board over the long weekend... took an Arduino-based VT100 terminal emulator from [here](https://www.youtube.com/watch?v=VEhVO7zY0RI), and refactored from **32 columns by 10 rows** up to **80 columns by 60 rows**. Then had it feed the character buffer into the VGA1306 running VGA **'character-mode'** firmware adapted from [here](https://github.com/shirriff/vga-fpga-fizzbuzz) / [here](https://github.com/milanvidakovic/FPGAComputer)!
