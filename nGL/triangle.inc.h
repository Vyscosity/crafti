//This file will be included in gl.cpp for various different versions
#ifdef TRANSPARENCY
    static void nglDrawTransparentTriangleXZClipped(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
    {
#else
    #ifdef FORCE_COLOR
        static void nglDrawTriangleXZClippedForceColor(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
        {
    #else
        void nglDrawTriangleXZClipped(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
        {
            #ifdef TEXTURE_SUPPORT
                if(!texture)
                    return nglDrawTriangleXZClippedForceColor(low, middle, high);

                if(__builtin_expect((low->c & TEXTURE_TRANSPARENT) == TEXTURE_TRANSPARENT, 0))
                    return nglDrawTransparentTriangleXZClipped(low, middle, high);
            #endif
    #endif
#endif
    if((low->y < GLFix(0) && middle->y < GLFix(0) && high->y < GLFix(0))
        || (low->y >= GLFix(SCREEN_HEIGHT) && middle->y >= GLFix(SCREEN_HEIGHT) && high->y >= GLFix(SCREEN_HEIGHT)))
        return;

    if(middle->y > high->y)
        std::swap(middle, high);

    if(low->y > middle->y)
        std::swap(low, middle);

    if(middle->y > high->y)
        std::swap(middle, high);

    if(high->y < GLFix(0) || low->y >= GLFix(SCREEN_HEIGHT))
        return;

    // The ranges of values from here on allows using some more bits for precision:
    // X is clipped to screen coords, Z is >= CLIP_PLANE and the application won't
    // draw primitives that far away.
    // U and V are bounded to the texture size and R, G and B are between 0 - 1.
    // Only issue is Y, but exceeding the range there is not that likely in practice.
    using TriFix = Fix<10, int32_t>;

    int low_y = low->y, middle_y = middle->y, high_y = high->y + 1;

    const int height_upper = high_y - middle_y;

    const TriFix dx_upper = TriFix(high->x - middle->x) / height_upper;
    const TriFix dz_upper = TriFix(high->z - middle->z) / height_upper;

    const int height_lower = middle_y - low_y + 1;

    const TriFix dx_lower = TriFix(middle->x - low->x) / height_lower;
    const TriFix dz_lower = TriFix(middle->z - low->z) / height_lower;

    const int height_far = high_y - low_y;

    const TriFix dx_far = TriFix(high->x - low->x) / height_far;
    const TriFix dz_far = TriFix(high->z - low->z) / height_far;

    #ifdef TEXTURE_SUPPORT
        #ifdef BETTER_PERSPECTIVE
            const float q_low = 1.0f / low->z.toFloat();
            const float q_middle = 1.0f / middle->z.toFloat();
            const float q_high = 1.0f / high->z.toFloat();

            const float uz_low = low->u.toFloat() * q_low;
            const float vz_low = low->v.toFloat() * q_low;
            const float uz_middle = middle->u.toFloat() * q_middle;
            const float vz_middle = middle->v.toFloat() * q_middle;
            const float uz_high = high->u.toFloat() * q_high;
            const float vz_high = high->v.toFloat() * q_high;

            const float inv_h_upper = 1.0f / float(height_upper);
            const float inv_h_lower = 1.0f / float(height_lower);
            const float inv_h_far = 1.0f / float(height_far);

            const float duz_upper = (uz_high - uz_middle) * inv_h_upper;
            const float dvz_upper = (vz_high - vz_middle) * inv_h_upper;
            const float dq_upper = (q_high - q_middle) * inv_h_upper;

            const float duz_lower = (uz_middle - uz_low) * inv_h_lower;
            const float dvz_lower = (vz_middle - vz_low) * inv_h_lower;
            const float dq_lower = (q_middle - q_low) * inv_h_lower;

            const float duz_far = (uz_high - uz_low) * inv_h_far;
            const float dvz_far = (vz_high - vz_low) * inv_h_far;
            const float dq_far = (q_high - q_low) * inv_h_far;

            float uzstart = uz_low, uzend = uz_low;
            float vzstart = vz_low, vzend = vz_low;
            float qstart = q_low, qend = q_low;
        #else
            const TriFix du_upper = TriFix(high->u - middle->u) / height_upper;
            const TriFix dv_upper = TriFix(high->v - middle->v) / height_upper;

            const TriFix du_lower = TriFix(middle->u - low->u) / height_lower;
            const TriFix dv_lower = TriFix(middle->v - low->v) / height_lower;

            const TriFix du_far = TriFix(high->u - low->u) / height_far;
            const TriFix dv_far = TriFix(high->v - low->v) / height_far;

            TriFix ustart = low->u, uend = low->u;
            TriFix vstart = low->v, vend = low->v;
        #endif
    #elif defined(INTERPOLATE_COLORS)
        const RGB high_rgb = rgbColor(high->c);
        const RGB middle_rgb = rgbColor(middle->c);
        const RGB low_rgb = rgbColor(low->c);

        const TriFix dr_upper = TriFix(high_rgb.r - middle_rgb.r) / height_upper;
        const TriFix dg_upper = TriFix(high_rgb.g - middle_rgb.g) / height_upper;
        const TriFix db_upper = TriFix(high_rgb.b - middle_rgb.b) / height_upper;

        const TriFix dr_lower = TriFix(middle_rgb.r - low_rgb.r) / height_lower;
        const TriFix dg_lower = TriFix(middle_rgb.g - low_rgb.g) / height_lower;
        const TriFix db_lower = TriFix(middle_rgb.b - low_rgb.b) / height_lower;

        const TriFix dr_far = TriFix(high_rgb.r - low_rgb.r) / height_far;
        const TriFix dg_far = TriFix(high_rgb.g - low_rgb.g) / height_far;
        const TriFix db_far = TriFix(high_rgb.b - low_rgb.b) / height_far;

        TriFix rstart = low_rgb.r, rend = low_rgb.r;
        TriFix gstart = low_rgb.g, gend = low_rgb.g;
        TriFix bstart = low_rgb.b, bend = low_rgb.b;
    #endif

    int y = low_y;
    TriFix xstart = low->x, zstart = low->z, xend = low->x, zend = low->z;

    //Vertical clipping
    if(y < 0)
    {
        const int diff = -y;
        int diff_lower = diff;
        int diff_upper = 0;
        if(diff_lower > height_lower)
        {
            diff_lower = height_lower;
            diff_upper = diff - diff_lower;
        }

        y = 0;

        xstart += dx_far * diff;
        zstart += dz_far * diff;
        xend += dx_lower * diff_lower;
        zend += dz_lower * diff_lower;
        xend += dx_upper * diff_upper;
        zend += dz_upper * diff_upper;

        #ifdef TEXTURE_SUPPORT
            #ifdef BETTER_PERSPECTIVE
                uzstart += duz_far * diff;
                vzstart += dvz_far * diff;
                qstart += dq_far * diff;

                uzend += duz_lower * diff_lower;
                vzend += dvz_lower * diff_lower;
                qend += dq_lower * diff_lower;

                uzend += duz_upper * diff_upper;
                vzend += dvz_upper * diff_upper;
                qend += dq_upper * diff_upper;
            #else
                ustart += du_far * diff;
                vstart += dv_far * diff;
                uend += du_lower * diff_lower;
                vend += dv_lower * diff_lower;
                uend += du_upper * diff_upper;
                vend += dv_upper * diff_upper;
            #endif
        #elif defined(INTERPOLATE_COLORS)
            rstart += dr_far * diff;
            gstart += dg_far * diff;
            bstart += db_far * diff;
            rend += dr_lower * diff_lower;
            gend += dg_lower * diff_lower;
            bend += db_lower * diff_lower;
            rend += dr_upper * diff_upper;
            gend += dg_upper * diff_upper;
            bend += db_upper * diff_upper;
        #endif
    }

    if(high_y >= SCREEN_HEIGHT)
        high_y = SCREEN_HEIGHT - 1;

    int pitch = y * SCREEN_WIDTH;
    decltype(z_buffer) z_buf_line = z_buffer + pitch;
    decltype(screen) screen_buf_line = screen + pitch;

    TriFix dx_current = dx_lower, dz_current = dz_lower;
#ifdef TEXTURE_SUPPORT
    #ifdef BETTER_PERSPECTIVE
        float duz_current = duz_lower, dvz_current = dvz_lower, dq_current = dq_lower;
    #else
        TriFix du_current = du_lower, dv_current = dv_lower;
    #endif
#elif defined(INTERPOLATE_COLORS)
    TriFix dr_current = dr_lower, dg_current = dg_lower, db_current = db_lower;
#endif

    if(__builtin_expect(y > middle_y, false))
    {
        dx_current = dx_upper;
        dz_current = dz_upper;

        #ifdef TEXTURE_SUPPORT
            #ifdef BETTER_PERSPECTIVE
                duz_current = duz_upper;
                dvz_current = dvz_upper;
                dq_current = dq_upper;
            #else
                du_current = du_upper;
                dv_current = dv_upper;
            #endif
        #elif defined(INTERPOLATE_COLORS)
            dr_current = dr_upper;
            dg_current = dg_upper;
            db_current = db_upper;
        #endif
    }

    #ifdef TEXTURE_SUPPORT
        //Stack access is faster
        TEXTURE loc_texture = *texture;
    #endif

    //If xstart will get smaller than xend
    if(dx_lower < dx_far)
        goto otherway;

    for(; y <= high_y; y += 1, z_buf_line += SCREEN_WIDTH, screen_buf_line += SCREEN_WIDTH)
    {
        const int x1 = xstart, x2 = xend;
        const int line_width = x2 - x1;
        if(__builtin_expect(line_width >= 1, true))
        {
            const auto inv_l = Fix<16, int32_t>(1) / line_width;
            const TriFix dz = (zend - zstart) * inv_l;
            TriFix z = zstart;

            #ifdef TEXTURE_SUPPORT
                #ifdef BETTER_PERSPECTIVE
                    const float span = float(line_width);
                    const float duz = (uzend - uzstart) / span;
                    const float dvz = (vzend - vzstart) / span;
                    const float dq = (qend - qstart) / span;
                    float uz = uzstart, vz = vzstart, q = qstart;
                #else
                    const TriFix du = (uend - ustart) * inv_l;
                    const TriFix dv = (vend - vstart) * inv_l;
                    TriFix u = ustart, v = vstart;
                #endif
            #elif defined(INTERPOLATE_COLORS)
                const TriFix dr = (rend - rstart) * inv_l;
                const TriFix dg = (gend - gstart) * inv_l;
                const TriFix db = (bend - bstart) * inv_l;

                TriFix r = rstart, g = gstart, b = bstart;
            #endif

            const int draw_x1 = x1 < 0 ? 0 : x1;
            const int draw_x2 = x2 >= SCREEN_WIDTH ? SCREEN_WIDTH - 1 : x2;

            if(__builtin_expect(draw_x1 <= draw_x2, true))
            {
                const int skip = draw_x1 - x1;
                z += dz * skip;

                #ifdef TEXTURE_SUPPORT
                    #ifdef BETTER_PERSPECTIVE
                        uz += duz * skip;
                        vz += dvz * skip;
                        q += dq * skip;
                    #else
                        u += du * skip;
                        v += dv * skip;
                    #endif
                #elif defined(INTERPOLATE_COLORS)
                    r += dr * skip;
                    g += dg * skip;
                    b += db * skip;
                #endif

                decltype(z_buffer) z_buf = z_buf_line + draw_x1;
                decltype(screen) screen_buf = screen_buf_line + draw_x1;
                for(int x = draw_x1; x <= draw_x2; x += 1, ++z_buf, ++screen_buf)
                {
                    if(__builtin_expect(TriFix(*z_buf) > z, true))
                    {
                        #ifdef TEXTURE_SUPPORT
                            COLOR c;
                            #ifdef BETTER_PERSPECTIVE
                                if(__builtin_expect(q > 0.0f, 1))
                                {
                                    const float inv_q = 1.0f / q;
                                    int tex_u = int(uz * inv_q);
                                    int tex_v = int(vz * inv_q);

                                    if(__builtin_expect(tex_u < 0, 0)) tex_u = 0;
                                    else if(__builtin_expect(tex_u >= loc_texture.width, 0)) tex_u = loc_texture.width - 1;

                                    if(__builtin_expect(tex_v < 0, 0)) tex_v = 0;
                                    else if(__builtin_expect(tex_v >= loc_texture.height, 0)) tex_v = loc_texture.height - 1;

                                    c = loc_texture.bitmap[tex_u + tex_v*loc_texture.width];
                                }
                            #else
                                c = loc_texture.bitmap[u.floor() + v.floor()*loc_texture.width];
                            #endif
                            #ifdef BETTER_PERSPECTIVE
                                if(__builtin_expect(q > 0.0f, 1))
                                {
                            #endif
                                    #ifdef TRANSPARENCY
                                        if(__builtin_expect(c != 0x0000, 1))
                                        {
                                            *screen_buf = c;
                                            *z_buf = z;
                                        }
                                    #else
                                *screen_buf = c;
                                *z_buf = z;
                                    #endif
                            #ifdef BETTER_PERSPECTIVE
                                }
                            #endif
                        #elif defined(INTERPOLATE_COLORS)
                            *screen_buf = colorRGB(r, g, b);
                            *z_buf = z;
                        #else
                            *screen_buf = low->c;
                            *z_buf = z;
                        #endif
                    }

                    #ifdef TEXTURE_SUPPORT
                        #ifdef BETTER_PERSPECTIVE
                            uz += duz;
                            vz += dvz;
                            q += dq;
                        #else
                            u += du;
                            v += dv;
                        #endif
                    #elif defined(INTERPOLATE_COLORS)
                        r += dr;
                        g += dg;
                        b += db;
                    #endif

                    z += dz;
                }
            }
        }

        xstart += dx_far;
        zstart += dz_far;

        xend += dx_current;
        zend += dz_current;

        #ifdef TEXTURE_SUPPORT
                #ifdef BETTER_PERSPECTIVE
                    uzstart += duz_far;
                    vzstart += dvz_far;
                    qstart += dq_far;
                    uzend += duz_current;
                    vzend += dvz_current;
                    qend += dq_current;
                #else
                    ustart += du_far;
                    vstart += dv_far;
                    uend += du_current;
                    vend += dv_current;
                #endif
        #elif defined(INTERPOLATE_COLORS)
                rstart += dr_far;
                gstart += dg_far;
                bstart += db_far;

                rend += dr_current;
                gend += dg_current;
                bend += db_current;
        #endif

        if(__builtin_expect(y == middle_y, false))
        {
            dx_current = dx_upper;
            dz_current = dz_upper;

            #ifdef TEXTURE_SUPPORT
                #ifdef BETTER_PERSPECTIVE
                    duz_current = duz_upper;
                    dvz_current = dvz_upper;
                    dq_current = dq_upper;
                #else
                    du_current = du_upper;
                    dv_current = dv_upper;
                #endif
            #elif defined(INTERPOLATE_COLORS)
                dr_current = dr_upper;
                dg_current = dg_upper;
                db_current = db_upper;
            #endif
        }
    }

    return;

    otherway:
    for(; y <= high_y; y += 1, screen_buf_line += SCREEN_WIDTH, z_buf_line += SCREEN_WIDTH)
    {
        const int x1 = xend, x2 = xstart;
        const int line_width = x1 - x2;
        if(__builtin_expect(line_width <= -1, true))
        {
            const auto inv_l = Fix<16, int32_t>(1) / line_width;
            //Here are the differences
            const TriFix dz = (zend - zstart) * inv_l;
            TriFix z = zend;

            #ifdef TEXTURE_SUPPORT
                #ifdef BETTER_PERSPECTIVE
                    const float span = float(line_width);
                    const float duz = (uzend - uzstart) / span;
                    const float dvz = (vzend - vzstart) / span;
                    const float dq = (qend - qstart) / span;
                    float uz = uzend, vz = vzend, q = qend;
                #else
                    const TriFix du = (uend - ustart) * inv_l;
                    const TriFix dv = (vend - vstart) * inv_l;
                    TriFix u = uend, v = vend;
                #endif
            #elif defined(INTERPOLATE_COLORS)
                const TriFix dg = (gend - gstart) * inv_l;
                const TriFix db = (bend - bstart) * inv_l;
                const TriFix dr = (rend - rstart) * inv_l;

                TriFix r = rend, g = gend, b = bend;
            #endif

            const int draw_x1 = x1 < 0 ? 0 : x1;
            const int draw_x2 = x2 >= SCREEN_WIDTH ? SCREEN_WIDTH - 1 : x2;

            if(__builtin_expect(draw_x1 <= draw_x2, true))
            {
                const int skip = draw_x1 - x1;
                z += dz * skip;

                #ifdef TEXTURE_SUPPORT
                    #ifdef BETTER_PERSPECTIVE
                        uz += duz * skip;
                        vz += dvz * skip;
                        q += dq * skip;
                    #else
                        u += du * skip;
                        v += dv * skip;
                    #endif
                #elif defined(INTERPOLATE_COLORS)
                    r += dr * skip;
                    g += dg * skip;
                    b += db * skip;
                #endif

                decltype(z_buffer) z_buf = z_buf_line + draw_x1;
                decltype(screen) screen_buf = screen_buf_line + draw_x1;
                for(int x = draw_x1; x <= draw_x2; x += 1, ++z_buf, ++screen_buf)
                {
                    if(__builtin_expect(TriFix(*z_buf) > z, true))
                    {
                        #ifdef TEXTURE_SUPPORT
                            COLOR c;
                            #ifdef BETTER_PERSPECTIVE
                                if(__builtin_expect(q > 0.0f, 1))
                                {
                                    const float inv_q = 1.0f / q;
                                    int tex_u = int(uz * inv_q);
                                    int tex_v = int(vz * inv_q);

                                    if(__builtin_expect(tex_u < 0, 0)) tex_u = 0;
                                    else if(__builtin_expect(tex_u >= loc_texture.width, 0)) tex_u = loc_texture.width - 1;

                                    if(__builtin_expect(tex_v < 0, 0)) tex_v = 0;
                                    else if(__builtin_expect(tex_v >= loc_texture.height, 0)) tex_v = loc_texture.height - 1;

                                    c = loc_texture.bitmap[tex_u + tex_v*loc_texture.width];
                                }
                            #else
                                c = loc_texture.bitmap[u.floor() + v.floor()*loc_texture.width];
                            #endif
                            #ifdef BETTER_PERSPECTIVE
                                if(__builtin_expect(q > 0.0f, 1))
                                {
                            #endif
                                    #ifdef TRANSPARENCY
                                        if(__builtin_expect(c != 0x0000, 1))
                                        {
                                            *screen_buf = c;
                                            *z_buf = z;
                                        }
                                    #else
                                *screen_buf = c;
                                *z_buf = z;
                                    #endif
                            #ifdef BETTER_PERSPECTIVE
                                }
                            #endif
                        #elif defined(INTERPOLATE_COLORS)
                            *screen_buf = colorRGB(r, g, b);
                            *z_buf = z;
                        #else
                            *screen_buf = low->c;
                            *z_buf = z;
                        #endif
                    }

                    #ifdef TEXTURE_SUPPORT
                        #ifdef BETTER_PERSPECTIVE
                            uz += duz;
                            vz += dvz;
                            q += dq;
                        #else
                            u += du;
                            v += dv;
                        #endif
                    #elif defined(INTERPOLATE_COLORS)
                        r += dr;
                        g += dg;
                        b += db;
                    #endif

                    z += dz;
                }
            }
        }

        xstart += dx_far;
        zstart += dz_far;

        xend += dx_current;
        zend += dz_current;

        #ifdef TEXTURE_SUPPORT
                #ifdef BETTER_PERSPECTIVE
                    uzstart += duz_far;
                    vzstart += dvz_far;
                    qstart += dq_far;
                    uzend += duz_current;
                    vzend += dvz_current;
                    qend += dq_current;
                #else
                    ustart += du_far;
                    vstart += dv_far;
                    uend += du_current;
                    vend += dv_current;
                #endif
        #elif defined(INTERPOLATE_COLORS)
                rstart += dr_far;
                gstart += dg_far;
                bstart += db_far;

                rend += dr_current;
                gend += dg_current;
                bend += db_current;
        #endif

        if(__builtin_expect(y == middle_y, false))
        {
            dx_current = dx_upper;
            dz_current = dz_upper;

            #ifdef TEXTURE_SUPPORT
                #ifdef BETTER_PERSPECTIVE
                    duz_current = duz_upper;
                    dvz_current = dvz_upper;
                    dq_current = dq_upper;
                #else
                    du_current = du_upper;
                    dv_current = dv_upper;
                #endif
            #elif defined(INTERPOLATE_COLORS)
                dr_current = dr_upper;
                dg_current = dg_upper;
                db_current = db_upper;
            #endif
        }
    }
}
