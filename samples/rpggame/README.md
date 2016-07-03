Overview
-------------
It's an OpenGL ES 2.0 texture render example using libphone, but you can undoubtedly create a full fledged RPG game based on this example.  
I learned how to make this example from the tutorial posted by `Ray Wenderlich`, you can find it [here](https://www.raywenderlich.com/3664/opengl-tutorial-for-ios-opengl-es-2-0) and [here](https://www.raywenderlich.com/4404/opengl-es-2-0-for-iphone-tutorial-part-2-textures). It's the best OpenGL ES 2.0 tutorial in the world.  

The texture images used in this example were download from the [flare](http://opengameart.org/content/flare) project created by `Clint Bellanger`, It's a great place if you want make a MMORPG game and have no idea how to make the game resource.  

[uPNG](https://github.com/elanthis/upng) used in this project to decode png file to RGBA data.

Tips
---------------
- **How to crop the source image**  
ImageMagick is the best tool to crop or change color of a picture. It's programmers photoshop.   
```sh
$ convert cursed_grave.png -crop 1024x128+1920+0 cursed_grave_textrue.png
```
*Note: convert is part of the ImageMagick command line toolset*
