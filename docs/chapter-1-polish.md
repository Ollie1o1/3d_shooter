# Chapter 1 polish: navigation, recovery and environmental detail

The chapter should feel like a factory that kept running after its workers
failed. Each addition therefore has a job in the scene: feed, collect, service,
inspect or discard. The aim is a place whose history can be understood while
moving, with optional records available at the player's pace.

## Research applied

- [The Level Design Book: Wayfinding](https://book.leveldesignbook.com/process/blockout/wayfinding)
  treats navigation as giving players information and useful landmarks.
  Application: consistent cyan readable lamps, an ASSEMBLY sign on the service
  lift, numbered freight landings, and a plainly labelled salvage transfer.
  The optional logs remain optional: only pressing interact records a discovery.
- [The Level Design Book: Composition](https://book.leveldesignbook.com/process/blockout/massing/composition)
  emphasizes spatial hierarchy and contrast in height, density, orientation and
  shape across many possible viewing angles. Application: dense salvage islands
  at the pit's edges, a comparatively open combat lane, low lights beside the
  new props, and visible destinations beyond the clutter. Collision must follow
  visible structure; broad invisible boxes undermine that spatial information.
- [The Level Design Book: Storytelling](https://book.leveldesignbook.com/process/env-art/storytelling)
  distinguishes evidence of past events from activity happening in front of the
  player. Application: damaged transfer hardware, spilled mechanical remains,
  discarded bodies and oil establish past failures; dust falling from the inlet,
  extractor fans, production machinery and occasional sparks show ongoing work.

These are design interpretations applied to this chapter, not claims that the
sources prescribe these exact colours, prop counts or traversal timings.

## Implementation and checks

The eight tiny shaft ledges after the Lancer are replaced by an automatic lift.
It provides a short recovery beat after the junction fight, with no repeated
jump/dash requirement. It rises in 4.5 seconds and returns to a player below.
The existing freight challenge retains its three defended stops. A recessed
floor and foot-contact boarding check fix the failure to depart.

Four authored Blender props add about 110 KB of GLB assets. The scrap pit has
78 discarded bodies (previously 34), collection bins, scattered panels and
cables, structural supports and warm work lights. Assembly's services, the
Archive's ordered machinery and the freight floor numbers give those spaces
different rhythms. New tall props sit beside movement routes; loose fragments
stay low. The existing pit climb and both secret routes remain traversable.

`make test` exercises both lifts using the real Player and the same lift logic
as gameplay, verifies all three freight stops hold, tests readable targeting
and wall occlusion, and checks scripted spawn clearance. The existing physics,
combat and boss suites also run. `make light` regenerates matching baked light.
Native screenshots check the entrance, pit, readable prompt, service deck,
freight shaft and Archive. A complete editable chapter scene can be rebuilt
with `make level-dump` and `tools/blender/level_import.py`.

The automated traversal and screenshots cannot establish how every route feels
to a human player. The useful next playtest is a normal run from the belt:
notice the salvage route, distinguish taking a weapon from reading a log,
board both lifts, and assess the pit's visibility while enemies are active.

## Native previews

[Reject transfer entrance](story-chapter-1/polish-entrance.png) ·
[Scrap pit](story-chapter-1/polish-pit.png) ·
[Readable beacon and E prompt](story-chapter-1/polish-readable.png) ·
[Service lift](story-chapter-1/polish-service-lift.png)

Final verification: native build, all four test suites, matching chapter light
bake, level export and Blender scene import completed successfully. The native
GPU regression target also passes with scenes exceeding 40,000 model parts.
The editable full scene is `build/levels/story1/cradle.blend`.

## Rendering, waypoint and boarding corrections

The extra bodies exposed a fixed 16,384-instance limit in `BoxRenderer`.
Geometry after that limit was silently dropped, including active enemies,
weapon models and pickups. The instance buffer now grows with the full frame;
both the cube-only and mixed-mesh draw paths submit every instance.
`make test-render` creates a native GL context and reads the rendered pixel of
an object after 18,001 and 40,001 instances. Those checks failed against the
old renderer and pass with the growing buffer. This target needs a display
(`xvfb-run make test-render` on a headless Linux runner).

Weapons and hostiles now share one world-marker projection. Behind-camera
targets stay at the edge with an arrow and BEHIND label. Rear-left and rear-right
retain their correct turn directions; labels stay within the viewport. Final
hostile markers follow the interpolated model's centre and include the type,
distance, floor difference and a BEHIND COVER hint where appropriate.

The service entrance also had a gap between the Lancer room's allowed volume
(ending at x=46) and the shaft (starting at x=46.4). Physical collision alone
allowed walking, so the old test passed; gameplay's room constraint clamped
walking at x=46. The room volume now reaches the shaft and a matching solid
threshold fills the floor seam. The smaller pit/duct seam is also bridged.
Traversal tests now run the same room constraint as gameplay while walking
onto active lifts, with no jump or dash. The new service test reproduced the
old failure at x=46 before the geometry correction.

Verified in-game views: [weapon ahead](story-chapter-1/waypoint-front.png),
[weapon behind](story-chapter-1/waypoint-behind.png), and
[final hostiles, including one behind cover](story-chapter-1/last-hostiles.png).
