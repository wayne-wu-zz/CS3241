# A0178802Y Assignment 1
Wayne Tzu-Wen Wu

## Pictures
![alt text](https://github.com/wayne-wu/doodle/pics/doodle1.png "Normal Mode")
![alt text](https://github.com/wayne-wu/doodle/pics/doodle2.png "Rage Mode")

## Summary (of the Drawing):
This drawing is based on the anime/mange Attack on Titan. 
Specifically, it is the "Titan" form of the main character Eren Jaeger.
It features a close up drawing of the Titan's upper body (mostly face),
as well as transformation from normal mode to a "rage" mode (darker).
The face of the Titan can be rotated, translated and scaled using the keyboard.

## Primitives Used:
- GL_TRIANGLE_FANS
- GL_POLYGON
- GL_LINE_LOOP
- GL_LINE_STRIP

## Transformation Used:
- glTranslatef
- glRotatef
- glScalef 

## Methods modified: 
- display()
- keyboard() for transformation constraints
- added idle() for animation

## Coolest things:
- Polygonal based drawings
- Layering of hair
- Shading with alpha blend
- Animation from normal to rage mode.
- Teeth

## Other Notes:
The picture is drawn mostly using point-to-point mapping. I first used Blender to generate
list of points of my selection from a picture (UV coordinate), and then copied those points 
into OpenGL for drawing. The coordinates were remapped to fit the screen space.


