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
     "id": "c0",
     "maxclass": "comment",
     "numinlets": 0,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      10,
      600,
      22
     ],
     "text": "prism.10 checks, client side: opened by tests/Max/check_max.py after check_device.maxpat, closes itself after 12 s"
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
      260,
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
      260,
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
      260,
      22
     ],
     "text": "t b b b"
    }
   },
   {
    "box": {
     "id": "cn",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      130,
      260,
      22
     ],
     "text": "ossia.client prism10_named"
    }
   },
   {
    "box": {
     "id": "k1",
     "maxclass": "newobj",
     "numinlets": 3,
     "numoutlets": 4,
     "outlettype": [
      "int",
      "",
      "",
      "int"
     ],
     "patching_rect": [
      20,
      160,
      260,
      22
     ],
     "text": "counter 1 1000000"
    }
   },
   {
    "box": {
     "id": "m2",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      190,
      260,
      22
     ],
     "text": "connect oscquery ws://127.0.0.1:41739"
    }
   },
   {
    "box": {
     "id": "cr",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      220,
      260,
      22
     ],
     "text": "p rec",
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
         "text": "ossia.client prism10_rec @reconnect 0"
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
     "id": "r2",
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
      260,
      22
     ],
     "text": "route connect"
    }
   },
   {
    "box": {
     "id": "k2",
     "maxclass": "newobj",
     "numinlets": 3,
     "numoutlets": 4,
     "outlettype": [
      "int",
      "",
      "",
      "int"
     ],
     "patching_rect": [
      20,
      280,
      260,
      22
     ],
     "text": "counter 1 1000000"
    }
   },
   {
    "box": {
     "id": "m3",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      310,
      260,
      22
     ],
     "text": "connect prism10_check"
    }
   },
   {
    "box": {
     "id": "cc",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      340,
      260,
      22
     ],
     "text": "p byname",
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
         "text": "ossia.client prism10_byname"
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
     "id": "r3",
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
      370,
      260,
      22
     ],
     "text": "route parameter node"
    }
   },
   {
    "box": {
     "id": "k3",
     "maxclass": "newobj",
     "numinlets": 3,
     "numoutlets": 4,
     "outlettype": [
      "int",
      "",
      "",
      "int"
     ],
     "patching_rect": [
      20,
      400,
      260,
      22
     ],
     "text": "counter 1 1000000"
    }
   },
   {
    "box": {
     "id": "dly",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      20,
      430,
      260,
      22
     ],
     "text": "delay 6000"
    }
   },
   {
    "box": {
     "id": "tr",
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
      460,
      260,
      22
     ],
     "text": "t b b b"
    }
   },
   {
    "box": {
     "id": "g1",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "int"
     ],
     "patching_rect": [
      20,
      490,
      260,
      22
     ],
     "text": "i"
    }
   },
   {
    "box": {
     "id": "g2",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "int"
     ],
     "patching_rect": [
      20,
      520,
      260,
      22
     ],
     "text": "i"
    }
   },
   {
    "box": {
     "id": "g3",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "int"
     ],
     "patching_rect": [
      20,
      550,
      260,
      22
     ],
     "text": "i"
    }
   },
   {
    "box": {
     "id": "o1",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      580,
      260,
      22
     ],
     "text": "/check/named_outputs $1"
    }
   },
   {
    "box": {
     "id": "o2",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      610,
      260,
      22
     ],
     "text": "/check/reconnect0_connects $1"
    }
   },
   {
    "box": {
     "id": "o3",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      640,
      260,
      22
     ],
     "text": "/check/byname_tree_changes $1"
    }
   },
   {
    "box": {
     "id": "d12",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      20,
      670,
      260,
      22
     ],
     "text": "delay 12000"
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
      700,
      260,
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
      730,
      260,
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
      "m2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m2",
      0
     ],
     "destination": [
      "cr",
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
      "m3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m3",
      0
     ],
     "destination": [
      "cc",
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
      "dly",
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
      "d12",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "cn",
      0
     ],
     "destination": [
      "k1",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "cr",
      0
     ],
     "destination": [
      "r2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "r2",
      0
     ],
     "destination": [
      "k2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "cc",
      0
     ],
     "destination": [
      "r3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "r3",
      0
     ],
     "destination": [
      "k3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "r3",
      1
     ],
     "destination": [
      "k3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "k1",
      0
     ],
     "destination": [
      "g1",
      1
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "k2",
      0
     ],
     "destination": [
      "g2",
      1
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "k3",
      0
     ],
     "destination": [
      "g3",
      1
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "dly",
      0
     ],
     "destination": [
      "tr",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tr",
      2
     ],
     "destination": [
      "g1",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tr",
      1
     ],
     "destination": [
      "g2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "tr",
      0
     ],
     "destination": [
      "g3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "g1",
      0
     ],
     "destination": [
      "o1",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "g2",
      0
     ],
     "destination": [
      "o2",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "g3",
      0
     ],
     "destination": [
      "o3",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "o1",
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
      "o2",
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
      "o3",
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
      "d12",
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
