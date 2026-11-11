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
      520,
      22
     ],
     "text": "prism.12 check, console: the device check_console (41761, 41762) and the client check_console_client, opened by tests/Max/check_console.py; closes itself after 90 s"
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
      520,
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
      520,
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
      520,
      22
     ],
     "text": "expose oscquery 41761 41762"
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
      520,
      22
     ],
     "text": "ossia.device check_console"
    }
   },
   {
    "box": {
     "id": "pl",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      160,
      520,
      22
     ],
     "text": "ossia.parameter level @type float"
    }
   },
   {
    "box": {
     "id": "dr",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      20,
      190,
      520,
      22
     ],
     "text": "delay 500"
    }
   },
   {
    "box": {
     "id": "mr",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      220,
      520,
      22
     ],
     "text": "/ready"
    }
   },
   {
    "box": {
     "id": "ur",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      250,
      520,
      22
     ],
     "text": "udpreceive 41766"
    }
   },
   {
    "box": {
     "id": "rt",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 4,
     "outlettype": [
      "",
      "",
      "",
      ""
     ],
     "patching_rect": [
      20,
      280,
      520,
      22
     ],
     "text": "route /connect /disconnect /close"
    }
   },
   {
    "box": {
     "id": "mc",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      310,
      520,
      22
     ],
     "text": "connect oscquery ws://127.0.0.1:41763"
    }
   },
   {
    "box": {
     "id": "td",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "bang",
      "bang"
     ],
     "patching_rect": [
      20,
      340,
      520,
      22
     ],
     "text": "t b b"
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
      370,
      520,
      22
     ],
     "text": "disconnect"
    }
   },
   {
    "box": {
     "id": "mdd",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      400,
      520,
      22
     ],
     "text": "/disconnected"
    }
   },
   {
    "box": {
     "id": "cl",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      430,
      520,
      22
     ],
     "text": "p client",
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
       120,
       120,
       500,
       300
      ],
      "showrootpatcherontab": 0,
      "showontab": 0,
      "autosave": 0,
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
          20,
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
          70,
          400,
          22
         ],
         "text": "ossia.client check_console_client @reconnect 0"
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
          120,
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
     "id": "pp",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      460,
      520,
      22
     ],
     "text": "prepend /client"
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
      490,
      520,
      22
     ],
     "text": "udpsend 127.0.0.1 41765"
    }
   },
   {
    "box": {
     "id": "d90",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      20,
      520,
      520,
      22
     ],
     "text": "delay 90000"
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
      550,
      520,
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
      580,
      520,
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
      "dr",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "dr",
      0
     ],
     "destination": [
      "mr",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "mr",
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
      "d90",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "d90",
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
   },
   {
    "patchline": {
     "source": [
      "ur",
      0
     ],
     "destination": [
      "rt",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "rt",
      0
     ],
     "destination": [
      "mc",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "mc",
      0
     ],
     "destination": [
      "cl",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "rt",
      1
     ],
     "destination": [
      "td",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "td",
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
      "cl",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "td",
      0
     ],
     "destination": [
      "mdd",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "mdd",
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
      "rt",
      2
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
      "cl",
      0
     ],
     "destination": [
      "pp",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "pp",
      0
     ],
     "destination": [
      "us",
      0
     ]
    }
   }
  ]
 }
}