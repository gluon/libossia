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
      860,
      400
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
          "text": "prism.11 check: an ossia.remote, by URL and by name, gets the value of a node the server adds once (7441, 7442); closes itself after 60 s"
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
            50,
            320,
            22
          ],
          "text": "loadbang"
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
            82,
            320,
            22
          ],
          "text": "del 60000"
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
            120,
            320,
            22
          ],
          "text": "udpreceive 41745"
        }
      },
      {
        "box": {
          "id": "rt",
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
            152,
            320,
            22
          ],
          "text": "route /connect /close"
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
            184,
            320,
            22
          ],
          "text": "t b b"
        }
      },
      {
        "box": {
          "id": "mu",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            180,
            216,
            300,
            22
          ],
          "text": "connect oscquery ws://127.0.0.1:7442"
        }
      },
      {
        "box": {
          "id": "mn",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20,
            216,
            150,
            22
          ],
          "text": "connect prism11_added"
        }
      },
      {
        "box": {
          "id": "pu",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "outlettype": [],
          "patching_rect": [
            180,
            248,
            300,
            22
          ],
          "text": "p url",
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
              540,
              220
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
                  "id": "cl",
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
                    55,
                    260,
                    22
                  ],
                  "text": "ossia.client prism11_url"
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
                    90,
                    260,
                    22
                  ],
                  "text": "prepend /url_client"
                }
              },
              {
                "box": {
                  "id": "rm",
                  "maxclass": "newobj",
                  "numinlets": 1,
                  "numoutlets": 3,
                  "outlettype": [
                    "",
                    "",
                    ""
                  ],
                  "patching_rect": [
                    300,
                    55,
                    200,
                    22
                  ],
                  "text": "ossia.remote bench/late"
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
                    300,
                    90,
                    200,
                    22
                  ],
                  "text": "prepend /url"
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
                    300,
                    125,
                    200,
                    22
                  ],
                  "text": "prepend /url_dump"
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
                    160,
                    260,
                    22
                  ],
                  "text": "udpsend 127.0.0.1 41744"
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
                    "cl",
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
                    "rm",
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
                    "tx",
                    0
                  ]
                }
              },
              {
                "patchline": {
                  "source": [
                    "rm",
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
              }
            ]
          }
        }
      },
      {
        "box": {
          "id": "pn",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "outlettype": [],
          "patching_rect": [
            20,
            248,
            150,
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
              540,
              220
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
                  "id": "cl",
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
                    55,
                    260,
                    22
                  ],
                  "text": "ossia.client prism11_byname"
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
                    90,
                    260,
                    22
                  ],
                  "text": "prepend /name_client"
                }
              },
              {
                "box": {
                  "id": "rm",
                  "maxclass": "newobj",
                  "numinlets": 1,
                  "numoutlets": 3,
                  "outlettype": [
                    "",
                    "",
                    ""
                  ],
                  "patching_rect": [
                    300,
                    55,
                    200,
                    22
                  ],
                  "text": "ossia.remote bench/late"
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
                    300,
                    90,
                    200,
                    22
                  ],
                  "text": "prepend /name"
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
                    300,
                    125,
                    200,
                    22
                  ],
                  "text": "prepend /name_dump"
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
                    160,
                    260,
                    22
                  ],
                  "text": "udpsend 127.0.0.1 41744"
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
                    "cl",
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
                    "rm",
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
                    "tx",
                    0
                  ]
                }
              },
              {
                "patchline": {
                  "source": [
                    "rm",
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
              }
            ]
          }
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
            360,
            82,
            100,
            22
          ],
          "text": "/ready"
        }
      },
      {
        "box": {
          "id": "tr",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "outlettype": [],
          "patching_rect": [
            360,
            114,
            200,
            22
          ],
          "text": "udpsend 127.0.0.1 41744"
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
            290,
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
            322,
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
            "tr",
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
            "mu",
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
            "mn",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "mu",
            0
          ],
          "destination": [
            "pu",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "mn",
            0
          ],
          "destination": [
            "pn",
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
