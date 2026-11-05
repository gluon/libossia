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
     "text": "prism.10 checks: a device on 127.0.0.1 (7419, 7420), and a second expose on the ports of prism10_check; closes itself after 40 s"
    }
   },
   {
    "box": {
     "id": "us",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      40,
      300,
      22
     ],
     "text": "udpsend 127.0.0.1 41730"
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
      70,
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
      100,
      300,
      22
     ],
     "text": "t b b b"
    }
   },
   {
    "box": {
     "id": "e3",
     "maxclass": "message",
     "numinlets": 2,
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
     "text": "expose oscquery 7419 7420 127.0.0.1"
    }
   },
   {
    "box": {
     "id": "dl",
     "maxclass": "newobj",
     "numinlets": 1,
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
     "text": "ossia.device prism10_local"
    }
   },
   {
    "box": {
     "id": "e2",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      190,
      300,
      22
     ],
     "text": "expose oscquery 7417 7418"
    }
   },
   {
    "box": {
     "id": "dd",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      220,
      300,
      22
     ],
     "text": "p dup",
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
       100,
       100,
       340,
       160
      ],
      "boxes": [
       {
        "box": {
         "id": "in",
         "maxclass": "inlet",
         "numinlets": 0,
         "numoutlets": 1,
         "outlettype": [
          ""
         ],
         "patching_rect": [
          20,
          15,
          30,
          30
         ]
        }
       },
       {
        "box": {
         "id": "obj",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 1,
         "outlettype": [
          ""
         ],
         "patching_rect": [
          20,
          50,
          280,
          22
         ],
         "text": "ossia.device prism10_dup"
        }
       },
       {
        "box": {
         "id": "out",
         "maxclass": "outlet",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20,
          90,
          30,
          30
         ]
        }
       }
      ],
      "lines": [
       {
        "patchline": {
         "source": [
          "in",
          0
         ],
         "destination": [
          "obj",
          0
         ]
        }
       },
       {
        "patchline": {
         "source": [
          "obj",
          0
         ],
         "destination": [
          "out",
          0
         ]
        }
       }
      ]
     }
    }
   },
   {
    "box": {
     "id": "re",
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
     "text": "route expose"
    }
   },
   {
    "box": {
     "id": "zl",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "patching_rect": [
      20,
      280,
      300,
      22
     ],
     "text": "zl.nth 1"
    }
   },
   {
    "box": {
     "id": "o",
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
     "text": "/check/dup_expose $1"
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
      340,
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
      370,
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
      400,
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
      "e3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "e3",
      0
     ],
     "destination": [
      "dl",
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
      "e2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "e2",
      0
     ],
     "destination": [
      "dd",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "dd",
      0
     ],
     "destination": [
      "re",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "re",
      0
     ],
     "destination": [
      "zl",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "zl",
      0
     ],
     "destination": [
      "o",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "o",
      0
     ],
     "destination": [
      "us",
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
