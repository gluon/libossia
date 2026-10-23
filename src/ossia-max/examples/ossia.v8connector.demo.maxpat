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
			745.0
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
					"text": "ossia.v8connector, Julien Bayle, PRISM (CNRS), https://www.prism.cnrs.fr/",
					"fontname": "Lato Regular",
					"fontsize": 12.0,
					"fontface": 1
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						40.0,
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
					"id": "obj-3",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20.0,
						75.0,
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
					"id": "obj-4",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"patching_rect": [
						230.0,
						75.0,
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
					"id": "obj-5",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						410.0,
						75.0,
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
					"id": "obj-6",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						105.0,
						80.0,
						22.0
					],
					"text": "print device"
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						150.0,
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
					"id": "obj-8",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						175.0,
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
					"id": "obj-9",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						205.0,
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
					"id": "obj-10",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						235.0,
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
					"id": "obj-11",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						280.0,
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
					"id": "obj-12",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						335.0,
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
					"id": "obj-13",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						375.0,
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
					"id": "obj-14",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						220.0,
						375.0,
						90.0,
						22.0
					],
					"text": "print ui_a"
				}
			},
			{
				"box": {
					"id": "obj-15",
					"maxclass": "v8ui",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20.0,
						410.0,
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
					"id": "obj-16",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						480.0,
						335.0,
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
					"id": "obj-17",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						480.0,
						375.0,
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
					"id": "obj-18",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						680.0,
						375.0,
						90.0,
						22.0
					],
					"text": "print ui_b"
				}
			},
			{
				"box": {
					"id": "obj-19",
					"maxclass": "v8ui",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						480.0,
						410.0,
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
					"id": "obj-20",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						540.0,
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
					"id": "obj-21",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						580.0,
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
					"id": "obj-22",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						20.0,
						610.0,
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
					"id": "obj-23",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						640.0,
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
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						300.0,
						580.0,
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
					"id": "obj-25",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						300.0,
						610.0,
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
					"id": "obj-26",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						300.0,
						640.0,
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
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						580.0,
						580.0,
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
					"id": "obj-28",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"patching_rect": [
						580.0,
						610.0,
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
					"id": "obj-29",
					"maxclass": "flonum",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						580.0,
						640.0,
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
					"id": "obj-30",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						685.0,
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
						"obj-3",
						0
					],
					"source": [
						"obj-4",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-6",
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
						"obj-10",
						0
					],
					"source": [
						"obj-9",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-15",
						0
					],
					"source": [
						"obj-13",
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
						"obj-13",
						1
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-19",
						0
					],
					"source": [
						"obj-17",
						0
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
						"obj-17",
						1
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
						"obj-23",
						0
					],
					"source": [
						"obj-22",
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
						"obj-26",
						0
					],
					"source": [
						"obj-25",
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
			},
			{
				"patchline": {
					"destination": [
						"obj-29",
						0
					],
					"source": [
						"obj-28",
						0
					]
				}
			}
		],
		"dependency_cache": [],
		"autosave": 0
	}
}
