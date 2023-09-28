# A Simple Language Puzzle

Author: Russell Emerine

Design: 
My game is a simple language puzzle where your
goal is to decipher the grammatical rules of a language.
The game gives you a visual representation of the information you have,
rather than telling it directly.

Text Drawing:
There is a program, [`render-glyphs.cpp`](render-glyphs.cpp),
that uses FreeType to produce a texture and some vertices for every glyph that
the font provides.
Those are stored in chunk formats,
the texture in its own format
and the vertices in the same format as any other `Mesh`.
The text is produced in the game by putting the rectangle
(actually two triangles) into the `Scene`'s list of `Drawable`s.
Glyphs are positioned from harfbuzz output.

TODO: I am mistimed things! I will submit now and complete later

Choices: (TODO: how does the game store choices and narrative? How are they authored? Anything nifty you want to point out?)

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

(TODO: describe the controls and (if needed) goals/strategy.)

Sources: (TODO: list a source URL for any assets you did not create yourself. Make sure you have a license for the asset.)

This game was built with [NEST](NEST.md).

