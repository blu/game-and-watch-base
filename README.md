# Game-and-Watch Screen Tests

A set of performance tests for the LCD screens of the 2020-and-later Game & Watch [mario](https://en.wikipedia.org/wiki/Game_%26_Watch%3A_Super_Mario_Bros.) and [zelda](https://en.wikipedia.org/wiki/The_Legend_of_Zelda_LCD_games#Game_&_Watch:_The_Legend_of_Zelda) handhelds. Goal is to have a tool for evaluating the pixel-switching latencies among primary colors. When high, such latencies usually produce characteristic ghosting effects during quick animation sequences. A phenomenon users of the 1st-gen PSP are too familiar with during 60fps animations.

# What Does It Look Like?

[![single-unit](http://i3.ytimg.com/vi/GHz2PSiBuE0/hqdefault.jpg)](https://youtu.be/GHz2PSiBuE0 "G&W screen tests")
[![side-by-side](http://i3.ytimg.com/vi/xlF22l-_bYo/hqdefault.jpg)](https://youtu.be/xlF22l-_bYo "G&W screen tests, side-by-side")

# How to Build

This project relies on `open-ocd` patched for accessing the full size of the STM32H7B0VBTx internal flash banks; see this [patch by Konrad Beckmann](https://github.com/kbeckmann/ubuntu-openocd-git-builder/blob/master/0001-Extend-bank1-and-enable-bank2-of-STM32H7B0VBTx.patch) and the corresponding helper project for getting [open-ocd built on Ubuntu and MacOS](https://github.com/kbeckmann/ubuntu-openocd-git-builder). Note that while `open-ocd` is built natively for the deploying host using a native toolchain, a cross-compiler cortex-m7 GNU toolchain is also required, which is beyond our scope.

Once a cortex-m7 toolchain and a patched `open-ocd` are provided, carry out this [STM32CubeH7 gist section](https://gist.github.com/blu/00501400ba73c1a010946deb4d92deb7#stm32cubeh7) to provide a suitable `STM32H7 HAL`. Finally, to produce an optimized build:

```bash
$ git clone https://github.com/blu/game-and-watch-base.git
$ cd game-and-watch-base
$ ln -s $STM32CubeH7/Drivers
$ make DEBUG=0 flash
# or if building RAM ELF
$ make DEBUG=0 ram
```

# Usage

	`D-Pad` -- select one of cyan-on-red, magenta-on-green, yellow-on-blue or white-on-black color combinations; not all tests feature all combinations, e.g. Blue-Turtle-shell is just blue
	`A` -- next test
	`B` -- prev test
	`PAUSE` -- pause/unpause animation
	`TIME` -- select animation speed 1x/2x/4x
	`GAME` -- select next model in the mesh tests
