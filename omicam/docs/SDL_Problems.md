# SDL Problems

## Original problem description
OK, so likely you're aware of what the problem (if not check the build folder for out.bmp or Omicontrol)
Basically it's the whole rendering the white strip, but more so, the fact that the software renderer is
STILL broken and not only that, but it's somehow faster than GPU (I assume because it bypasses all the copying?)

It's not a camera issue because the same thing happens with just a blank colour (via memset) or with a BMP image.
It's also not a hardware issue since it seems to happen with the software renderer (may want to check with
SDL_CreateSoftwareRenderer and the SDL_Surface setup).
So I'm thinking it's a threading issue since you'll observe this executes from the MMAL thread. We can check
by making a new main file which just loads and displays the test BMP image. Otherwise regretfully it's a
raspi driver issue which means we're fucked and have to do it via CPU instead (which may be worth it anyway
since GPU copying takes bloody ages).

## Research from 7/11/19
- Seems to be a problem with the GPU itself
- Problem occurs when using SDL_CreateRenderer in accelerated mode, but not when using SDL_CreateSoftwareRenderer leading me
to believe it's a problem with the GPU hardware.
- Using SDL_CreateRenderer and passing it the flag SDL_RENDERER_SOFTWARE instead of SDL_RENDERER_ACCELERATED 
- In hardware mode, instead of drawing the texture it draws a white rectangle indicating that the texture upload fails.
It also doesn't seem to respond to SDL_SetDrawColour. Software mode works fine (of course).

### Potential issues
- Driver issue (with raspi-config): **CONFIRMED THIS IS NOT THE CASE**
- Recompile SDL (due to it linking against the mesa driver not the Broadcom GLES)?
- Try with HDMI plugged in?