import sys

with open('nGL/triangle.inc.h', 'r') as f:
    data = f.read()

old = """                            #else
                                c = loc_texture.bitmap[u.floor() + v.floor()*loc_texture.width];
                            #endif
                            #ifdef BETTER_PERSPECTIVE
                                if(__builtin_expect(q > 0.0f, 1))"""

new = """                            #else
                                c = loc_texture.bitmap[u.floor() + v.floor()*loc_texture.width];
                            #endif
                            
                            unsigned int light_shade = low->c & 0x00FF;
                            if (light_shade > 0 && light_shade != 128) {
                                unsigned int r_shade = (c >> 11) & 0x1F;
                                unsigned int g_shade = (c >> 5) & 0x3F;
                                unsigned int b_shade = c & 0x1F;
                                r_shade = (r_shade * light_shade) >> 7;
                                g_shade = (g_shade * light_shade) >> 7;
                                b_shade = (b_shade * light_shade) >> 7;
                                c = (r_shade << 11) | (g_shade << 5) | b_shade;
                            }

                            #ifdef BETTER_PERSPECTIVE
                                if(__builtin_expect(q > 0.0f, 1))"""

if old in data:
    with open('nGL/triangle.inc.h', 'w') as f:
        f.write(data.replace(old, new))
    print("Patched successfully")
else:
    print("Old string not found")
