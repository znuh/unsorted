#!/usr/bin/lua

local scale = 0.38

function parse_mod(fname)
	local tbl = {}
	local fh = io.open(fname)
	local xmin, xmax = math.huge, -math.huge
	local ymin, ymax = math.huge, -math.huge
	for line in fh:lines() do
		local x1, y1, x2, y2 = line:match("fp_line %(start (%S+) (%S+)%) %(end (%S+) (%S+)%) %(layer F.CrtYd%) ")
		local x, y, sz = line:match("pad %S+ smd circle %(at (%S+) (%S+)%) %(size (%S+)")
		if y2 then
			xmin = math.min(xmin, x1, x2)
			ymin = math.min(ymin, y1, y2)
			xmax = math.max(xmax, x1, x2)
			ymax = math.max(ymax, y1, y2)
		elseif sz then
			table.insert(tbl,{x=x,y=y,sz=sz/2})
		end
	end
	tbl.xmin=xmin
	tbl.ymin=ymin
	tbl.xmax=xmax
	tbl.ymax=ymax
	tbl.xs=xmax-xmin
	tbl.ys=ymax-ymin
	return tbl
end

function mkball(p)
	local template = [[Transform {
		translation    %f %f %f
		children [
		Shape {
		   appearance Appearance {
				   material Material {
					   diffuseColor    0.8 0.8 0.8
				}
			}
		   geometry Sphere {
			   radius    %f
			}
		}
		]
	}
	]]
	return string.format(template,p.x,p.y,p.z,p.sz)	
end

function mkbody(p)
	local template = [[Transform {
		translation    %f %f %f
		children [
		Shape {
		   appearance Appearance {
				   material Material {
					   diffuseColor    0.1 0.1 0.1
				}
			}
		   geometry Box {
			   size    %f %f %f
			}
		}
		]
	}
	]]
	return string.format(template,p.x,p.y,p.z,p.xs,p.ys,p.zs)
end

z_s = 0.6
z_ofs = 0.3

pkg = parse_mod(arg[1])

print("#VRML V2.0 utf8")
p={x=0,y=0,z=((z_s/2)+z_ofs)*scale,xs=pkg.xs*scale,ys=pkg.ys*scale,zs=z_s*scale};
print(mkbody(p))

for i=1,table.maxn(pkg) do
	local pad = pkg[i]
	pad.x=pad.x*scale
	pad.y=pad.y*scale
	pad.z=(z_ofs/2)*scale
	pad.sz=pad.sz*1.2*scale
	print(mkball(pad))
end

-- import & export with Freecad afterwards
-- KiCAD doesn't seem to support Box/Sphere geometries
