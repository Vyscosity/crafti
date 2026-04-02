OBJS := font_bmp.h font_dat.h inventory.h inventory2.h crafting_table.h inv_selection.h loading.h loadingtext.h menu.h selection.h terrain.h terrain2.h blockselection.h part_fire.h items_texture.h steve.h

all: $(OBJS)

%.h: %.bmp
	ConvertImg --format=ngl $^ $@

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
