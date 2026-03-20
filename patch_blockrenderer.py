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
"""

data = data.replace("UniversalBlockRenderer global_block_renderer;", "UniversalBlockRenderer global_block_renderer;\n" + helper)

target1 = "void BlockRenderer::renderNormalBlockSide(int local_x, int local_y, int local_z, const BLOCK_SIDE side, const TextureAtlasEntry &tex, Chunk &c, const COLOR color)\n{"
replacement1 = target1 + "\n    COLOR shaded_color = computeLighting(side, color);\n"
data = data.replace(target1, replacement1)

target2 = "void BlockRenderer::renderNormalBlockSideQuad(int local_x, int local_y, int local_z, const BLOCK_SIDE side, const TextureAtlasEntry &tex, Chunk &c, const COLOR color)\n{"
replacement2 = target2 + "\n    COLOR shaded_color = computeLighting(side, color);\n"
data = data.replace(target2, replacement2)

data = re.sub(r'c\.addAlignedVertex(.*?), color\);', r'c.addAlignedVertex\1, shaded_color);', data)
data = re.sub(r'c\.addAlignedVertexQuad(.*?), color\);', r'c.addAlignedVertexQuad\1, shaded_color);', data)

with open('blockrenderer.cpp', 'w') as f:
    f.write(data)
