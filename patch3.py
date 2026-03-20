import sys
import re

with open('blockrenderer.cpp', 'r') as f:
    data = f.read()

helper = """
static COLOR computeLighting(BLOCK_SIDE side, COLOR base_color) {
    if ((base_color & 0x00FF) != 0) return base_color; 
    unsigned int shade = 128;
    switch(side) {
        case BLOCK_TOP:    shade = 119; break;
        case BLOCK_BOTTOM: shade = 64; break;
        case BLOCK_RIGHT:  shade = 91; break;
        case BLOCK_LEFT:   shade = 64; break;
        case BLOCK_BACK:   shade = 80; break;
        case BLOCK_FRONT:  shade = 64; break;
    }
    return (base_color & 0xFF00) | shade;
}

void BlockRenderer::renderNormalBlockSide(int local_x, int local_y, int local_z, const BLOCK_SIDE side, const TextureAtlasEntry &tex, Chunk &c, const COLOR color)
{
    COLOR shaded_color = computeLighting(side, color);
"""
data = data.replace("void BlockRenderer::renderNormalBlockSide(int local_x, int local_y, int local_z, const BLOCK_SIDE side, const TextureAtlasEntry &tex, Chunk &c, const COLOR color)\n{", helper)

target_quad = "void BlockRenderer::renderNormalBlockSideQuad(int local_x, int local_y, int local_z, const BLOCK_SIDE side, const TextureAtlasEntry &tex, Chunk &c, const COLOR color)\n{"
replacement_quad = target_quad + "\n    COLOR shaded_color = computeLighting(side, color);\n"
data = data.replace(target_quad, replacement_quad)

# Replaces addAlignedVertex calls, but be careful of others!
# We find all `c.addAlignedVertex` and `c.addAlignedVertexQuad`
lines = data.split('\n')
inside_func1 = False
inside_func2 = False

new_lines = []
for line in lines:
    if "void BlockRenderer::renderNormalBlockSide(" in line:
        inside_func1 = True
    elif "void BlockRenderer::renderNormalBlockSideQuad(" in line:
        inside_func1 = False
        inside_func2 = True
    elif "void BlockRenderer::renderNormalBlockSideForceColor(" in line:
        inside_func1 = False
        inside_func2 = False

    if inside_func1 or inside_func2:
        if "c.addAlignedVertex" in line and "ForceColor" not in line:
            line = line.replace(", color)", ", shaded_color)")
    new_lines.append(line)

data = '\n'.join(new_lines)


with open('blockrenderer.cpp', 'w') as f:
    f.write(data)
