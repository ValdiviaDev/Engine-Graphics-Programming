# Cotxe11 engine
 
Cotxe11 engine is a model viewer engine used to make various shaders, made by Manel Mourelo, Alfonso Sánchez & David Valdivia for the "Advanced Graphics Programming" subject in our university.

## Instructions
### Navigation

## Implemented shaders
We've implemented the next selection of shaders:

### Grid
The grid in the engine is procedurally generated with a shader that generates an infinite grid.

<img src="docs/grid.png" width="500">

### Background
The background of the engine is printed using a shader that distinguishes the colours bettween the floor and the walls of the engine.

<img src="docs/background.png" width="500">

### Mouse picking
The mouse picking is the process of selecting the object that is picked with the mouse button. This process to pick a certain object is using object identifiers on a texture via a shader.

### Outline shader
An outline that envelops the current selected model.

<img src="docs/no_outline.png" width="400"> <img src="docs/outline.png" width="400">

### Bloom
The bloom shader is a shader that gives brightly lit parts of an object or the scene an intense glowing effect that really accentuates the brightness of the illumination.

<img src="docs/no_bloom.png" width="400"> <img src="docs/bloom.png" width="400">

### SSAO
SSAO, abbreviation of **Screen space ambient occlusion** is a shader that applies ambient occlusion to the scene objects in real time.

<img src="docs/no_ssao.png" width="400"> <img src="docs/ssao.png" width="400">

## Configuration options
<img src="docs/options.png" width="200">
