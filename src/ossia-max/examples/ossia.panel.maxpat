{
 "patcher": {
  "fileversion": 1,
  "appversion": {
   "major": 9,
   "minor": 2,
   "revision": 0,
   "architecture": "x64",
   "modernui": 1
  },
  "classnamespace": "box",
  "rect": [
   100.0,
   100.0,
   880.0,
   680.0
  ],
  "showrootpatcherontab": 0,
  "showontab": 0,
  "autosave": 0,
  "boxes": [
   {
    "box": {
     "id": "c1",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      10,
      600,
      20
     ],
     "text": "ossia panel and @hold, Julien Bayle, PRISM (CNRS), ossia-prism"
    }
   },
   {
    "box": {
     "id": "c2",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      32,
      600,
      48
     ],
     "text": "The device hold_demo is exposed on 7415 (OSC) and 7416 (HTTP, WebSocket) when the patch opens. The page on the right is ossia.panel.html, a client of it: it builds a control per parameter. Move a control there: Max receives it."
    }
   },
   {
    "box": {
     "id": "lb",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      20,
      90,
      60,
      22
     ],
     "text": "loadbang"
    }
   },
   {
    "box": {
     "id": "tb",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 3,
     "outlettype": [
      "bang",
      "bang",
      "bang"
     ],
     "patching_rect": [
      20,
      120,
      60,
      22
     ],
     "text": "t b b b"
    }
   },
   {
    "box": {
     "id": "exp",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      160,
      170,
      22
     ],
     "text": "expose oscquery 7415 7416"
    }
   },
   {
    "box": {
     "id": "dev",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      195,
      160,
      22
     ],
     "text": "ossia.device hold_demo"
    }
   },
   {
    "box": {
     "id": "prt",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      225,
      100,
      22
     ],
     "text": "print hold_demo"
    }
   },
   {
    "box": {
     "id": "c3",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      265,
      300,
      76
     ],
     "text": "level: @hold 500. While you move the slider, the values of the network (the page) are held, then the last one comes out 500 ms after you let go. Move the page's control and this slider together: the one you hold keeps its value."
    }
   },
   {
    "box": {
     "id": "sl",
     "maxclass": "slider",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      345,
      20,
      140
     ],
     "floatoutput": 1,
     "size": 1.0,
     "parameter_enable": 0
    }
   },
   {
    "box": {
     "id": "lv",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 3,
     "outlettype": [
      "",
      "",
      ""
     ],
     "patching_rect": [
      60,
      400,
      330,
      22
     ],
     "text": "ossia.parameter level @type float @range 0 1 @hold 500"
    }
   },
   {
    "box": {
     "id": "st",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      60,
      440,
      50,
      22
     ],
     "text": "set $1"
    }
   },
   {
    "box": {
     "id": "c4",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      60,
      470,
      330,
      34
     ],
     "text": "Without @hold, the slider and the page fight while both move."
    }
   },
   {
    "box": {
     "id": "onp",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 3,
     "outlettype": [
      "",
      "",
      ""
     ],
     "patching_rect": [
      20,
      520,
      190,
      22
     ],
     "text": "ossia.parameter on @type bool"
    }
   },
   {
    "box": {
     "id": "tg",
     "maxclass": "toggle",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      "int"
     ],
     "patching_rect": [
      20,
      550,
      24,
      24
     ],
     "parameter_enable": 0
    }
   },
   {
    "box": {
     "id": "col",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 3,
     "outlettype": [
      "",
      "",
      ""
     ],
     "patching_rect": [
      20,
      590,
      360,
      22
     ],
     "text": "ossia.parameter color @type vec3f @unit color.rgb @range 0 1"
    }
   },
   {
    "box": {
     "id": "d1",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      110,
      160,
      60,
      22
     ],
     "text": "delay 500"
    }
   },
   {
    "box": {
     "id": "rf",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      420,
      160,
      160,
      22
     ],
     "text": "readfile ossia.panel.html"
    }
   },
   {
    "box": {
     "id": "d2",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      200,
      120,
      70,
      22
     ],
     "text": "delay 1500"
    }
   },
   {
    "box": {
     "id": "ej",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      420,
      195,
      300,
      22
     ],
     "text": "executejavascript \"ossiaPanelConnect('127.0.0.1:7416')\""
    }
   },
   {
    "box": {
     "id": "jw",
     "maxclass": "jweb",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      420,
      230,
      420,
      420
     ],
     "rendermode": 2
    }
   },
   {
    "box": {
     "id": "st2",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      230.0,
      550.0,
      50.0,
      22.0
     ],
     "text": "set $1"
    }
   }
  ],
  "lines": [
   {
    "patchline": {
     "source": [
      "lb",
      0
     ],
     "destination": [
      "tb",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tb",
      2
     ],
     "destination": [
      "exp",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "exp",
      0
     ],
     "destination": [
      "dev",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "dev",
      0
     ],
     "destination": [
      "prt",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tb",
      1
     ],
     "destination": [
      "d1",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "d1",
      0
     ],
     "destination": [
      "rf",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "rf",
      0
     ],
     "destination": [
      "jw",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tb",
      0
     ],
     "destination": [
      "d2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "d2",
      0
     ],
     "destination": [
      "ej",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "ej",
      0
     ],
     "destination": [
      "jw",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "sl",
      0
     ],
     "destination": [
      "lv",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "lv",
      0
     ],
     "destination": [
      "st",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "st",
      0
     ],
     "destination": [
      "sl",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tg",
      0
     ],
     "destination": [
      "onp",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "onp",
      0
     ],
     "destination": [
      "st2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "st2",
      0
     ],
     "destination": [
      "tg",
      0
     ]
    }
   }
  ]
 }
}