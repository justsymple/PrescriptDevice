# Software Installation
This guide will help you get my software up and running on your LILYGO T-Display S3!

But first, some software will be needed before we can continue.
### Software to install:
- Microsoft Visual Studio Code ([Link](https://code.visualstudio.com/))
- PlatformIO IDE Extension (found in the extension store for VSCode)
- And don't forget to download the source code and extract it for this project! ([Link](https://github.com/justsymple/PrescriptDevice/releases/tag/release)

Once you have everything installed and set up, the installation should be pretty simple.

1. Open the PrescriptDevice folder inside of Visual Studio Code.
2. Plug your T-Display S3 into your PC with a USB-C cable.
3. Ensure that PlatformIO is done setting up, then click the little arrow pointing to the right on the bottom left of the screen.
4. It should transfer the software over to your board and make it work!

And that should be it!

If you would like to modify the software, here is some helpful information for that:
# Software Modification Guide
Two common and simple modifications one might want to make is to the wiring and to the Prescript messages. 

In main.cpp (inside of the src folder), you can easily make any changes both of these!

For wiring modification: at the top of the file, under button vars, simply change the numbers corresponding to each function to your desired GPIO pin number (this should be labeled on your board).

For Prescript message modification: scroll down to around line 230 (in the prescript stuff section) and right there is the PRESCRIPTS_LIST variable where you can change, add, or remove prescripts to your liking!
