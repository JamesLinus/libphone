Overview
-------------
It's an OpenGL ES 2.0 texture render example using libphone, but you can undoubtedly create a fully fledged RPG game based on this example.  
I learned how to make this example from the tutorial posted by `Ray Wenderlich`, you can find it [here](https://www.raywenderlich.com/3664/opengl-tutorial-for-ios-opengl-es-2-0) and [here](https://www.raywenderlich.com/4404/opengl-es-2-0-for-iphone-tutorial-part-2-textures). It's the best OpenGL ES 2.0 tutorial in the world.  

The texture images used in this example were download from the [flare](http://opengameart.org/content/flare) project created by `Clint Bellanger`, It's a great place if you want to make a MMORPG game and have no idea how to make the game resources.  

[uPNG](https://github.com/elanthis/upng) used in this project to decode png file to RGBA data.

Screenshots
----------------
<img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/rpggame/screenshots/mmorpg.gif" width="240" height="360"/>
<img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/rpggame/screenshots/android.png" width="240" height="360"/>  

How to Run this RPG-game example ?
--------------
[libphone](https://github.com/huxingyi/libphone) is a cross-platform library which supports development iOS and Android apps in C language.    This example is based on libphone.  
Before dive into this example, please follow the [Quick Start](https://github.com/huxingyi/libphone/blob/master/README.md#quick-start) to create a basic helloworld project.  
If you have created a helloworld project successfully, please follow the next steps to run this example.  
- iOS  
  - Replace the `helloworld.c` with `rpggame.c`  
  - Add the following source files to project.  
    ```
    upng.c
    ```
  - Add the following resources to `Copy Bundle Resources`  
    ```
    vertex.glsl
    fragment.glsl
    man_texture.png
    cursed_grave_texture.png
    ```
  - Run and start the journey of MMORPG game deveopment!  
- Android  
 - Replace `/Users/jeremy/Repositories/libphone/samples/helloworld` with `/Users/jeremy/Repositories/libphone/samples/rpggame` inside `build.grade (Module: app)`  
 - Add the following resources to `/assets` folder  
   ```
   vertex.glsl
   fragment.glsl
   man_texture.png
   cursed_grave_texture.png
   ```
 - `Build / Clean Project`  
 - Run and start the journey of MMORPG game deveopment!  

Tips
---------------
- **How to crop the source image**  
  ImageMagick is the best tool to crop or change color of a picture. It's programmers photoshop.   
  ```sh
  $ convert cursed_grave.png -crop 1024x128+1920+0 cursed_grave_texture.png
  $ convert orc_regular_0.png -crop 1024x128+2048+0 man_killed_texture.png
  $ convert orc_regular_0.png -crop 512x128+1536+0 man_blade_texture.png
  $ convert -background transparent -append man_killed_texture.png man_blade_texture.png man_texture.png
  ```
  *Note: convert is part of the ImageMagick command line toolset*

- **Enable OpenGL traces for Android 4.2+**  
  The `Enable OpenGL traces` option sit in the phone's developer options. It's very useful to set it's value as `Call stack on glGetError`.  
