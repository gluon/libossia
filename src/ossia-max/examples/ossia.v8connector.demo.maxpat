{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 4,
			"architecture": "x64",
			"modernui": 1
		},
		"classnamespace": "box",
		"rect": [
			100.0,
			100.0,
			980.0,
			720.0
		],
		"gridsize": [
			15.0,
			15.0
		],
		"showrootpatcherontab": 0,
		"showontab": 0,
		"originid": "pat-9100",
		"boxes": [
			{
				"box": {
					"id": "obj-1",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						15.0,
						700.0,
						21.0
					],
					"text": "ossia.v8connector demo: one tree, an ossia.parameter placed by hand and two v8ui that declare their parameters.",
					"fontname": "Lato Regular",
					"fontsize": 12.0,
					"fontface": 1
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20.0,
						50.0,
						190.0,
						22.0
					],
					"outlettype": [
						""
					],
					"text": "ossia.device v8connector_demo"
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						230.0,
						50.0,
						170.0,
						22.0
					],
					"outlettype": [
						""
					],
					"text": "expose oscquery 7001 7002"
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						410.0,
						50.0,
						420.0,
						21.0
					],
					"text": "optional: publish the tree (OSC port, OSCQuery port). Both ports must be free.",
					"fontname": "Lato Regular",
					"fontsize": 12.0
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						80.0,
						80.0,
						22.0
					],
					"text": "print device"
				}
			},
			{
				"box": {
					"id": "obj-6",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						125.0,
						400.0,
						21.0
					],
					"text": "1. a parameter placed by hand:",
					"fontname": "Lato Regular",
					"fontsize": 12.0
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						150.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-8",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						180.0,
						380.0,
						22.0
					],
					"outlettype": [
						"",
						""
					],
					"text": "ossia.parameter manual/level @type float @min 0 @max 1 @default 0.5"
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						210.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-10",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						255.0,
						900.0,
						48.0
					],
					"text": "2. two v8ui, each with its own ossia.v8connector, one parameter set. The left script declares gain, cutoff and voices under the node ui (add). The right script creates nothing: it binds to ui/gain, ui/cutoff and ui/voices, the parameters of the left one, and mirrors them.",
					"fontname": "Lato Regular",
					"fontsize": 12.0,
					"linecount": 3
				}
			},
			{
				"box": {
					"id": "obj-11",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						310.0,
						440.0,
						35.0
					],
					"text": "scripting name ui_a, add mode: the script finds the object by that name, no cord to it",
					"fontname": "Lato Regular",
					"fontsize": 12.0,
					"linecount": 2
				}
			},
			{
				"box": {
					"id": "obj-12",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						350.0,
						180.0,
						22.0
					],
					"outlettype": [
						"",
						""
					],
					"text": "ossia.v8connector ui",
					"varname": "ui_a",
					"color": [
						0.0,
						0.933333333333333,
						1.0,
						1.0
					],
					"saved_attribute_attributes": {
						"color": {
							"expression": "themecolor.live_threshold_line_color"
						}
					},
					"saved_newobj_attribute_attributes": {
						"color": {
							"expression": "themecolor.live_threshold_line_color"
						}
					}
				}
			},
			{
				"box": {
					"id": "obj-13",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						220.0,
						350.0,
						90.0,
						22.0
					],
					"text": "print ui_a"
				}
			},
			{
				"box": {
					"id": "obj-14",
					"maxclass": "v8ui",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20.0,
						385.0,
						300.0,
						120.0
					],
					"outlettype": [
						""
					],
					"border": 0,
					"filename": "ossia.v8connector.demo.js",
					"jsarguments": [
						"ui_a"
					]
				}
			},
			{
				"box": {
					"id": "obj-15",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						480.0,
						310.0,
						440.0,
						35.0
					],
					"text": "scripting name ui_b, bind mode: drag here, the left v8ui and the remotes follow; drag there, this one follows. Remove the left connector: this one waits and rebinds when it comes back.",
					"fontname": "Lato Regular",
					"fontsize": 12.0,
					"linecount": 2
				}
			},
			{
				"box": {
					"id": "obj-16",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						480.0,
						350.0,
						180.0,
						22.0
					],
					"outlettype": [
						"",
						""
					],
					"text": "ossia.v8connector mirror",
					"varname": "ui_b",
					"color": [
						0.0,
						0.933333333333333,
						1.0,
						1.0
					],
					"saved_attribute_attributes": {
						"color": {
							"expression": "themecolor.live_threshold_line_color"
						}
					},
					"saved_newobj_attribute_attributes": {
						"color": {
							"expression": "themecolor.live_threshold_line_color"
						}
					}
				}
			},
			{
				"box": {
					"id": "obj-17",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						680.0,
						350.0,
						90.0,
						22.0
					],
					"text": "print ui_b"
				}
			},
			{
				"box": {
					"id": "obj-18",
					"maxclass": "v8ui",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						480.0,
						385.0,
						300.0,
						120.0
					],
					"outlettype": [
						""
					],
					"border": 0,
					"filename": "ossia.v8connector.demo.js",
					"jsarguments": [
						"ui_b",
						"bind",
						"ui"
					]
				}
			},
			{
				"box": {
					"id": "obj-19",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						515.0,
						900.0,
						35.0
					],
					"text": "3. other ossia objects listening to the same addresses. Drag a slider in either v8ui: the matching remote follows. Type a value here: both v8ui follow (the values come back through the connectors' left outlets).",
					"fontname": "Lato Regular",
					"fontsize": 12.0,
					"linecount": 2
				}
			},
			{
				"box": {
					"id": "obj-20",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						555.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-21",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20.0,
						585.0,
						190.0,
						22.0
					],
					"outlettype": [
						"",
						"",
						""
					],
					"text": "ossia.remote ui/gain"
				}
			},
			{
				"box": {
					"id": "obj-22",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						615.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-23",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						300.0,
						555.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-24",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						300.0,
						585.0,
						190.0,
						22.0
					],
					"outlettype": [
						"",
						"",
						""
					],
					"text": "ossia.remote ui/cutoff"
				}
			},
			{
				"box": {
					"id": "obj-25",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						300.0,
						615.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-26",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						580.0,
						555.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-27",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						580.0,
						585.0,
						190.0,
						22.0
					],
					"outlettype": [
						"",
						"",
						""
					],
					"text": "ossia.remote ui/voices"
				}
			},
			{
				"box": {
					"id": "obj-28",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						580.0,
						615.0,
						60.0,
						22.0
					],
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-29",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						660.0,
						900.0,
						35.0
					],
					"text": "A script's own set never comes back to it; it reaches the network, the remotes above and the other script. A declaration without @min and @max for a numeric type is refused (see the Max console).",
					"fontname": "Lato Regular",
					"fontsize": 12.0,
					"linecount": 2
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"destination": [
						"obj-2",
						0
					],
					"source": [
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-5",
						0
					],
					"source": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-8",
						0
					],
					"source": [
						"obj-7",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-9",
						0
					],
					"source": [
						"obj-8",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-14",
						0
					],
					"source": [
						"obj-12",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-13",
						0
					],
					"source": [
						"obj-12",
						1
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-18",
						0
					],
					"source": [
						"obj-16",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-17",
						0
					],
					"source": [
						"obj-16",
						1
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-21",
						0
					],
					"source": [
						"obj-20",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-22",
						0
					],
					"source": [
						"obj-21",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-24",
						0
					],
					"source": [
						"obj-23",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-25",
						0
					],
					"source": [
						"obj-24",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-27",
						0
					],
					"source": [
						"obj-26",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-28",
						0
					],
					"source": [
						"obj-27",
						0
					]
				}
			}
		],
		"dependency_cache": [],
		"autosave": 0
	}
}
