# A1078802Y Lab 3
Wayne Tzu-Wen Wu

## Summary (of the Drawing):
There are 4 drawings in total as required by the assignment.
1. The first drawing is a simple sphere.
2. The second drawing is a twisted torus which can be defined parametrically.
   The twisted torus features an animated rainbow material color.
3. The third drawing is are trees generated using the L-system.
   Using the L-system plus randomized parameters, different trees can be generated 
   although they all have similar pattern. The leaves are drawn as a circular blob for simplicity.
4. The last drawing is a terrain generator using Perlin Noise. When set to the best
   camera angle, it is as if the terrain is running indefinitely. By pressing "t", the
   trees from drawing #3 will be added on top of the terrain. Use "t" and "T" to reduce or increase
   the density of the trees.

## Primitives Used:
OpenGL: GL_POLYGON, GL_LINE
3D: Sphere, Twisted Torus, Cylinder, Terrain

## Transformation Used
- glTranslatef
- glRotatef
- glScalef
- gluLookAt (Camera)
- gluPerspective (Camera)

## Methods Modified: 
- display()
- keyboard(): extra inputs

## Materials
- Sphere: (Plastic) Red Color + Matted
- Twisted Torus: (Plastic) Rainbow Color + More Shiny
- Leaves: (Organic) Green Color + Moderately Shiny
- Branch: (Organic) Brown Color + Not Shiny
- Terrain: (Organic) Light Green Color + Moderately Shiny

## Coolest Things:
- Animated rainbow material color
- Twisted Torus shading using Finite Difference.
- Generate tree branches using L-System.
- Generate terrain using Perlin Noise with smooth shading.
- Combining the terrain with trees.

## Things to Note:
- The computation for drawing #4 can be very costly (when drawing many trees). Continuous rotation and zooming is NOT recommended.
- The program uses an external header file which is included in the zip file (FastNoise.cpp and FastNoise.h), please include them in the solution.
