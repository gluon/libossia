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
     "text": "prism.10 checks: [ossia], the global device, exposed on 7421, 7422; bgo sets ba and bb in one tick; closes itself after 40 s"
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
     "numoutlets": 2,
     "outlettype": [
      "bang",
      "bang"
     ],
     "patching_rect": [
      20,
      70,
      300,
      22
     ],
     "text": "t b b"
    }
   },
   {
    "box": {
     "id": "e4",
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
     "text": "expose oscquery 7421 7422"
    }
   },
   {
    "box": {
     "id": "og",
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
     "text": "ossia"
    }
   },
   {
    "box": {
     "id": "mbd",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      160,
      300,
      22
     ],
     "text": "bundle 1"
    }
   },
   {
    "box": {
     "id": "pa",
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
     "text": "ossia.parameter ba @type float"
    }
   },
   {
    "box": {
     "id": "pb",
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
     "text": "ossia.parameter bb @type float"
    }
   },
   {
    "box": {
     "id": "pbg",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      250,
      300,
      22
     ],
     "text": "ossia.parameter bgo @type impulse"
    }
   },
   {
    "box": {
     "id": "tb2",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "bang",
      "bang"
     ],
     "patching_rect": [
      20,
      280,
      300,
      22
     ],
     "text": "t b b"
    }
   },
   {
    "box": {
     "id": "ma",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      310,
      300,
      22
     ],
     "text": "0.25"
    }
   },
   {
    "box": {
     "id": "mb",
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
     "text": "0.75"
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
      370,
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
      400,
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
      430,
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
      1
     ],
     "destination": [
      "mbd",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "mbd",
      0
     ],
     "destination": [
      "og",
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
      "e4",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "e4",
      0
     ],
     "destination": [
      "og",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "pbg",
      0
     ],
     "destination": [
      "tb2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tb2",
      1
     ],
     "destination": [
      "ma",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "ma",
      0
     ],
     "destination": [
      "pa",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tb2",
      0
     ],
     "destination": [
      "mb",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "mb",
      0
     ],
     "destination": [
      "pb",
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