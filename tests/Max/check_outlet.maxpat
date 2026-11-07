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
            800,
            22
          ],
          "text": "prism.11 check: a value of the network and a value of the inlet of ossia.parameter at the same time (7431, 7432); closes itself after 180 s"
        }
      },
      {
        "box": {
          "id": "dev",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            ""
          ],
          "patching_rect": [
            20,
            72,
            260,
            22
          ],
          "text": "ossia.device prism11_outlet"
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
            104,
            260,
            22
          ],
          "text": "expose oscquery 7431 7432"
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
            136,
            260,
            22
          ],
          "text": "loadbang"
        }
      },
      {
        "box": {
          "id": "par",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            ""
          ],
          "patching_rect": [
            20,
            168,
            260,
            22
          ],
          "text": "ossia.parameter level @type float @default 0.5"
        }
      },
      {
        "box": {
          "id": "tt",
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
            200,
            260,
            22
          ],
          "text": "t f f f"
        }
      },
      {
        "box": {
          "id": "pd",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            232,
            260,
            22
          ],
          "text": "prepend /direct"
        }
      },
      {
        "box": {
          "id": "tx",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "outlettype": [],
          "patching_rect": [
            20,
            264,
            260,
            22
          ],
          "text": "udpsend 127.0.0.1 41740"
        }
      },
      {
        "box": {
          "id": "pr",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            296,
            260,
            22
          ],
          "text": "prepend rx"
        }
      },
      {
        "box": {
          "id": "js",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            ""
          ],
          "patching_rect": [
            20,
            328,
            260,
            22
          ],
          "text": "js check_outlet.js"
        }
      },
      {
        "box": {
          "id": "gate",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            360,
            260,
            22
          ],
          "text": "gate 1 0"
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
            392,
            260,
            22
          ],
          "text": "t b b"
        }
      },
      {
        "box": {
          "id": "uzi",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 3,
          "outlettype": [
            "bang",
            "bang",
            "int"
          ],
          "patching_rect": [
            20,
            424,
            260,
            22
          ],
          "text": "uzi 1"
        }
      },
      {
        "box": {
          "id": "pl",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            456,
            260,
            22
          ],
          "text": "+ 1"
        }
      },
      {
        "box": {
          "id": "done",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            488,
            260,
            22
          ],
          "text": "/slow_done"
        }
      },
      {
        "box": {
          "id": "rx",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            520,
            260,
            22
          ],
          "text": "udpreceive 41741"
        }
      },
      {
        "box": {
          "id": "rt",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 6,
          "outlettype": [
            "",
            "",
            "",
            "",
            "",
            ""
          ],
          "patching_rect": [
            20,
            552,
            260,
            22
          ],
          "text": "route /go /slow /count /workaround /close"
        }
      },
      {
        "box": {
          "id": "pg",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            584,
            260,
            22
          ],
          "text": "prepend go"
        }
      },
      {
        "box": {
          "id": "dm",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "bang"
          ],
          "patching_rect": [
            300,
            40,
            260,
            22
          ],
          "text": "del 180000"
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
            300,
            72,
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
            300,
            104,
            260,
            22
          ],
          "text": "thispatcher"
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
            300,
            136,
            260,
            22
          ],
          "text": "del 1500"
        }
      },
      {
        "box": {
          "id": "rd",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            300,
            168,
            260,
            22
          ],
          "text": "/ready"
        }
      },
      {
        "box": {
          "id": "pw",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            580,
            400,
            260,
            22
          ],
          "text": "prepend workaround"
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
            "par",
            0
          ],
          "destination": [
            "tt",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "tt",
            2
          ],
          "destination": [
            "pd",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "pd",
            0
          ],
          "destination": [
            "tx",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "tt",
            1
          ],
          "destination": [
            "pr",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "pr",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "js",
            0
          ],
          "destination": [
            "par",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "js",
            1
          ],
          "destination": [
            "tx",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "tt",
            0
          ],
          "destination": [
            "gate",
            1
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "gate",
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
            "uzi",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "uzi",
            2
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
            "tb",
            0
          ],
          "destination": [
            "done",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "done",
            0
          ],
          "destination": [
            "tx",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "rx",
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
            "pg",
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
            "js",
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
            "gate",
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
            "uzi",
            1
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "lb",
            0
          ],
          "destination": [
            "dm",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "dm",
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
            "lb",
            0
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
            "rd",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "rd",
            0
          ],
          "destination": [
            "tx",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "rt",
            3
          ],
          "destination": [
            "pw",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "pw",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "rt",
            4
          ],
          "destination": [
            "dsp",
            0
          ]
        }
      }
    ]
  }
}