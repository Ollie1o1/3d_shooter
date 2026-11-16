# Shepherd anatomical reference data

The mesh and morph targets in this directory are CC0 assets from MakeHuman,
not MakeHuman application source. They are used by `../../shepherd_anatomy.py`
to create the edited adult face, torso and arm surfaces in Blender. The game's
runtime loads only the resulting GLBs and has no MakeHuman dependency.

Upstream: https://github.com/makehumancommunity/makehuman
Pinned revision: `a8bc2d54ff0ac92e78ff71431b1023eda42bf482`

License: the upstream [asset license](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/LICENSE.ASSETS.md), also saved here as `LICENSE.ASSETS.md`.
The upstream [license summary](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/LICENSE.md#c-the-license-for-the-bundled-assets) explicitly covers the base mesh and targets under CC0. Original notices remain in the data files.

Source paths:

- `base.obj`: `makehuman/data/3dobjs/base.obj`
- `caucasian-male-young.target`, `universal-male-young-maxmuscle-averageweight.target`: `makehuman/data/targets/macrodetails/`
- `head-fat-decr.target`, `head-rectangular.target`: `makehuman/data/targets/head/`
- `chin-prominent-incr.target`, `chin-width-incr.target`: `makehuman/data/targets/chin/`
- `nose-greek-incr.target`: `makehuman/data/targets/nose/`

Morph weights and the model's edits are specified in `shepherd_anatomy.py`.
The head is not the previous procedural face with a new material: eyelids,
lips, nose, jaw and cheeks now come from the anatomical topology. The limbs
are extracted and fitted along the game's articulated skeleton. Hull armor,
keels, capsule docks, magnetic palms and finger armor are original geometry
in `shepherd_build.py`.

SHA-256 of the retained upstream data:

- `LICENSE.ASSETS.md`: `f6089cba01cb570a24712b41ab8a586ccd3cc5ef53dc266ca50b95c288956d2c`
- `base.obj`: `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c`
- `caucasian-male-young.target`: `70e228ba7164737dae664454394536fc5935fa48d333c1a97d77e2dc6eacc5f5`
- `chin-prominent-incr.target`: `d2493ca5f69aabeb8557d77a56c49cfcf2062ff90c0cad03073d87990d11448b`
- `chin-width-incr.target`: `deef506bfe1883d9d4c38667332c9970785226566efbae4724fd3d60c02efcf4`
- `head-fat-decr.target`: `00473b6069f9cf6c05a2f1fd0c0bae20bb4ec45e9552bb5695f1d1533f3cc119`
- `head-rectangular.target`: `bbfcbd4d6334df18515e73abb707b8a948adb74e0aba88a728c8e146dd5f4e4b`
- `nose-greek-incr.target`: `26377458cdc453ff93a9b025c3a6043b4c5f3000173fb125e5b2a7cc66981855`
- `universal-male-young-maxmuscle-averageweight.target`: `c171ff9cc95e96273beb3e3b1969a01988c2d4982ebc6fe04bf81cf72ee3b295`
