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
   80,
   80,
   900,
   700
  ],
  "showrootpatcherontab": 0,
  "showontab": 0,
  "autosave": 0,
  "boxes": [
   {
    "box": {
     "id": "c",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      10,
      700,
      22
     ],
     "text": "prism.10 checks: the device prism10_check, opened by tests/Max/check_max.py, closes itself after 40 s"
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
      40,
      300,
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
      70,
      300,
      22
     ],
     "text": "t b b b"
    }
   },
   {
    "box": {
     "id": "e1",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      100,
      300,
      22
     ],
     "text": "expose oscquery 7417 7418"
    }
   },
   {
    "box": {
     "id": "dv",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      130,
      300,
      22
     ],
     "text": "ossia.device prism10_check"
    }
   },
   {
    "box": {
     "id": "pf",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      160,
      300,
      22
     ],
     "text": "ossia.parameter flag @type bool"
    }
   },
   {
    "box": {
     "id": "pp",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      190,
      300,
      22
     ],
     "text": "ossia.parameter pos @type vec3f @min 0 @max 1"
    }
   },
   {
    "box": {
     "id": "pd",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      220,
      300,
      22
     ],
     "text": "ossia.parameter dumpme @type float @default 0.5"
    }
   },
   {
    "box": {
     "id": "md",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      250,
      300,
      22
     ],
     "text": "dump"
    }
   },
   {
    "box": {
     "id": "rd",
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
      280,
      300,
      22
     ],
     "text": "ossia.remote dumpme"
    }
   },
   {
    "box": {
     "id": "pg",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      310,
      300,
      22
     ],
     "text": "ossia.parameter go @type impulse"
    }
   },
   {
    "box": {
     "id": "m01",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      340,
      300,
      22
     ],
     "text": "0.1"
    }
   },
   {
    "box": {
     "id": "pl",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      370,
      300,
      22
     ],
     "text": "ossia.parameter lvl @type float @range 0 1 @hold 500"
    }
   },
   {
    "box": {
     "id": "po",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      400,
      300,
      22
     ],
     "text": "ossia.parameter lvl_out @type float"
    }
   },
   {
    "box": {
     "id": "dlc",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      20,
      430,
      300,
      22
     ],
     "text": "delay 40000"
    }
   },
   {
    "box": {
     "id": "dsp",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      460,
      300,
      22
     ],
     "text": "dispose"
    }
   },
   {
    "box": {
     "id": "tp",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      490,
      300,
      22
     ],
     "text": "thispatcher"
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
      "e1",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "e1",
      0
     ],
     "destination": [
      "dv",
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
      "md",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "md",
      0
     ],
     "destination": [
      "rd",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "pg",
      0
     ],
     "destination": [
      "m01",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m01",
      0
     ],
     "destination": [
      "pl",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "pl",
      0
     ],
     "destination": [
      "po",
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
      "dlc",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "dlc",
      0
     ],
     "destination": [
      "dsp",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "dsp",
      0
     ],
     "destination": [
      "tp",
      0
     ]
    }
   }
  ]
 }
}