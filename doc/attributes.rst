.. _sec_attributes:

Attributes
==========

.. c:type:: termpaint_attr

The attributes object contains paint settings used when painting on a :doc:`surface<surface>`. It consists of the
foreground color, the background color and the decoration color. In addition it has a set of enabled styles.

.. _colors:

Colors
------

Terminal colors are a bit complicated for historical reasons. Even if colors seem the same they are all distinct and
terminals can (and do) act differently depending on how the color was exactly selected.

.. _default-colors:

Default colors
..............

The most basic color is :c:macro:`TERMPAINT_DEFAULT_COLOR`. It's the terminals default color. This color acts differently
when used as foreground, background or decoration color. For foreground and background it's the respective default color.
For decoration color the default color means that the terminal will use the foreground color of that particular cell.

.. _named-colors:

Named colors
............

Next there is the set of named colors. The first 8 named colors are also often called ANSI colors. These colors are
have constants like ``TERMPAINT_COLOR_RED`` and can also be  referred to as :c:macro:`TERMPAINT_NAMED_COLOR` + color
number. The first 8 colors are supported by almost all color
terminals. The following 8 colors are still very widely supported. The named colors have names like "red", but terminal
implementations often allow reconfiguring these colors from easy accessible settings dialogs. Expect named colors to
have different concrete color values for many users.

.. container:: hidden-references

  .. c:macro:: TERMPAINT_COLOR_BLACK
  .. c:macro:: TERMPAINT_COLOR_RED
  .. c:macro:: TERMPAINT_COLOR_GREEN
  .. c:macro:: TERMPAINT_COLOR_YELLOW
  .. c:macro:: TERMPAINT_COLOR_BLUE
  .. c:macro:: TERMPAINT_COLOR_MAGENTA
  .. c:macro:: TERMPAINT_COLOR_CYAN
  .. c:macro:: TERMPAINT_COLOR_LIGHT_GREY
  .. c:macro:: TERMPAINT_COLOR_DARK_GREY
  .. c:macro:: TERMPAINT_COLOR_BRIGHT_RED
  .. c:macro:: TERMPAINT_COLOR_BRIGHT_GREEN
  .. c:macro:: TERMPAINT_COLOR_BRIGHT_YELLOW
  .. c:macro:: TERMPAINT_COLOR_BRIGHT_BLUE
  .. c:macro:: TERMPAINT_COLOR_BRIGHT_MAGENTA
  .. c:macro:: TERMPAINT_COLOR_BRIGHT_CYAN
  .. c:macro:: TERMPAINT_COLOR_WHITE

.. table:: Named Colors
  :align: left

  ======  ==============================  ====
  Number  Constant                        Name
  ======  ==============================  ====
  0       TERMPAINT_COLOR_BLACK           black
  1       TERMPAINT_COLOR_RED             red
  2       TERMPAINT_COLOR_GREEN           green
  3       TERMPAINT_COLOR_YELLOW          yellow
  4       TERMPAINT_COLOR_BLUE            blue
  5       TERMPAINT_COLOR_MAGENTA         magenta
  6       TERMPAINT_COLOR_CYAN            cyan
  7       TERMPAINT_COLOR_LIGHT_GREY      "white" (on terminals supporting 16 colors this is light gray)
  8       TERMPAINT_COLOR_DARK_GREY       dark gray
  9       TERMPAINT_COLOR_BRIGHT_RED      bright red
  10      TERMPAINT_COLOR_BRIGHT_GREEN    bright green
  11      TERMPAINT_COLOR_BRIGHT_YELLOW   bright yellow
  12      TERMPAINT_COLOR_BRIGHT_BLUE     bright blue
  13      TERMPAINT_COLOR_BRIGHT_MAGENTA  bright magenta
  14      TERMPAINT_COLOR_BRIGHT_CYAN     bright cyan
  15      TERMPAINT_COLOR_WHITE           bright white
  ======  ==============================  ====

.. _indexed-colors:

Indexed colors
..............

