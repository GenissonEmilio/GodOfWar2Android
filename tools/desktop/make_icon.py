"""Render the launcher's original omega emblem; no game assets are used."""
from pathlib import Path
import math
import cairo
surface=cairo.ImageSurface(cairo.FORMAT_ARGB32,256,256)
cr=cairo.Context(surface)
cr.set_source_rgb(.12,.07,.08);cr.paint()
cr.translate(128,143);cr.scale(1.12,1.12)
cr.set_source_rgb(.73,.51,.26);cr.set_line_width(17)
cr.arc(0,-15,64,math.pi*.72,math.pi*2.28);cr.stroke()
for sign in (-1,1):
 cr.move_to(sign*42,33);cr.line_to(sign*42,61);cr.line_to(sign*78,61);cr.stroke()
cr.set_source_rgba(.72,.10,.09,.9)
cr.move_to(22,-96);cr.line_to(52,-96);cr.line_to(-12,88);cr.line_to(-32,88);cr.close_path();cr.fill()
surface.write_to_png(str(Path(__file__).parent/'assets/icon.png'))
