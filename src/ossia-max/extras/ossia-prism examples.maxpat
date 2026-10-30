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
        "rect": [ 368.0, 158.0, 790.0, 300.0 ],
        "showrootpatcherontab": 0,
        "showontab": 0,
        "boxes": [
            {
                "box": {
                    "fontname": "Lato Regular",
                    "fontsize": 12.0,
                    "id": "obj-1",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 20.0, 15.0, 411.0, 21.0 ],
                    "text": "ossia-prism, Julien Bayle, PRISM (CNRS), https://www.prism.cnrs.fr/"
                }
            },
            {
                "box": {
                    "fontname": "Lato Regular",
                    "fontsize": 12.0,
                    "fontface": 1,
                    "id": "obj-2",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 20.0, 41.0, 640.0, 21.0 ],
                    "text": "ossia-prism examples of ossia.v8connector: click a message to open its patcher."
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 20.0, 240.0, 55.0, 22.0 ],
                    "text": "pcontrol"
                }
            },
            {
                "box": {
                    "id": "obj-10",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 20.0, 80.0, 260.0, 22.0 ],
                    "text": "load ossia.v8connector.demo.maxpat"
                }
            },
            {
                "box": {
                    "fontname": "Lato Regular",
                    "fontsize": 12.0,
                    "id": "obj-11",
                    "linecount": 2,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 290.0, 74.0, 470.0, 35.0 ],
                    "text": "ossia.v8connector: one tree with an ossia.parameter placed by hand and two v8ui, one that declares its parameters through the object, one that binds to them."
                }
            },
            {
                "box": {
                    "id": "obj-12",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 20.0, 130.0, 260.0, 22.0 ],
                    "text": "load ossia.v8connector.grid.maxpat"
                }
            },
            {
                "box": {
                    "fontname": "Lato Regular",
                    "fontsize": 12.0,
                    "id": "obj-13",
                    "linecount": 2,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 290.0, 124.0, 470.0, 35.0 ],
                    "text": "ossia.v8connector: a click on a cell of a v8ui grid declares a parameter, a second click removes it; a jweb shows the served OSCQuery tree live."
                }
            },
            {
                "box": {
                    "id": "obj-14",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 20.0, 180.0, 260.0, 22.0 ],
                    "text": "load ossia.v8connector.load.maxpat"
                }
            },
            {
                "box": {
                    "fontname": "Lato Regular",
                    "fontsize": 12.0,
                    "id": "obj-15",
                    "linecount": 2,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 290.0, 174.0, 470.0, 35.0 ],
                    "text": "ossia.v8connector: 3000 parameters set at each frame and followed by a client in the same Max, drawn as a cloud of points; compare @bundle 1 and 0."
                }
            }
        ],
        "lines": [
            {
                "patchline": {
                    "destination": [ "obj-3", 0 ],
                    "source": [ "obj-10", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-3", 0 ],
                    "source": [ "obj-12", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-3", 0 ],
                    "source": [ "obj-14", 0 ]
                }
            }
        ],
        "autosave": 0
    }
}