The next color space is :c:macro:`TERMPAINT_INDEXED_COLOR`. This is a indexed color space. For most terminals it has
256 colors. Some terminals only implement 88 colors though. Per convention the first 16 colors (0-15) are the same as
the named colors. Some terminals handle these differently in combination with the :c:macro:`TERMPAINT_STYLE_BOLD`
style. For terminals supporting 256 colors in the default palette the rest of the indices are divided in a 6x6x6 color
cube and a 23 step gray ramp (indices 232-255). The defaults are the color cube uses intensity levels of
[0, 95, 135, 175, 215, 255] and calculates the components as red is (index-16) / 36, green is (index-16) / 6) % 6 and
blue (index-16) % 6. The gray ramp uses the intensity levels of
[8, 18, 28, 38, 48, 58, 68, 78, 88, 98, 108, 118, 128, 138, 148, 158, 168, 178, 188, 198, 208, 218, 228, 238]. Of course
the index colors are redefinable so users might have a changed palette active.

For some terminal implementations using index colors leads to garbled display because not all terminals support parsing
the needed control sequences.

.. image:: color256.png

.. _rgb-colors:

RGB colors
..........

The last color space is :c:macro:`TERMPAINT_RGB_COLOR`. This is a direct color space which does not allow redefining
colors. A color is specified by red, green and blue intensities in the range 0 to 255. For example
``TERMPAINT_RGB_COLOR(255, 128, 64)``

For some terminal implementations using direct rgb colors leads to garbled display because not all terminals support
parsing the needed control sequences. Termpaint automatically translates RGB colors to indexed colors using the default
palette when terminal capability detection yields that the terminal can not handle them. This detection is not always
exact and can be switched to a more conservative mode by disabling the capability ``TERMPAINT_CAPABILITY_TRUECOLOR_MAYBE_SUPPORTED``.
In the opposite direction promising the capability ``TERMPAINT_CAPABILITY_TRUECOLOR_SUPPORTED`` disables translation to
index colors.

.. c:macro:: TERMPAINT_RGB_COLOR_OFFSET(red, green, blue)

    A macro that returns a rgb color composed of the given values for red, green and blue. The value range
    is 0 to 255 for each color component

Styles
------

Clusters may have one or more styles. The style changes how characters are displayed. The
:c:macro:`TERMPAINT_STYLE_INVERSE` style also applies to blank cells.

Styles can be enabled and disabled by using the :c:func:`termpaint_attr_set_style()` and
:c:func:`termpaint_attr_unset_style()` functions. These functions take one or more of the style macros combined with
bitwise or (``|``).

Attribute support varies with terminal implementation.

.. table:: Available styles
  :align: left

  ===================  =========
  Style                Macro
  ===================  =========
  bold                 :c:macro:`TERMPAINT_STYLE_BOLD`
  inverse              :c:macro:`TERMPAINT_STYLE_INVERSE`
  italic               :c:macro:`TERMPAINT_STYLE_ITALIC`
  blink                :c:macro:`TERMPAINT_STYLE_BLINK`
  underline            :c:macro:`TERMPAINT_STYLE_UNDERLINE`
  double underline     :c:macro:`TERMPAINT_STYLE_UNDERLINE_DBL`
  curly underline      :c:macro:`TERMPAINT_STYLE_UNDERLINE_CURLY`
  strikethrough        :c:macro:`TERMPAINT_STYLE_STRIKE`
  overline             :c:macro:`TERMPAINT_STYLE_OVERLINE`
  ===================  =========

Functions
---------

See :ref:`safety` for general rules for calling functions in termpaint.

.. c:macro:: TERMPAINT_DEFAULT_COLOR

  A Macro used to denote the terminals default color

.. c:macro:: TERMPAINT_NAMED_COLOR

  A Macro used to denote the first of the named colors. For example use ``TERMPAINT_NAMED_COLOR + 1`` to denote red.

.. c:macro:: TERMPAINT_INDEXED_COLOR

  A Macro used to denote the first indexed color. For example use ``TERMPAINT_NAMED_COLOR + 243`` to denote a mid gray.

.. c:macro:: TERMPAINT_RGB_COLOR(r, g, b)

  A Macro used to denote the rgb direct colors. Parameters are in the range 0 to 255.

.. c:function:: termpaint_attr* termpaint_attr_new(unsigned fg, unsigned bg)

  Creates a new attributes object with the foreground color ``fg`` and background color ``bg``. No styles will
  be selected.

  The application has to free this with :c:func:`termpaint_attr_free`.

