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
			47.0,
			115.0,
			879.0,
			831.0
		],
		"gridsize": [
			15.0,
			15.0
		],
		"showrootpatcherontab": 0,
		"showontab": 0,
		"originid": "pat-9001",
		"boxes": [
			{
				"box": {
					"id": "obj-1",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						165.0,
						172.0,
						200.0,
						22.0
					],
					"outlettype": [
						""
					],
					"text": "ossia.device v8connector_maxhelp"
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "newobj",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						165.0,
						242.0,
						50.0,
						22.0
					],
					"text": "p Basic",
					"saved_object_attributes": {
						"globalpatchername": ""
					},
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
							47.0,
							141.0,
							879.0,
							805.0
						],
						"gridsize": [
							15.0,
							15.0
						],
						"showontab": 1,
						"originid": "pat-9002",
						"boxes": [
							{
								"box": {
									"id": "obj-1",
									"maxclass": "jsui",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										8.0,
										7.0,
										561.5,
										143.0
									],
									"outlettype": [
										""
									],
									"border": 0,
									"filename": "helpdetails.js",
									"ignoreclick": 1,
									"jsarguments": [
										"ossia.v8connector"
									],
									"parameter_enable": 0
								}
							},
							{
								"box": {
									"id": "obj-2",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										165.0,
										420.0,
										62.0
									],
									"text": "ossia.v8connector creates a node of the ossia tree, like ossia.model, but its parameters are declared by messages instead of ossia.parameter objects. The usual sender is a v8 or v8ui script (see the Script tab); the messages below do the same thing by hand.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 4
								}
							},
							{
								"box": {
									"id": "obj-3",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										30.0,
										240.0,
										410.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "add gain @type float @min -60 @max 6 @unit gain.db @default -12 @clip both @description \"main gain\""
								}
							},
							{
								"box": {
									"id": "obj-4",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										30.0,
										270.0,
										290.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "add voices @type int @min 1 @max 16 @default 4"
								}
							},
							{
								"box": {
									"id": "obj-5",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										30.0,
										300.0,
										200.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "add cutoff @type float @unit time.hz"
								}
							},
							{
								"box": {
									"id": "obj-6",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										240.0,
										300.0,
										230.0,
										35.0
									],
									"text": "refused: a numeric type needs @min and @max (or @range)",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 2
								}
							},
							{
								"box": {
									"id": "obj-7",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										30.0,
										340.0,
										310.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "add mode @type string @range sine saw square @default sine"
								}
							},
							{
								"box": {
									"id": "obj-8",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										375.0,
										420.0,
										21.0
									],
									"text": "set pushes a value; it is clipped to the declared range (@clip both here):",
									"fontname": "Lato Regular",
									"fontsize": 12.0
								}
							},
							{
								"box": {
									"id": "obj-9",
									"maxclass": "flonum",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										30.0,
										400.0,
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
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										30.0,
										430.0,
										80.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "set gain $1"
								}
							},
							{
								"box": {
									"id": "obj-11",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										130.0,
										430.0,
										90.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "remove voices"
								}
							},
							{
								"box": {
									"id": "obj-12",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										230.0,
										430.0,
										40.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "clear"
								}
							},
							{
								"box": {
									"id": "obj-13",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										290.0,
										430.0,
										75.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "namespace"
								}
							},
							{
								"box": {
									"id": "obj-14",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										30.0,
										480.0,
										300.0,
										22.0
									],
									"outlettype": [
										"",
										""
									],
									"text": "ossia.v8connector synth @description \"declared by messages\"",
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
									"id": "obj-15",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										530.0,
										85.0,
										22.0
									],
									"text": "print values"
								}
							},
							{
								"box": {
									"id": "obj-16",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										180.0,
										530.0,
										85.0,
										22.0
									],
									"text": "print status"
								}
							},
							{
								"box": {
									"id": "obj-17",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										560.0,
										420.0,
										48.0
									],
									"text": "Left outlet: <address> <value> for every value changed by another source (network, other ossia objects). A set sent to this instance never comes back here. Right outlet: address, add, remove, clear and error messages.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 3
								}
							},
							{
								"box": {
									"id": "obj-18",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										625.0,
										420.0,
										48.0
									],
									"text": "The node registers like ossia.model: under the nearest ossia.model, ossia.device or ossia.client in the patcher hierarchy (here the ossia.device of the parent patcher), otherwise under the global device.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 3
								}
							},
							{
								"box": {
									"id": "obj-19",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										690.0,
										420.0,
										35.0
									],
									"text": "Two instances with the same name become synth and synth.1, like two ossia.model.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 2,
									"fontface": 1
								}
							},
							{
								"box": {
									"id": "obj-20",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										735.0,
										420.0,
										62.0
									],
									"text": "Initialization follows ossia-max: the default of a leaf is stored quietly, then comes out of the left outlet once the leaf exists; bind outputs the value it finds as soon as it reaches the parameter. @priority is stored on the nodes but does not order these outputs: ossia-max orders the outputs of ossia.parameter and ossia.remote only.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 4
								}
							},
							{
								"box": {
									"id": "obj-21",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										490.0,
										165.0,
										360.0,
										76.0
									],
									"text": "Every other ossia object sees the same tree. This remote listens to synth/gain: a set from the connector reaches it, and a value typed here reaches the connector's left outlet (print values). Add gain first.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 4
								}
							},
							{
								"box": {
									"id": "obj-22",
									"maxclass": "flonum",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										490.0,
										250.0,
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
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 3,
									"patching_rect": [
										490.0,
										280.0,
										170.0,
										22.0
									],
									"outlettype": [
										"",
										"",
										""
									],
									"text": "ossia.remote synth/gain"
								}
							},
							{
								"box": {
									"id": "obj-24",
									"maxclass": "flonum",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										490.0,
										320.0,
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
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										490.0,
										370.0,
										360.0,
										35.0
									],
									"text": "An ossia.parameter placed by hand lives next to the declared parameters, in the same tree:",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 2
								}
							},
							{
								"box": {
									"id": "obj-26",
									"maxclass": "flonum",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										490.0,
										410.0,
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
									"numoutlets": 2,
									"patching_rect": [
										490.0,
										440.0,
										330.0,
										22.0
									],
									"outlettype": [
										"",
										""
									],
									"text": "ossia.parameter manual @type float @min 0 @max 1 @default 0.5"
								}
							},
							{
								"box": {
									"id": "obj-28",
									"maxclass": "flonum",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										490.0,
										480.0,
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
										490.0,
										530.0,
										360.0,
										62.0
									],
									"text": "Attributes of add use the names of ossia.parameter: @type (required), @min, @max, @range, @unit, @default, @clip, @mode, @description, @tags, @priority, @rate, @critical, @invisible, @repetitions, @enable, @recall_safe.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 4
								}
							},
							{
								"box": {
									"id": "obj-30",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										490.0,
										605.0,
										360.0,
										76.0
									],
									"text": "bind follows a parameter that exists elsewhere, without creating anything: here the ossia.parameter above, addressed like an ossia.remote would (relative to the device). set pushes to it with the owner's type and range; its changes come back on the left outlet under the bound address.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 5
								}
							},
							{
								"box": {
									"id": "obj-31",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										490.0,
										690.0,
										80.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "bind manual"
								}
							},
							{
								"box": {
									"id": "obj-32",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										580.0,
										690.0,
										90.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "unbind manual"
								}
							},
							{
								"box": {
									"id": "obj-33",
									"maxclass": "flonum",
									"numinlets": 1,
									"numoutlets": 2,
									"patching_rect": [
										690.0,
										690.0,
										50.0,
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
									"id": "obj-34",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										690.0,
										720.0,
										100.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "set manual $1"
								}
							}
						],
						"lines": [
							{
								"patchline": {
									"destination": [
										"obj-14",
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
										"obj-14",
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
										"obj-14",
										0
									],
									"source": [
										"obj-5",
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
										"obj-7",
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
										"obj-10",
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
										"obj-11",
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
										"obj-14",
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
										"obj-14",
										0
									]
								}
							},
							{
								"patchline": {
									"destination": [
										"obj-16",
										0
									],
									"source": [
										"obj-14",
										1
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
							},
							{
								"patchline": {
									"destination": [
										"obj-14",
										0
									],
									"source": [
										"obj-31",
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
										"obj-32",
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
										"obj-34",
										0
									]
								}
							},
							{
								"patchline": {
									"destination": [
										"obj-34",
										0
									],
									"source": [
										"obj-33",
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
					"id": "obj-3",
					"maxclass": "newobj",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						230.0,
						242.0,
						50.0,
						22.0
					],
					"text": "p Script",
					"saved_object_attributes": {
						"globalpatchername": ""
					},
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
							47.0,
							141.0,
							879.0,
							805.0
						],
						"gridsize": [
							15.0,
							15.0
						],
						"showontab": 1,
						"originid": "pat-9003",
						"boxes": [
							{
								"box": {
									"id": "obj-1",
									"maxclass": "jsui",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										8.0,
										7.0,
										561.5,
										143.0
									],
									"outlettype": [
										""
									],
									"border": 0,
									"filename": "helpdetails.js",
									"ignoreclick": 1,
									"jsarguments": [
										"ossia.v8connector"
									],
									"parameter_enable": 0
								}
							},
							{
								"box": {
									"id": "obj-2",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										165.0,
										800.0,
										48.0
									],
									"text": "From a v8 or v8ui script, the helper javascript/ossia.v8connector.js reaches the object through its scripting name (varname in the inspector), without a patch cord. One cord brings the values back: the left outlet of the object into the inlet of the script.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 3
								}
							},
							{
								"box": {
									"id": "obj-3",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										225.0,
										800.0,
										260.0
									],
									"text": "const OssiaConnector = require(\"ossia.v8connector\");\nconst synth = new OssiaConnector(\"synth\");   // scripting name of [ossia.v8connector synth]\n\nfunction loadbang() {\n  synth.declare({\n    gain:   { type: \"float\", min: -60, max: 6, unit: \"gain.db\", default: -12 },\n    cutoff: { type: \"float\", range: [20, 20000], unit: \"time.hz\" },\n    voices: { type: \"int\", min: 1, max: 16, default: 4 },\n  });\n  synth.on(\"gain\", (v) => { gain = v; });\n}\nfunction anything() { synth.dispatch(messagename, arrayfromargs(arguments)); }\n\nsynth.set(\"gain\", -6);      // to the network and to the other ossia objects, not back to this script\nsynth.remove(\"voices\");",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 15
								}
							},
							{
								"box": {
									"id": "obj-4",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										500.0,
										800.0,
										62.0
									],
									"text": "Declarations are kept by the object: add can be sent before the object has registered (for instance from the script's loadbang). set is refused until the node exists; the right outlet sends 'address <global address>' when it does. A numeric type without @min and @max (or @range) is refused.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 4
								}
							},
							{
								"box": {
									"id": "obj-5",
									"maxclass": "comment",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										30.0,
										580.0,
										800.0,
										35.0
									],
									"text": "The demo patcher ossia.v8connector.demo.maxpat (examples folder of the package) mixes two v8ui that declare through the object with an ossia.parameter placed by hand, in one tree.",
									"fontname": "Lato Regular",
									"fontsize": 12.0,
									"linecount": 2
								}
							},
							{
								"box": {
									"id": "obj-6",
									"maxclass": "message",
									"numinlets": 2,
									"numoutlets": 1,
									"patching_rect": [
										30.0,
										625.0,
										250.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "load ossia.v8connector.demo.maxpat"
								}
							},
							{
								"box": {
									"id": "obj-7",
									"maxclass": "newobj",
									"numinlets": 1,
									"numoutlets": 1,
									"patching_rect": [
										30.0,
										655.0,
										55.0,
										22.0
									],
									"outlettype": [
										""
									],
									"text": "pcontrol"
								}
							}
						],
						"lines": [
							{
								"patchline": {
									"destination": [
										"obj-7",
										0
									],
									"source": [
										"obj-6",
										0
									]
								}
							}
						]
					}
				}
			}
		],
		"lines": [],
		"dependency_cache": [],
		"autosave": 0
	}
}
