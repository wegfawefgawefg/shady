# Examples

The examples are small direct-WGSL projects. `basics` introduces coordinates and time;
`input` uses live controls; `textures`, `video`, and `audio` show channels; `buffers`
shows feedback; `raymarch` gives a compact 3D example; and `pills/frosted_glass.wgsl`
is the visual study that started Shady.

The pill study begins with one frosted capsule. Click where its center should sit and
drag away from that point to set its orientation. A fresh project uses a centered,
slightly tilted pose until the first click.

Every file defines the same `shade` function. No generated WGSL is checked in. Asset
files are deliberately tiny except for the credited Big Buck Bunny test clip retained
for video experiments.