.. c:function:: termpaint_attr* termpaint_attr_clone(termpaint_attr* attr)

  Creates a new attributes object with the same settings as the attributes object passed in ``attr``.

  The application has to free this with :c:func:`termpaint_attr_free`.

.. c:function:: void termpaint_attr_free(termpaint_attr* attr)

  Frees a attributes object allocated with :c:func:`termpaint_attr_new()` or :c:func:`termpaint_attr_clone()`.

.. c:function:: void termpaint_attr_set_fg(termpaint_attr* attr, unsigned fg)

  Set the foreground to be used when painting to ``fg``.

.. c:function:: void termpaint_attr_set_bg(termpaint_attr* attr, unsigned bg)

  Set the background to be used when painting to ``bg``.

.. c:function:: void termpaint_attr_set_deco(termpaint_attr* attr, unsigned deco_color)

  Set the decoration color to be used when painting to ``deco_color``.

.. c:macro:: TERMPAINT_STYLE_BOLD

  Style the text in bold. `(widely supported) <https://terminalguide.netlify.com/attr/1/>`__

  Some terminal implementations change named colors in the range 0-7 to their bright variants when rendering bold text.

.. c:macro:: TERMPAINT_STYLE_ITALIC

  Style the text in italic. `(widely supported) <https://terminalguide.netlify.com/attr/3/>`__

.. c:macro:: TERMPAINT_STYLE_BLINK

  Text should blink. `(support varies by terminal implementation) <https://terminalguide.netlify.com/attr/5/>`__

.. c:macro:: TERMPAINT_STYLE_OVERLINE

  Style the text with a overline. `(limited support in terminal implementations) <https://terminalguide.netlify.com/attr/53/>`__

.. c:macro:: TERMPAINT_STYLE_INVERSE

  Display the text with inverted foreground and background color. `(widely supported) <https://terminalguide.netlify.com/attr/7/>`__

.. c:macro:: TERMPAINT_STYLE_STRIKE

  Style the text in strikethrough. `(support varies by terminal implementation) <https://terminalguide.netlify.com/attr/9/>`__

.. c:macro:: TERMPAINT_STYLE_UNDERLINE

  Style the text with a single underline. `(widely supported) <https://terminalguide.netlify.com/attr/4/>`__

  If supported by the terminal emulator the underline uses the decoration color.

.. c:macro:: TERMPAINT_STYLE_UNDERLINE_DBL

  Style the text with a double underline. `(limited support in terminal implementations) <https://terminalguide.netlify.com/attr/21/>`__

  If supported by the terminal emulator the underline uses the decoration color.

.. c:macro:: TERMPAINT_STYLE_UNDERLINE_CURLY

  Style the text with a curly underline. `(limited support in terminal implementations) <https://terminalguide.netlify.com/attr/4-3/>`__

  If supported by the terminal emulator the underline uses the decoration color.

.. c:function:: void termpaint_attr_set_style(termpaint_attr* attr, int bits)

  Adds the styles given in ``bits`` to the attributes.

.. c:function:: void termpaint_attr_unset_style(termpaint_attr* attr, int bits)

  Removes the styles given in ``bits`` to the attributes.

.. c:function:: void termpaint_attr_reset_style(termpaint_attr* attr)

  Removes all previously set styles.

.. c:function:: void termpaint_attr_set_patch(termpaint_attr* attr, _Bool optimize, const char *setup, const char * cleanup)

  This function allows to use additional attributes for rendering that are not otherwise explicitly supported.

  .. warning:: This is a low level feature with potential to garble the whole terminal rendering. Use with care.

  Allows setting escape sequences to be output before (``setup``) and after (``cleanup``) rendering each cluster with
  this style. If ``optimize`` is set, do not use ``setup`` and ``cleanup`` between clusters that have the exact same
  patch.

  The caller is responsible to ensure that the patches don't break rendering. Setup is output after the "select graphics
  rendition" escape sequence right before the text of the cluster is output. If ``optimize`` is not set cleanup is output
  directly following the text of the cluster.

  If ``optimize`` is true, the setup sequence must not contain "select graphics rendition" sequences because the
  rendering resets SGR state between clusters if styles change in a display run.
