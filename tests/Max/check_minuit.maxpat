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
      500,
      560
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
          "text": "prism.11 check: a Minuit device that leaves the network does not freeze Max; closes itself after 60 s"
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
            72,
            320,
            22
          ],
          "text": "loadbang"
        }
      },
      {
        "box": {
          "id": "on",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            104,
            320,
            22
          ],
          "text": "1"
        }
      },
      {
        "box": {
          "id": "m",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "bang"
          ],
          "patching_rect": [
            20,
            136,
            320,
            22
          ],
          "text": "metro 100"
        }
      },
      {
        "box": {
          "id": "beat",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            168,
            320,
            22
          ],
          "text": "/beat"
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
            200,
            320,
            22
          ],
          "text": "udpsend 127.0.0.1 41742"
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
            232,
            320,
            22
          ],
          "text": "ossia.client"
        }
      },
      {
        "box": {
          "id": "pc",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            264,
            320,
            22
          ],
          "text": "prepend /client"
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
            296,
            320,
            22
          ],
          "text": "udpreceive 41743"
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
            328,
            320,
            22
          ],
          "text": "route /connect /close /devices"
        }
      },
      {
        "box": {
          "id": "dv",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            360,
            360,
            120,
            22
          ],
          "text": "get devices"
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
            360,
            320,
            22
          ],
          "text": "prepend connect"
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
            20,
            392,
            320,
            22
          ],
          "text": "del 60000"
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
            424,
            320,
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
            456,
            320,
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
            "on",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "on",
            0
          ],
          "destination": [
            "m",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "m",
            0
          ],
          "destination": [
            "beat",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "beat",
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
            "cl",
            0
          ],
          "destination": [
            "pc",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "pc",
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
            "cl",
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
            "dv",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "dv",
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
            "rt",
            1
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
      }
    ]
  }
}