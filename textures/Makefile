OBJS := font_bmp.h font_dat.h inventory.h inventory2.h crafting_table.h furnace.h inv_selection.h loading.h loadingtext.h menu.h selection.h terrain.h terrain2.h blockselection.h part_fire.h items_texture.h steve.h chicken.h creeper.h

all: $(OBJS)

%.h: %.bmp
	ConvertImg --format=ngl $^ $@

# Prefer ConvertImg when installed; else RGB565 emitter from creeper.png
creeper.h: creeper.png gen_creeper_h.py
	@command -v ConvertImg >/dev/null 2>&1 && ConvertImg --format=ngl creeper.png creeper.h || python3 gen_creeper_h.py

%.h: %.png
	ConvertImg --format=ngl $^ $@

items_texture.h: items.png
	ConvertImg --format=ngl $^ $@

inventory.png: gui2.png
	cp $^ $@

inv_selection.png: gui.png
	convert -scale 512x512 -crop 46x46+1+44 $^ $@

part_fire.png: particles.png
	convert -crop 16x16+0+48 $^ $@

font_dat.h: font.dat
	xxd -i $^ > $@

clean:
	rm -f $(OBJS)
